// ReSharper disable CppTooWideScopeInitStatement
#include "Common.hh"
#include "ipc/connection_impl.hh"

#ifndef O_BINARY
# define O_BINARY 0
#endif
#ifndef SOCK_CLOEXEC
# define SOCK_CLOEXEC 0
#endif

#ifdef _WIN32
# define DWSOCKET_ERROR static_cast<DWORD>(SOCKET_ERROR)
# define ERRNO          WSAGetLastError()
# define ERR(t)         util::win32::error_exit_wsa(L"" t)
#else
# define ERRNO errno
# define ERR(t) err(t)
#endif

#ifndef SOCKET_ERROR
# define SOCKET_ERROR (-1)
#endif

#define READ_FD     0
#define WRITE_FD    1
#define INPUT_PIPE  0
#define OUTPUT_PIPE 1

#define AUTOC auto const


inline namespace emlsp {
namespace ipc::detail::base {
/****************************************************************************************/


#ifdef _WIN32
using native_descriptor_t = HANDLE;
using rw_param_t          = unsigned;
#else
using native_descriptor_t = int;
using rw_param_t          = size_t;
#endif


/*======================================================================================*/
/*======================================================================================*/
#ifdef _WIN32
/*======================================================================================*/
/*======================================================================================*/


namespace win32 {

static __forceinline bool
create_process_normal(wchar_t const       *args,
                      STARTUPINFOW        *start_info,
                      PROCESS_INFORMATION *pi)
{
      static constexpr DWORD flags = CREATE_DEFAULT_ERROR_MODE;
      return ::CreateProcessW(nullptr, const_cast<LPWSTR>(args), nullptr, nullptr,
                              true, flags, nullptr, nullptr, start_info, pi);
}

static __forceinline bool
create_process_detours(wchar_t const       *args,
                       STARTUPINFOW        *start_info,
                       PROCESS_INFORMATION *pi)
{
      static constexpr DWORD flags = CREATE_SUSPENDED | CREATE_DEFAULT_ERROR_MODE;

#if defined _DEBUG || !defined NDEBUG
      static constexpr char const *const dlls[] = {
          R"(D:\ass\VisualStudio\Repos\dumb_injector.dll\x64\Debug\dumb_injector.dll)", nullptr
      };
#else
      static constexpr char const *const dlls[] = {
          R"(D:\ass\VisualStudio\Repos\dumb_injector.dll\x64\Release\dumb_injector.dll)", nullptr
      };
#endif

      return ::DetourCreateProcessWithDllsW(
            nullptr,
            const_cast<LPWSTR>(args),
            nullptr, nullptr, true,
            flags,
            nullptr, nullptr,
            start_info, pi,
            static_cast<DWORD>(std::size(dlls) - SIZE_C(1)),
            const_cast<LPCSTR *>(dlls),
            nullptr
      );
}


template <typename Elem>
static std::wstring
protect_argv(intc argc, Elem **argv)
{
      std::wstring str;
      str.reserve(8192);

      for (int i = 0; i < argc; ++i) {
            Elem *p           = argv[i];
            int   pre_bslash  = 0;
            bool  need_quotes = false;

            while (*p) {
                  if (*p == ' ' || *p == '\t')
                        need_quotes = true;
                  ++p;
            }

            p = argv[i];
            if (need_quotes)
                  str += '"';

            while (*p) {
                  if (*p == '"') {
                        str += '\\';
                        for (; pre_bslash > 0; --pre_bslash)
                              str += '\\';
                  }

                  *p == '\\' ? ++pre_bslash : pre_bslash = 0;
                  str += *p++;
            }

            if (need_quotes) {
                  for (; pre_bslash > 0; --pre_bslash)
                        str += '\\';
                  str += '"';
            }

            if (i != argc - 1)
                  str += ' ';
      }

      return str;
}


static procinfo_t
spawn_connection_common_detours(HANDLE       const   err_fd,
                                size_t       const   argc,
                                char               **argv,
                                std::wstring const  &stype,
                                std::wstring const  &sname,
                                bool         const   dual,
                                uint16_t     const   hport = 0U)
{
      static constexpr wchar_t devnull[] = L"NUL";
      static std::mutex        mtx;

      PROCESS_INFORMATION pid{};
      SECURITY_ATTRIBUTES sec_attr = {
          .nLength              = sizeof(::SECURITY_ATTRIBUTES),
          .lpSecurityDescriptor = nullptr,
          .bInheritHandle       = true,
      };

      AUTOC tmphand_in  = ::CreateFileW(devnull, GENERIC_READ, 0, &sec_attr,
                                        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
      AUTOC tmphand_out = ::CreateFileW(devnull, GENERIC_WRITE, 0, &sec_attr,
                                        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);

      AUTOC        safe_argv = protect_argv(static_cast<int>(argc), argv);
      STARTUPINFOW startinfo = {};
      startinfo.cb           = sizeof startinfo;
      startinfo.dwFlags      = STARTF_USESTDHANDLES;
      startinfo.hStdInput    = tmphand_in;
      startinfo.hStdOutput   = tmphand_out;
      startinfo.hStdError    = err_fd;

      if (!::SetHandleInformation(startinfo.hStdError, HANDLE_FLAG_INHERIT, 1))
            util::win32::error_exit(L"Stderr SetHandleInformation");
      if (!::SetHandleInformation(GetStdHandle(STD_ERROR_HANDLE), HANDLE_FLAG_INHERIT, 1))
            util::win32::error_exit(L"Stderr SetHandleInformation");

      std::lock_guard lock(mtx);

      if (!::SetEnvironmentVariableW(L"EMLSP_SERVER_TYPE", stype.c_str()))
            util::win32::error_exit(L"SetEnvironmentVariableW");
      if (!::SetEnvironmentVariableW(L"EMLSP_SERVER_NAME", sname.c_str()))
            util::win32::error_exit(L"SetEnvironmentVariableW");
      if (!::SetEnvironmentVariableW(L"EMLSP_SERVER_PORT", fmt::to_wstring(hport).c_str()))
            util::win32::error_exit(L"SetEnvironmentVariableW");
      if (!::SetEnvironmentVariableW(L"EMLSP_SERVER_DUAL", dual ? L"1" : L"0"))
            util::win32::error_exit(L"SetEnvironmentVariableW");
      if (!::SetEnvironmentVariableW(L"EMLSP_TRUE_STDERR", std::to_wstring((uintptr_t)::GetStdHandle(STD_ERROR_HANDLE)).c_str()))
            util::win32::error_exit(L"SetEnvironmentVariableW");

      if (!create_process_detours(safe_argv.c_str(), &startinfo, &pid))
            util::win32::error_exit(L"create_process_detours()");

      ::CloseHandle(tmphand_in);
      ::CloseHandle(tmphand_out);

      return pid;
}


static __inline HANDLE socket_to_handle(SOCKET const sock)
{
      return sock == 0 || sock == invalid_socket ? INVALID_HANDLE_VALUE
                                                 : reinterpret_cast<HANDLE>(sock);
}


static procinfo_t
spawn_connection_common(socket_t const   stdin_sock,
                        socket_t const   stdout_sock,
                        HANDLE const     err_fd,
                        UU size_t const  argc,
                        char           **argv)
{
      AUTOC io_in     = socket_to_handle(stdin_sock);
      AUTOC io_out    = socket_to_handle(stdout_sock);
      AUTOC safe_argv = protect_argv(static_cast<int>(argc), argv);

      PROCESS_INFORMATION pid{};
      STARTUPINFOW        startinfo{};
      startinfo.cb         = sizeof startinfo;
      startinfo.dwFlags    = 0;
      startinfo.hStdInput  = io_in;
      startinfo.hStdOutput = io_out;
      startinfo.hStdError  = err_fd;

      if (!::SetHandleInformation(startinfo.hStdError, HANDLE_FLAG_INHERIT, 1))
            util::win32::error_exit(L"Stderr SetHandleInformation");
      if (!create_process_normal(safe_argv.c_str(), &startinfo, &pid))
            util::win32::error_exit(L"CreateProcessW()");

      return pid;
}


static int
start_process_with_pipe(UNUSED char const   *exe,
                        wchar_t const       *argv,
                        HANDLE               pipefds[2],
                        void const *const    err_handle,
                        PROCESS_INFORMATION *pi)
{
      HANDLE              handles[2][2] = {};
      STARTUPINFOW        start_info    = {};
      SECURITY_ATTRIBUTES sec_attr      = {
          .nLength              = sizeof(SECURITY_ATTRIBUTES),
          .lpSecurityDescriptor = nullptr,
          .bInheritHandle       = true,
      };

      memset(pi, 0, sizeof(PROCESS_INFORMATION));

      if (!::CreatePipe(&handles[INPUT_PIPE][READ_FD], &handles[INPUT_PIPE][WRITE_FD], &sec_attr, 0))
            util::win32::error_exit(L"Stdin CreatePipe()");
      if (!::SetHandleInformation(handles[INPUT_PIPE][WRITE_FD], HANDLE_FLAG_INHERIT, 0))
            util::win32::error_exit(L"Stdin SetHandleInformation() (write end)");

      if (!::CreatePipe(&handles[OUTPUT_PIPE][READ_FD], &handles[OUTPUT_PIPE][WRITE_FD], &sec_attr, 0))
            util::win32::error_exit(L"Stdout CreatePipe()");
      if (!::SetHandleInformation(handles[OUTPUT_PIPE][READ_FD], HANDLE_FLAG_INHERIT, 0))
            util::win32::error_exit(L"Stdout SetHandleInformation() (read end)");

      start_info.cb         = sizeof start_info;
      start_info.dwFlags    = STARTF_USESTDHANDLES;
      start_info.hStdInput  = handles[INPUT_PIPE][READ_FD];
      start_info.hStdOutput = handles[OUTPUT_PIPE][WRITE_FD];
      start_info.hStdError  = const_cast<HANDLE>(err_handle ? err_handle
                                                            : ::GetStdHandle(STD_ERROR_HANDLE));

      if (!::SetHandleInformation(start_info.hStdError, HANDLE_FLAG_INHERIT, 1))
            util::win32::error_exit(L"Stderr SetHandleInformation");

      pipefds[WRITE_FD] = handles[INPUT_PIPE][WRITE_FD];
      pipefds[READ_FD]  = handles[OUTPUT_PIPE][READ_FD];

      if (!create_process_normal(argv, &start_info, pi))
            util::win32::error_exit(L"CreateProcess() failed");

      ::CloseHandle(handles[INPUT_PIPE][READ_FD]);
      ::CloseHandle(handles[OUTPUT_PIPE][WRITE_FD]);

      return 0;
}


} // namespace win32


using win32::spawn_connection_common;


/****************************************************************************************/
/* Pipe Connection */


void pipe_connection_impl::close() noexcept
{
      if (read_ > 2)
            ::close(read_);
      if (write_ > 2)
            ::close(write_);
      read_  = (-1);
      write_ = (-1);
}

procinfo_t
pipe_connection_impl::do_spawn_connection(size_t const argc, char **argv)
{
      throw_if_initialized(typeid(*this));

      HANDLE pipefds[2];
      procinfo_t pid{};

      AUTOC err_handle = get_err_handle();
      AUTOC cmdline    = win32::protect_argv(int(argc), argv);

      win32::start_process_with_pipe(argv[0], cmdline.c_str(),
                                     pipefds, err_handle, &pid);

      if (err_sink_type_ != sink_type::DEFAULT)
            ::CloseHandle(err_handle);

      read_  = ::_open_osfhandle(intptr_t(pipefds[0]), _O_RDONLY | _O_BINARY);
      write_ = ::_open_osfhandle(intptr_t(pipefds[1]), _O_WRONLY | _O_BINARY);

      set_initialized();
      return pid;
}


ssize_t
pipe_connection_impl::writev(iovec *vec, size_t const nbufs, intc flags)
{
      auto    lock  = std::lock_guard{write_mtx_};
      ssize_t total = 0;

      for (size_t i = 0; i < nbufs; ++i)
            total += write(vec[i].buf, vec[i].len, flags);

      return total;
}


fd_connection_impl::fd_connection_impl(intc readfd, intc writefd)
{
      set_descriptors(readfd, writefd);
      set_initialized();
}


fd_connection_impl::fd_connection_impl(HANDLE readfd, HANDLE writefd)
{
      set_descriptors(::_open_osfhandle(reinterpret_cast<intptr_t>(readfd),
                                        _O_RDONLY | _O_BINARY),
                      ::_open_osfhandle(reinterpret_cast<intptr_t>(writefd),
                                        _O_WRONLY | _O_BINARY));
      set_initialized();
}


void
fd_connection_impl::set_descriptors(intc readfd, intc writefd)
{
      read_  = readfd;
      write_ = writefd;
}


/****************************************************************************************/
/* HANDLE based pipe connection */


void
pipe_handle_connection_impl::close() noexcept
{
      if (read_ != INVALID_HANDLE_VALUE && read_ != ::GetStdHandle(STD_INPUT_HANDLE))
            ::CloseHandle(read_);
      read_ = INVALID_HANDLE_VALUE;
      if (write_ != INVALID_HANDLE_VALUE && write_ != ::GetStdHandle(STD_OUTPUT_HANDLE))
            ::CloseHandle(write_);
      write_ = INVALID_HANDLE_VALUE;
}

procinfo_t
pipe_handle_connection_impl::do_spawn_connection(size_t const argc, char **argv)
{
      throw_if_initialized(typeid(*this));

      HANDLE pipefds[2];
      procinfo_t pid{};

      auto const err_handle = get_err_handle();
      auto const cmdline    = win32::protect_argv(int(argc), argv);

      win32::start_process_with_pipe(argv[0], cmdline.c_str(),
                                     pipefds, err_handle, &pid);

      if (err_sink_type_ != sink_type::DEFAULT)
            ::CloseHandle(err_handle);

      read_  = pipefds[0];
      write_ = pipefds[1];

      set_initialized();
      return pid;
}

ssize_t
pipe_handle_connection_impl::read(void *buf, ssize_t const nbytes, intc flags)
{
      auto    lock  = std::lock_guard{read_mtx_};
      ssize_t total = 0;
      DWORD   n;

      if (flags & MSG_WAITALL) {
            do {
                  if (read_ == INVALID_HANDLE_VALUE) [[unlikely]]
                        throw except::connection_closed();

                  bool b = ::ReadFile(read_, static_cast<char *>(buf) + total,
                                      DWORD(nbytes - total), &n, nullptr);
                  if (!b)
                        util::win32::error_exit(L"ReadFile()");
            } while (ssize_t(n) != ssize_t(-1LL) && (total += n) < nbytes);
      } else {
            if (read_ == INVALID_HANDLE_VALUE) [[unlikely]]
                  throw except::connection_closed();
            bool b = ::ReadFile(read_, buf, DWORD(nbytes), &n, nullptr);
            if (!b)
                  util::win32::error_exit(L"ReadFile()");
            total = n;
      }

      return total;
}

ssize_t
pipe_handle_connection_impl::write(void const   *buf,
                                   ssize_t const nbytes,
                                   UNUSED intc   flags)
{
      auto    lock  = std::lock_guard{write_mtx_};
      ssize_t total = 0;
      DWORD n;

      do {
            if (write_ == INVALID_HANDLE_VALUE) [[unlikely]]
                  throw except::connection_closed();
            bool b = ::WriteFile(write_, static_cast<char const *>(buf) + total,
                                 DWORD(nbytes - total), &n, nullptr);
            if (!b)
                  util::win32::error_exit(L"ReadFile()");
      } while (ssize_t(n) != ssize_t(-1LL) && (total += n) < nbytes);

      return total;
}


ssize_t
pipe_handle_connection_impl::writev(iovec *vec, size_t const nbufs, intc flags)
{
      auto    lock  = std::lock_guard{write_mtx_};
      ssize_t total = 0;

      for (size_t i = 0; i < nbufs; ++i)
            total += write(vec[i].buf, vec[i].len, flags);

      return total;
}


/*======================================================================================*/
/*======================================================================================*/
#else // !defined _Win32
/*======================================================================================*/
/*======================================================================================*/


template <typename ...Types>
NORETURN static void
patricide(pid_t const parent, Types ...args)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat"
      warnx(args...);
#pragma GCC diagnostic pop

      ::fflush(stderr);
      ::kill(parent, SIGABRT);
      err_nothrow("aborting");
}


static procinfo_t
spawn_connection_common(socket_t const      stdout_sock,
                        socket_t const      stdin_sock,
                        intc           err_fd,
                        UNUSED size_t const argc,
                        char              **argv)
{
      pid_t const this_id = ::getpid();
      procinfo_t  pid;

      if ((pid = ::fork()) == 0) {
            intc io_in  = stdin_sock >= 0
                             ? stdin_sock
                             : ::open("/dev/null", O_RDWR | O_BINARY | O_APPEND, 0600);
            intc io_out = stdout_sock >= 0
                             ? stdout_sock
                             : ::open("/dev/null", O_RDWR | O_BINARY | O_APPEND, 0600);

            ::dup2(io_in, STDIN_FILENO);
            ::dup2(io_out, STDOUT_FILENO);
            ::close(io_in);
            ::close(io_out);
            assert(argv[0] != nullptr);

            if (err_fd > 2) {
                  ::dup2(err_fd, STDERR_FILENO);
                  ::close(err_fd);
            }

            errno = 0;
            ::execvp(argv[0], const_cast<char **>(argv));
            patricide(this_id, "execvp() of '%s' failed: %d", argv[0], errno);
      }

      return pid;
}


/****************************************************************************************/
/* Socketpair Connection */


# ifdef HAVE_SOCKETPAIR

void
socketpair_connection_impl::open()
{
      throw_if_initialized(typeid(*this), 1);
      open_new_socket();
      set_initialized(1);
}

procinfo_t
socketpair_connection_impl::do_spawn_connection(UNUSED size_t const argc, char **argv)
{
      throw_if_initialized(typeid(*this), 2);
      if (!check_initialized(1))
            open();

      err_fd_   = get_err_handle();
      AUTOC pid = spawn_connection_common(con_read_, con_write_, err_fd_, argc, argv);
      util::close_descriptor(con_read_);
      util::close_descriptor(con_write_);

      set_initialized(2);
      return pid;
}

socket_t
socketpair_connection_impl::open_new_socket()
{
      int fds[2];
      if (socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, fds) == (-1))
            err("socketpair()");
      acc_read_  = fds[0];
      con_read_ = fds[1];

      if (use_dual_sockets()) {
            if (socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, fds) == (-1))
                  err("socketpair()");
            acc_write_ = fds[0];
            con_write_ = fds[1];
      } else {
            acc_write_ = acc_read_;
            con_write_ = con_read_;
      }

      return fds[0];
}

socket_t
socketpair_connection_impl::connect_internally()
{
      return con_read_; /* ugh */
}

# endif


/****************************************************************************************/
/* Pipe Connection */


void
pipe_connection_impl::close() noexcept
{
      if (read_ > 2)
            ::close(read_);
      if (write_ > 2)
            ::close(write_);
      read_  = (-1);
      write_ = (-1);
}

static void
open_pipe(int fds[2])
{
      if (::pipe2(fds, O_CLOEXEC) == (-1))
            err("pipe2()");

# ifdef F_SETPIPE_SZ
      if (::fcntl(fds[0], F_SETPIPE_SZ, 65536) == (-1))
            err("fcntl(F_SETPIPE_SZ)");
# endif
}

procinfo_t
pipe_connection_impl::do_spawn_connection(UNUSED size_t argc, char **argv)
{
      throw_if_initialized(typeid(*this), 2);

      int         fds[2][2];
      procinfo_t  pid;
      pid_t const this_id = ::getpid();
      err_fd_             = get_err_handle();

      open_pipe(fds[0]);
      open_pipe(fds[1]);

      assert(fds[0][0] >= 0);
      assert(fds[0][1] >= 0);
      assert(fds[1][0] >= 0);
      assert(fds[1][1] >= 0);

      if ((pid = fork()) == 0) {
            if (::dup2(fds[INPUT_PIPE][READ_FD], STDIN_FILENO) == (-1))
                  patricide(this_id, "dup2() failed");
            if (::dup2(fds[OUTPUT_PIPE][WRITE_FD], STDOUT_FILENO) == (-1))
                  patricide(this_id, "dup2() failed");
            if (err_fd_ > 2) {
                  if (::dup2(err_fd_, STDERR_FILENO) == (-1))
                        patricide(this_id, "dup2() failed");
                  ::close(err_fd_);
            }

            ::close(fds[0][0]); ::close(fds[0][1]);
            ::close(fds[1][0]); ::close(fds[1][1]);

            if (::execvp(argv[0], argv) == (-1))
                  patricide(this_id, "execvp() of '%s' failed", argv[0]);
      }

      util::close_descriptor(fds[INPUT_PIPE][READ_FD]);
      util::close_descriptor(fds[OUTPUT_PIPE][WRITE_FD]);
      if (err_fd_ > 2)
            util::close_descriptor(err_fd_);

      write_ = fds[INPUT_PIPE][WRITE_FD];
      read_  = fds[OUTPUT_PIPE][READ_FD];

      set_initialized(2);
      return pid;
}


ssize_t
pipe_connection_impl::writev(iovec *vec, size_t const nbufs, UU intc flags)
{
      auto    lock   = std::lock_guard{write_mtx_};
      ssize_t nbytes = 0;
      ssize_t total  = 0;
      ssize_t n;

      for (size_t i = 0; i < nbufs; ++i)
            total += static_cast<ssize_t>(vec[i].iov_len);

      do {
            if (write_ == (-1)) [[unlikely]]
                  throw except::connection_closed();
            n = ::writev(write_, vec, static_cast<int>(nbufs));
      } while (n != SSIZE_C(-1) && (total += n) < nbytes);

      return total;
}


/*======================================================================================*/
/*======================================================================================*/
#endif // defined _WIN32
/*======================================================================================*/
/*======================================================================================*/


/* Not system specific. */


ssize_t
pipe_connection_impl::read(void *buf, ssize_t const nbytes, intc flags)
{
      auto    lock  = std::lock_guard{read_mtx_};
      ssize_t total = 0;

      if (flags & MSG_WAITALL) {
            ssize_t n;
            do {
                  if (read_ == (-1)) [[unlikely]]
                        throw except::connection_closed();
                  n = ::read(read_, static_cast<char *>(buf) + total,
                             static_cast<rw_param_t>(nbytes - total));
            } while (n != SSIZE_C(-1) && (total += n) < nbytes);
      } else {
            if (read_ == (-1)) [[unlikely]]
                  throw except::connection_closed();
            total = ::read(read_, buf, static_cast<rw_param_t>(nbytes));
      }

      return total;
}

ssize_t
pipe_connection_impl::write(void const *buf, ssize_t const nbytes, UNUSED intc flags)
{
      auto    lock  = std::lock_guard{write_mtx_};
      ssize_t total = 0;
      ssize_t n;

      do {
            if (write_ == (-1)) [[unlikely]]
                  throw except::connection_closed();
            n = ::write(write_, static_cast<char const *>(buf) + total,
                        static_cast<rw_param_t>(nbytes - total));
      } while (n != SSIZE_C(-1) && (total += n) < nbytes);

      return total;
}


size_t
pipe_connection_impl::available() const noexcept(false)
{
#ifdef _WIN32
      auto *hand = reinterpret_cast<HANDLE>(_get_osfhandle(read_));
      return util::available_in_fd(hand);
#else
      return util::available_in_fd(read_);
#endif
}


/****************************************************************************************/
/* Unix Socket Connection */


void
unix_socket_connection_impl::open()
{
      throw_if_initialized(typeid(*this), 1);
      if (path_.empty())
            path_ = util::get_temporary_filename(nullptr, ".sock").string();
      if (listener_ == invalid_socket)
            listener_ = open_new_socket();

      if (should_connect())
            connect_internally();

      set_initialized(1);
}

procinfo_t
unix_socket_connection_impl::do_spawn_connection(size_t const argc, char **argv)
{
      throw_if_initialized(typeid(*this), 2);
      if (!check_initialized(1))
            open();

      err_fd_ = get_err_handle();

#ifdef _WIN32
      procinfo_t pid;
      if (listener_ == invalid_socket)
            pid = spawn_connection_common(invalid_socket, invalid_socket,
                                          err_fd_, argc, argv);
      else
            pid = win32::spawn_connection_common_detours(err_fd_, argc, argv, L"unix"s,
                                                         util::recode<wchar_t>(path_),
                                                         use_dual_sockets());
#else
      procinfo_t const pid = spawn_connection_common(con_read_, con_write_, err_fd_,
                                                     argc, argv);
#endif

      if (err_sink_type_ == sink_type::FILENAME)
            util::close_descriptor(err_fd_);

      set_initialized(2);
      return pid;
}

socket_t
unix_socket_connection_impl::open_new_socket()
{
      util::socket::init_sockaddr_un(addr_, path_);

      auto listener = invalid_socket;

      if (should_open_listener()) {
            listener = util::socket::call_socket_listener(AF_UNIX);

//#ifdef _WIN32
//            {
//                  // ReSharper disable once CppJoinDeclarationAndAssignment
//                  int   ret;
//                  DWORD bytes_returned = 0;
//                  auto const apath  = std::filesystem::path{R"(\\?\)" + path_};
//                  auto const parent = apath.parent_path().string();
//                  auto const base   = apath.filename().string();
//
//                  ret = WSAIoctl(listener, SIO_AF_UNIX_SETBINDPARENTPATH,
//                                 (LPVOID)parent.c_str(), parent.size(), nullptr, 0,
//                                 &bytes_returned, nullptr, nullptr);
//                  if (ret)
//                        util::win32::error_exit_wsa(
//                            L"WSAIoctl() (" + std::to_wstring(bytes_returned) + L")");
//
//                  //bytes_returned = 0;
//                  //ret = WSAIoctl(listener, SIO_AF_UNIX_SETCONNPARENTPATH,
//                  //               (LPVOID)parent.c_str(), parent.size(), nullptr, 0,
//                  //               &bytes_returned, nullptr, nullptr);
//                  //if (ret)
//                  //      util::win32::error_exit_wsa(
//                  //          L"WSAIoctl() (" + std::to_wstring(bytes_returned) + L")");
//
//                  if (base.length() + 1 > sizeof(addr_.sun_path))
//                        throw std::logic_error(fmt::format(
//                            FC("The file path \"{}\" (len: {}) is too large to "
//                               "fit into a UNIX socket address (size: {})"),
//                            base, base.size() + 1, sizeof(addr_.sun_path)));
//
//                  memcpy(addr_.sun_path, base.data(), base.size() + SIZE_C(1));
//            }
//#endif

            if (listener == invalid_socket)
                  err("socket");
            if (::bind(listener, (sockaddr *)&addr_, sizeof addr_) == SOCKET_ERROR)
                  err("bind");
            if (::listen(listener, 2) == SOCKET_ERROR)
                  err("listen");
      }

      return listener;
}

socket_t
unix_socket_connection_impl::connect_internally()
{
      util::socket::unix_socket_connect_pair(listener_, acc_read_, con_read_, path_.c_str());
      if (use_dual_sockets()) {
            util::socket::unix_socket_connect_pair(listener_, acc_write_, con_write_, path_.c_str());
      } else {
            con_write_ = con_read_;
            acc_write_ = acc_read_;
      }

      return acc_read_;
}

void
unix_socket_connection_impl::set_address(std::string path)
{
      throw_if_initialized(typeid(*this));
      path_ = std::move(path);
}

socket_t
unix_socket_connection_impl::connect()
{
      if (!check_initialized(1))
            throw std::logic_error("Cannot connect to uninitialized address!");

      socket_t const connector = util::socket::call_socket_connect(AF_UNIX);

      if (connector == invalid_socket)
            ERR("socket");
      if (::connect(connector, (sockaddr const *)&addr_, sizeof addr_) == SOCKET_ERROR)
            ERR("connect");

      acc_read_  = connector;
      acc_write_ = connector;
      return con_read_;
}

socket_t
unix_socket_connection_impl::accept()
{
      if (!check_initialized(1))
            throw std::logic_error("Cannot connect to uninitialized address!");

      sockaddr_un con_addr = {};
      socklen_t   len      = sizeof con_addr;
      acc_read_ = ::accept(listener_, (sockaddr *)&con_addr, &len);
      if (acc_read_ == invalid_socket)
            ERR("accept()");

#ifdef _WIN32
      u_long blocking_io = 1;
      //constexpr DWORD buffer_size = 32;
      // if (setsockopt(acc_read_, SOL_SOCKET, SO_RCVBUF, reinterpret_cast<char const *>(&buffer_size), sizeof(buffer_size)))
      //       util::win32::error_exit_wsa(L"setsockopt for receive buffer");
      // if (setsockopt(acc_read_, SOL_SOCKET, SO_SNDBUF, reinterpret_cast<char const *>(&buffer_size), sizeof(buffer_size)))
      //       util::win32::error_exit_wsa(L"setsockopt for send buffer");
      if (::ioctlsocket(acc_read_, FIONBIO, &blocking_io))
            util::win32::error_exit_wsa(L"ioctl for blocking io");
#else
      // unsigned blocking_io = 1;
      // ioctl(acc_read_, FIONBIO, &blocking_io);
#endif

      if (use_dual_sockets()) {
            acc_write_ = ::accept(listener_, (sockaddr *)&con_addr, &len);
            if (acc_write_ == invalid_socket)
                  ERR("accept()");
#ifdef _WIN32
            if (::ioctlsocket(acc_write_, FIONBIO, &blocking_io))
                  util::win32::error_exit_wsa(L"ioctl for blocking io");
#endif
      } else {
            acc_write_ = acc_read_;
      }

      return acc_read_;
}


/*======================================================================================*/
/* Inet Connection */


/*--------------------------------------------------------------------------------------*/
/* Either */


inet_any_socket_connection_impl::inet_any_socket_connection_impl() noexcept
{
      addr_ = nullptr;
}

inet_any_socket_connection_impl::~inet_any_socket_connection_impl()
{
      ::free(this->addr_);
}

void
inet_any_socket_connection_impl::open()
{
      throw_if_initialized(typeid(*this), 1);
      open_new_socket();

      if (should_connect()) {
            connect_internally();
            socklen_t size = addr_length_;
            acc_read_      = ::accept(listener_, addr_, &size);
            if (acc_read_ == invalid_socket || size != addr_length_)
                  ERR("accept");
            if (use_dual_sockets()) {
                  acc_write_ = ::accept(listener_, addr_, &size);
                  if (acc_write_ == invalid_socket || size != addr_length_)
                        ERR("accept");
            } else {
                  acc_write_ = acc_read_;
            }
      }

      set_initialized(1);
}

procinfo_t
inet_any_socket_connection_impl::do_spawn_connection(size_t const argc, char **argv)
{
      throw_if_initialized(typeid(*this), 2);
      if (!check_initialized(1))
            open();

      procinfo_t ret;
      err_fd_ = get_err_handle();

#ifdef _WIN32
      ret = listener_ == invalid_socket
                ? spawn_connection_common(invalid_socket, invalid_socket, err_fd_,
                                          argc, argv)
                : win32::spawn_connection_common_detours(
                      err_fd_, argc, argv,
                        family_ == AF_INET  ? L"ipv4"s
                      : family_ == AF_INET6 ? L"ipv6"s
                                          : L""s,
                      util::recode<wchar_t>(addr_string_), use_dual_sockets(), hport_
                );
#else
      ret = spawn_connection_common(con_read_, con_write_, err_fd_, argc, argv);
#endif

      set_initialized(2);
      return ret;
}

void
inet_any_socket_connection_impl::init_strings()
{
      if (addr_string_.empty()) {
            char buf[128];
            if (family_ == AF_INET) {
                  auto const *a = reinterpret_cast<sockaddr_in *>(addr_);
                  ::inet_ntop(AF_INET, &a->sin_addr, buf, sizeof buf);
                  addr_string_ = buf;
            } else if (family_ == AF_INET6) {
                  auto const *a = reinterpret_cast<sockaddr_in6 *>(addr_);
                  ::inet_ntop(AF_INET6, &a->sin6_addr, buf, sizeof buf);
                  addr_string_ = buf;
            } else {
                  errx("Invalid family %d", family_);
            }
      }

      if (family_ == AF_INET) {
            hport_ = ntohs(reinterpret_cast<sockaddr_in *>(addr_)->sin_port);
            addr_string_with_port_ = fmt::format(FC("{}:{}"), addr_string_, hport_);
      } else if (family_ == AF_INET6) {
            hport_ = ntohs(reinterpret_cast<sockaddr_in6 *>(addr_)->sin6_port);
            addr_string_with_port_ = fmt::format(FC("[{}]:{}"), addr_string_, hport_);
      } else {
            errx("Invalid family %d", family_);
      }
}

socket_t
inet_any_socket_connection_impl::open_new_socket()
{
      if (!addr_init_) {
            addr_length_ = sizeof(sockaddr_in);
            family_        = AF_INET;
            auto *addr   = ::util::xcalloc<sockaddr_in>();

            addr->sin_addr.s_addr = htonl(INADDR_LOOPBACK);  // 127.0.0.1
            addr->sin_family = static_cast<uint16_t>(family_);
            addr->sin_port   = 0;
            addr_            = reinterpret_cast<sockaddr *>(addr);
            addr_init_       = true;

            char buf[32];
            ::inet_ntop(AF_INET, &addr->sin_addr, buf, sizeof buf);
            addr_string_ = buf;
      }

      if (should_open_listener()) {
            listener_ = util::socket::open_new_inet_socket(addr_, addr_length_, family_);
            if (!addr_init_) {
                  memset(addr_, 0, sizeof(sockaddr_in));
                  util::socket::lazy_getsockname(listener_, addr_, sizeof(sockaddr_in));
                  hport_     = ntohs(((sockaddr_in *)addr_)->sin_port);
                  addr_init_ = true;
                  addr_string_with_port_ = fmt::format(FC("{}:{}"), addr_string_, hport_);
            } else {
                  init_strings();
            }
      }

      return listener_;
}

socket_t
inet_any_socket_connection_impl::connect_internally()
{
      if (listener_ != invalid_socket) {
            if (!addr_init_)
                  addr_init_ = true;
            /* else { nothing to do, listener is open and addr_ is set up. } */
      } else if (should_open_listener()) {
            listener_ = open_new_socket();
      }

      memset(addr_, 0, addr_length_);
      util::socket::lazy_getsockname(listener_, addr_, addr_length_);

      init_strings();

      if (should_connect()) {
            con_read_ = util::socket::connect_to_inet_socket(addr_, addr_length_, family_);
            if (use_dual_sockets())
                  con_write_ = util::socket::connect_to_inet_socket(addr_, addr_length_, family_);
            else
                  con_write_ = con_read_;
      }

      return con_read_;
}


void inet_any_socket_connection_impl::assign_from_addrinfo(addrinfo const *info)
{
      if (addr_)
            util::free_all(addr_);
      family_        = info->ai_family;
      addr_length_ = static_cast<socklen_t>(info->ai_addrlen);
      addr_        = util::xcalloc<sockaddr>(1, addr_length_);
      memcpy(addr_, info->ai_addr, info->ai_addrlen);
      addr_init_ = true;
}


int inet_any_socket_connection_impl::resolve(char const *server_name, char const *port)
{
      auto *info = ::util::socket::resolve_addr(server_name, port, SOCK_STREAM);
      if (!info)
            return (-1);

      assign_from_addrinfo(info);
      ::freeaddrinfo(info);
      init_strings();
      set_initialized(1);
      return 0;
}


int inet_any_socket_connection_impl::resolve(char const *const server_and_port)
{
#define BAD_ADDRESS(ipver)      \
      throw std::runtime_error( \
          fmt::format(FC("Invalid IPv" ipver " address \"{}\" - @(line {})"), \
                      server_and_port, __LINE__))

      char buf[48];
      char const *server;
      char const *port;

      if (!server_and_port || !*server_and_port)
            BAD_ADDRESS("?");

      if (server_and_port[0] == '[') {
            /* IPv6 address, in brackets, hopefully with port.
             * e.g. [::1]:12345 */
            if (!*(server = server_and_port + 1))
                  BAD_ADDRESS("6");
            port = strchr(server, ']');
            if (!port)
                  BAD_ADDRESS("6");
            size_t const size = port - server;
            if (size > SIZE_C(39) || port[1] != ':' || !isdigit(port[2]))
                  BAD_ADDRESS("6");

            memcpy(buf, server, size);
            buf[size] = '\0';
            server    = buf;
            port     += 2;
      } else {
            /*
             * Assume it's either an ipv4 address with port, or the hideous form of
             * an ipv6 address with a port.
             * e.g. 127.0.0.1:12345
             * e.g. ::1:12345
             */
            port = strrchr(server_and_port, ':');
            if (!port) {
                  /* Sigh. God knows what it was meant to be at this point. */
                  if (strchr(server_and_port, '.'))
                        BAD_ADDRESS("4");
                  else
                        BAD_ADDRESS("?");
            }
            size_t const size = port - server_and_port;
            memcpy(buf, server_and_port, size);
            buf[size] = '\0';
            server    = buf;

            ++port;
      }

      return resolve(server, port);

#undef BAD_ADDRESS
}


/*--------------------------------------------------------------------------------------*/
/* IPv4 only */


void
inet_ipv4_socket_connection_impl::open()
{
      throw_if_initialized(typeid(*this), 1);
      connect_internally();

      if (should_connect()) {
            socklen_t size = sizeof addr_;
            acc_read_ = ::accept(listener_, reinterpret_cast<sockaddr *>(&addr_), &size);
            if (acc_read_ == invalid_socket || size != sizeof addr_)
                  ERR("accept");
            if (use_dual_sockets()) {
                  acc_write_ = ::accept(listener_, reinterpret_cast<sockaddr *>(&addr_), &size);
                  if (acc_write_ == invalid_socket || size != sizeof addr_)
                        ERR("accept");
            } else {
                  acc_write_ = acc_read_;
            }
      }

      set_initialized(1);
}

procinfo_t
inet_ipv4_socket_connection_impl::do_spawn_connection(size_t const argc, char **argv)
{
      throw_if_initialized(typeid(*this), 2);
      if (!check_initialized(1))
            open();

      err_fd_ = get_err_handle();
      procinfo_t ret;

#ifdef _WIN32
      if (listener_ == invalid_socket)
            ret = spawn_connection_common(invalid_socket, invalid_socket, err_fd_, argc, argv);
      else
            ret = win32::spawn_connection_common_detours(
                err_fd_, argc, argv, L"ipv4"s,
                util::recode<wchar_t>(addr_string_),
                use_dual_sockets(), hport_
            );
#else
      ret = spawn_connection_common(con_read_, con_write_, err_fd_, argc, argv);
#endif

      set_initialized(2);
      return ret;
}

socket_t
inet_ipv4_socket_connection_impl::open_new_socket()
{
      if (!addr_init_) {
            memset(&addr_, 0, sizeof addr_);
            addr_.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // 127.0.0.1
            addr_.sin_family      = AF_INET;
            addr_.sin_port        = 0;
            addr_string_          = "127.0.0.1"s;
      }

      listener_  = util::socket::open_new_inet_socket(reinterpret_cast<sockaddr *>(&addr_), sizeof addr_, AF_INET);
      memset(&addr_, 0, sizeof addr_);
      util::socket::lazy_getsockname(listener_, reinterpret_cast<sockaddr *>(&addr_), sizeof addr_);
      addr_init_ = true;

      if (addr_string_.empty()) {
            char buf[128];
            addr_string_ = ::inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof buf);
      }
      hport_ = ntohs(addr_.sin_port);
      addr_string_with_port_ = fmt::format(FC("{}:{}"), addr_string_, hport_);

      return listener_;
}

socket_t
inet_ipv4_socket_connection_impl::connect_internally()
{
      if (listener_ != invalid_socket) {
            if (!addr_init_) {
                  util::socket::lazy_getsockname(listener_, reinterpret_cast<sockaddr *>(&addr_), sizeof addr_);
                  addr_init_ = true;
            }
            /* else { nothing to do, listener is open and addr_ is set up. } */
      } else if (should_open_listener()) {
            listener_ = open_new_socket();
      }

      if (should_connect()) {
            con_read_ = util::socket::connect_to_inet_socket(
                reinterpret_cast<sockaddr *>(&addr_), sizeof addr_, AF_INET);
            con_write_ = use_dual_sockets() ? util::socket::connect_to_inet_socket(
                                                  reinterpret_cast<sockaddr *>(&addr_),
                                                  sizeof addr_, AF_INET)
                                            : con_read_;
      }

      return con_read_;
}


/*--------------------------------------------------------------------------------------*/
/* IPv6 only */


void
inet_ipv6_socket_connection_impl::open()
{
      throw_if_initialized(typeid(*this), 1);
      connect_internally();

      if (should_connect()) {
            socklen_t size = sizeof addr_;
            acc_read_ = ::accept(listener_, reinterpret_cast<sockaddr *>(&addr_), &size);
            if (acc_read_ == invalid_socket || size != sizeof addr_)
                  ERR("accept");
            if (use_dual_sockets()) {
                  acc_write_ = ::accept(listener_, reinterpret_cast<sockaddr *>(&addr_), &size);
                  if (acc_write_ == invalid_socket || size != sizeof addr_)
                        ERR("accept");
            } else {
                  acc_write_ = acc_read_;
            }
      }

      set_initialized(1);
}

procinfo_t
inet_ipv6_socket_connection_impl::do_spawn_connection(size_t const argc, char **argv)
{
      throw_if_initialized(typeid(*this), 2);
      if (!check_initialized(1))
            open();

      err_fd_ = get_err_handle();

      procinfo_t ret;
#ifdef _WIN32
      if (listener_ == invalid_socket)
            ret = spawn_connection_common(invalid_socket, invalid_socket, err_fd_,
                                          argc, argv);
      else
            ret = win32::spawn_connection_common_detours(
                err_fd_, argc, argv, L"ipv6"s, util::recode<wchar_t>(addr_string_),
                use_dual_sockets(), hport_);
#else
      ret = spawn_connection_common(con_read_, con_write_, err_fd_, argc, argv);
#endif

      set_initialized(2);
      return ret;
}

socket_t
inet_ipv6_socket_connection_impl::open_new_socket()
{
      if (!addr_init_) {
            memset(&addr_, 0, sizeof addr_);
            addr_.sin6_addr   = IN6ADDR_LOOPBACK_INIT;
            addr_.sin6_family = AF_INET6;
            addr_.sin6_port   = 0;
            addr_string_      = "[::1]"s;
      }

      listener_ = util::socket::open_new_inet_socket(reinterpret_cast<sockaddr *>(&addr_), sizeof addr_, AF_INET6);
      memset(&addr_, 0, sizeof addr_);
      util::socket::lazy_getsockname(listener_, reinterpret_cast<sockaddr *>(&addr_), sizeof addr_);

      addr_init_ = true;
      if (addr_string_.empty() || addr_string_.back() == ']') {
            char buf[128];
            addr_string_ = ::inet_ntop(AF_INET6, &addr_.sin6_addr, buf, sizeof buf);
      }
      hport_ = ntohs(addr_.sin6_port);
      addr_string_with_port_ = fmt::format(FC("{}:{}"), addr_string_, hport_);
      return listener_;
}

socket_t
inet_ipv6_socket_connection_impl::connect_internally()
{
      if (listener_ != invalid_socket) {
            if (!addr_init_) {
                  util::socket::lazy_getsockname(listener_, reinterpret_cast<sockaddr *>(&addr_), sizeof addr_);
                  addr_init_ = true;
            }
            /* else { nothing to do, listener is open and addr_ is set up. } */
      } else if (should_open_listener()) {
            listener_ = open_new_socket();
      }

      if (should_connect()) {
            con_read_ = util::socket::connect_to_inet_socket(
                reinterpret_cast<sockaddr *>(&addr_), sizeof addr_, AF_INET6);
            con_write_ = use_dual_sockets() ? util::socket::connect_to_inet_socket(
                                                  reinterpret_cast<sockaddr *>(&addr_),
                                                  sizeof addr_, AF_INET6)
                                            : con_read_;
      }

      return con_read_;
}


/****************************************************************************************/
/* Libuv Pipe connection */


void
libuv_pipe_handle_impl::close() noexcept
{
      ::uv_process_kill(&proc_handle_, SIGTERM);
}

void
libuv_pipe_handle_impl::open()
{
      throw_if_initialized(typeid(*this), 2);

      assert(loop_ != nullptr);
      ::uv_pipe_init(loop_, &read_handle_, 0);
      ::uv_pipe_init(loop_, &write_handle_, 0);

      set_initialized(2);
}

size_t
libuv_pipe_handle_impl::available() const noexcept(false)
{
      return util::available_in_fd(reinterpret_cast<HANDLE>(fd()));
}

void
libuv_pipe_handle_impl::set_loop(uv_loop_t *loop)
{
      throw_if_initialized(typeid(*this), 1);
      loop_ = loop;
      set_initialized(1);
}

procinfo_t
libuv_pipe_handle_impl::do_spawn_connection(UNUSED size_t argc, char **argv)
{
      throw_if_initialized(typeid(*this), 3);
      if (!check_initialized(2))
            open();

#ifdef _WIN32
      int    err_fd;
      HANDLE err_handle = get_err_handle();
      if (err_handle == ::GetStdHandle(STD_ERROR_HANDLE)) {
            err_fd = 2;
      } else {
            err_fd = ::_open_osfhandle(reinterpret_cast<intptr_t>(err_handle),
                                       _O_WRONLY | _O_BINARY);
            if (err_fd == -1)
                  util::win32::error_exit(L"_open_osfhandle()");
      }
      crt_err_fd_ = err_fd;
#else
      int err_fd = get_err_handle();
#endif

      constexpr int uvflags[2] = {
            UV_CREATE_PIPE | UV_READABLE_PIPE | UV_WRITABLE_PIPE | UV_OVERLAPPED_PIPE,
            UV_CREATE_PIPE | UV_READABLE_PIPE | UV_WRITABLE_PIPE | UV_OVERLAPPED_PIPE,
      };

      uv_stdio_container_t containers[] = {
            { .flags = static_cast<uv_stdio_flags>(uvflags[0]),
              .data  = {.stream = reinterpret_cast<uv_stream_t *>(&write_handle_)} },
            { .flags = static_cast<uv_stdio_flags>(uvflags[1]),
              .data  = {.stream = reinterpret_cast<uv_stream_t *>(&read_handle_)} },
            { .flags = static_cast<uv_stdio_flags>(UV_INHERIT_FD),
              .data  = {.fd = err_fd} },
      };
      uv_process_options_t const opts = {
          .exit_cb = nullptr,
          .file  = argv[0],
          .args  = argv,
          .env   = nullptr,
          .cwd   = nullptr,
          .flags = 0,
          .stdio_count = static_cast<int>(std::size(containers)),
          .stdio = containers,
          .uid   = 0,
          .gid   = 0,
      };

      if (int const e = ::uv_spawn(loop_, &proc_handle_, &opts); e < 0)
      {
#ifdef _WIN32
            auto const s = util::recode<wchar_t>(uv_strerror(e));
            util::win32::error_exit_message(L"Fatal libuv error: "s + s);
#else
            errx_nothrow("libuv error (%d): \"%s\"", e, uv_strerror(e));
#endif
      }

      set_initialized(3);

#ifdef _WIN32
      return {};
#else
      return proc_handle_.pid;
#endif
}

void
libuv_pipe_handle_impl::uvwrite_callback(uv_write_t *req, intc status)
{
      auto *self = static_cast<this_type *>(req->data);
      req->data  = reinterpret_cast<void *>(static_cast<uintptr_t>(status));
      self->write_cond_.notify_one();
}

ssize_t
libuv_pipe_handle_impl::write(void const *buf, ssize_t const nbytes, int /*flags*/)
{
      std::unique_lock write_lock{write_mtx_};
      uv_write_t req;
      uv_buf_t   ubuf;
      req.data = this;
      init_iovec(reinterpret_cast<iovec &>(ubuf),
                 static_cast<char *>(const_cast<void *>(buf)),
                 static_cast<decltype(ubuf.len)>(nbytes));

      intc ret = ::uv_write(&req, reinterpret_cast<uv_stream_t *>(&write_handle_),
                            &ubuf, 1U, uvwrite_callback);

      write_lock.unlock();
      std::mutex       mtx;
      std::unique_lock cb_lock{mtx};
      write_cond_.wait(cb_lock);

      intc status = static_cast<int>(reinterpret_cast<uintptr_t>(req.data));
      if (status != 0)
            throw std::runtime_error(::uv_strerror(status));
      return ret;
}

ssize_t
libuv_pipe_handle_impl::writev(iovec *vec, size_t const nbufs, int /*flags*/)
{
      std::unique_lock write_lock{write_mtx_};
      uv_write_t  req;
      auto const *ubufs = reinterpret_cast<uv_buf_t const *>(vec);
      req.data          = this;
      req.type          = UV_WRITE;

      intc ret = ::uv_write(&req, reinterpret_cast<uv_stream_t *>(&write_handle_),
                            ubufs, static_cast<unsigned>(nbufs), uvwrite_callback);

      write_lock.unlock();
      std::mutex       mtx;
      std::unique_lock cb_lock{mtx};
      write_cond_.wait(cb_lock);

      intc status = static_cast<int>(reinterpret_cast<uintptr_t>(req.data));
      if (status != 0)
            throw std::runtime_error(::uv_strerror(status));

      return ret;
}

libuv_pipe_handle_impl::descriptor_type
libuv_pipe_handle_impl::fd() const noexcept
{
#ifdef _WIN32
      return reinterpret_cast<descriptor_type>(read_handle_.handle);
#else
      return read_handle_.accepted_fd;
#endif
}


/****************************************************************************************/


namespace io_impl {

#ifdef _WIN32

static __inline WSAOVERLAPPED
create_wsa_overlapped()
{
      WSAOVERLAPPED ov = {};
      ov.hEvent        = ::WSACreateEvent();
      if (ov.hEvent == nullptr)
            util::win32::error_exit_wsa(L"WSACreateEvent()");
      return ov;
}

ssize_t
socket_recv_impl(socket_t const sock, void *buf, size_t const nbytes, int const flags)
{
      WSAOVERLAPPED ov    = create_wsa_overlapped();
      size_t        total = 0;
      DWORD         nread = 0;
      WSABUF        wbuf;

      goto first_iter;  // NOLINT(cppcoreguidelines-avoid-goto, hicpp-avoid-goto)

      do {
            ::WSAResetEvent(ov.hEvent);
      first_iter:
            wbuf = {static_cast<ULONG>(nbytes - total),
                    (static_cast<char *>(buf) + total)};

            DWORD flg = 0;
            DWORD rc  = ::WSARecv(sock, &wbuf, 1, &nread, &flg, &ov, nullptr);

            if (rc != 0) {
                  if (rc == DWSOCKET_ERROR && ::WSAGetLastError() != WSA_IO_PENDING)
                        util::win32::error_exit_wsa(L"WSARecv()");

                  rc = ::WSAWaitForMultipleEvents(1, &ov.hEvent, true, WSA_INFINITE, true);
                  if (rc == WSA_WAIT_FAILED)
                        util::win32::error_exit_wsa(L"WSAWaitForMultipleEvents()");

                  flg = 0;
                  rc  = ::WSAGetOverlappedResult(sock, &ov, &nread, false, &flg);
                  if (rc == 0) {
                        rc = ::WSAGetLastError();
                        if (rc == WSAECONNRESET)
                              break;
                        util::win32::error_exit_explicit(
                            L"WSARecv via WSAGetOverlappedResult", rc);
                  }
            }
            total += nread;
      } while (((flags & MSG_WAITALL) && total < nbytes) || total == 0);

      ::WSACloseEvent(ov.hEvent);
      return static_cast<ssize_t>(total);
}

ssize_t
socket_send_impl(socket_t const sock,
                 void const    *buf,
                 size_t const   nbytes,
                 int const      flags)
{
      WSAOVERLAPPED ov    = create_wsa_overlapped();
      size_t        total = 0;
      DWORD         nsent = 0;
      DWORD         flg   = 0;
      WSABUF        wbuf;

      goto first_iter;  // NOLINT(cppcoreguidelines-avoid-goto, hicpp-avoid-goto)

      do {
            ::WSAResetEvent(ov.hEvent);
      first_iter:
            wbuf = {static_cast<ULONG>(nbytes - total),
                    const_cast<char *>(static_cast<char const *>(buf) + total)};

            DWORD rc = ::WSASend(sock, &wbuf, 1, &nsent, flags, &ov, nullptr);

            if (rc != 0) {
                  if (rc == DWSOCKET_ERROR && ::WSAGetLastError() != WSA_IO_PENDING)
                        util::win32::error_exit_wsa(L"WSASend()");

                  rc = ::WSAWaitForMultipleEvents(1, &ov.hEvent, true, WSA_INFINITE, true);
                  if (rc == WSA_WAIT_FAILED)
                        util::win32::error_exit_wsa(L"WSAWaitForMultipleEvents()");

                  flg = 0;
                  rc  = ::WSAGetOverlappedResult(sock, &ov, &nsent, false, &flg);
                  if (rc == 0) {
                        rc = ::WSAGetLastError();
                        if (rc == WSAECONNRESET)
                              break;
                        util::win32::error_exit_explicit(
                            L"WSASend via WSAGetOverlappedResult", rc);
                  }
            }
      } while ((total += nsent) < nbytes);

      ::WSACloseEvent(ov.hEvent);
      return static_cast<ssize_t>(total);
}

ssize_t
socket_writev_impl(socket_t const sock,
                   iovec const   *bufs,
                   size_t const   nbufs,
                   int const    /*flags*/)
{
      WSAOVERLAPPED ov = create_wsa_overlapped();
      DWORD nsent = 0;
      DWORD flg   = 0;
      DWORD rc    = ::WSASend(sock, const_cast<LPWSABUF>(bufs),
                              static_cast<DWORD>(nbufs), &nsent, 0, &ov, nullptr);

      if (rc != 0) {
            if (rc == DWSOCKET_ERROR && ::WSAGetLastError() != WSA_IO_PENDING)
                  util::win32::error_exit_wsa(L"WSASend()");

            rc = ::WSAWaitForMultipleEvents(1, &ov.hEvent, true, WSA_INFINITE, true);
            if (rc == WSA_WAIT_FAILED)
                  util::win32::error_exit_wsa(L"WSAWaitForMultipleEvents()");

            rc = ::WSAGetOverlappedResult(sock, &ov, &nsent, false, &flg);
            if (rc == 0)
                  util::win32::error_exit_wsa(L"WSASend failed via WSAGetOverlappedResult");
      }

      ::WSACloseEvent(ov.hEvent);
      return static_cast<ssize_t>(nsent);
}

#else /*--------------------------------------------------------------------------------*/

ssize_t
socket_recv_impl(socket_t sock, void *buf, size_t nbytes, int flags)
{
      ssize_t n;
      ssize_t total = 0;

      if (flags & MSG_WAITALL) {
            // Put it in a loop just in case...
            do {
                  n = ::recv(sock, (char *)buf + total, (size_t)(nbytes - total), flags); //NOLINT(*-casting,*-cast)
            } while ((n != (-1)) && (total += n) < (ssize_t)nbytes);                      //NOLINT(*-casting)
      } else {
            for (;;) {
                  n = ::recv(sock, buf, (size_t)nbytes, flags); //NOLINT(*-casting,*-cast)
                  if (n != 0)
                        break;
                  std::this_thread::sleep_for(10ms);
            }
            total = n;
      }

      if (n == (-1)) [[unlikely]] {
            auto const e = errno;
            if (e == EBADF || e == ECONNRESET)
                  throw except::connection_closed();
            err("recv()");
      }

      return total;
}

ssize_t
socket_send_impl(socket_t sock, void const *buf, size_t nbytes, UU int flags)
{
      ssize_t n;
      ssize_t total = 0;

      do {
            // n = ::send(sock, (char const *)buf + total, (size_t)(nbytes - total), flags);
            n = ::write(sock, (char const *)buf + total, (size_t)(nbytes - total)); //NOLINT(*cast*)
      } while (n != (-1) && (total += n) < (ssize_t)nbytes);                        //NOLINT(*cast*)

      if (n == (-1)) [[unlikely]] {
            auto const e = errno;
            if (e == EBADF || e == ECONNRESET)
                  throw except::connection_closed();
            err("send()");
      }

      return total;
}

ssize_t
socket_writev_impl(socket_t const sock, iovec const *bufs, size_t const nbufs, UU int flags)
{
      ssize_t bytes_sent;
#if 0
      struct msghdr hdr = {
            .msg_iov    = const_cast<struct ::iovec *>(bufs),
            .msg_iovlen = nbufs,
      };
#endif

      do {
            bytes_sent = ::writev(sock, bufs, static_cast<int>(nbufs));
            // bytes_sent = sendmsg(sock, &hdr, flags);

            if (bytes_sent == (-1)) [[unlikely]] {
                  auto const e = errno;
                  if (e == EBADF || e == ECONNRESET)
                        throw except::connection_closed();
                  err("sendmsg()");
            }
      } while (bytes_sent == 0);

      return bytes_sent;
}

#endif

} // namespace io_impl


/****************************************************************************************/
} // namespace ipc::detail::base
} // namespace emlsp
