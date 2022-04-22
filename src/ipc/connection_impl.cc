// ReSharper disable CppTooWideScopeInitStatement
#include "Common.hh"
#include "ipc/connection_impl.hh"

#include "util/recode.hh"

#ifndef O_BINARY
# define O_BINARY 0
#endif
#ifndef SOCK_CLOEXEC
# define SOCK_CLOEXEC 0
#endif

#ifdef _WIN32
# define ERRNO WSAGetLastError()
# define ERR(n, t) util::win32::error_exit_wsa(L ## t)
#else
# define ERRNO errno
# define ERR(n, t) err(n, t)
#endif

#define READ_FD     0
#define WRITE_FD    1
#define INPUT_PIPE  0
#define OUTPUT_PIPE 1

#define AUTOC auto const

inline namespace emlsp {
namespace ipc::detail {
/****************************************************************************************/

namespace {

constexpr auto invalid_socket = static_cast<socket_t>(-1);

#ifdef _WIN32

using native_descriptor_t = HANDLE;

/*
 * This little piece of stupidity is actually pretty important. Sockets passed as
 * stdio handles must not be considered OVERLAPPED. The parent socket, however, can.
 */
__forceinline socket_t
call_socket_listener(int const family, int const type, int const protocol)
{
      return ::WSASocketW(family, type, protocol, nullptr, 0, WSA_FLAG_OVERLAPPED);
      //return ::socket(family, type, protocol);
}
__forceinline socket_t
call_socket_connect(int const family, int const type, int const protocol)
{
      //return ::WSASocketW(family, type, protocol, nullptr, 0, 0);
      return ::socket(family, type, protocol);
}

#else

using native_descriptor_t = int;

__forceinline socket_t
call_socket_listener(int const family, int const type, int const protocol)
{
      return ::socket(family, type, protocol);
}
__forceinline socket_t
call_socket_connect(int const family, int const type, int const protocol)
{
      return ::socket(family, type, protocol);
}
#endif


void
do_open_unix_socket_connection(socket_t const     listener_,
                               socket_t          &acceptor,
                               socket_t          &connector,
                               std::string const &path)
{
      int         result[2];
      int         mode     = 0;
      sockaddr_un con_addr = {};
      socklen_t   len      = sizeof(sockaddr_un);

      connector = util::ipc::connect_to_unix_socket(path.c_str());
      acceptor  = ::accept(listener_, reinterpret_cast<sockaddr *>(&con_addr), &len);

#ifdef _WIN32
      result[0] = ioctlsocket(connector, FIONBIO, (ULONG *)&mode);
      result[1] = ioctlsocket(acceptor, FIONBIO, (ULONG *)&mode);
#else
      constexpr int NO_ERROR = 0;
      result[0] = ioctl(connector, FIONBIO, &mode);
      result[1] = ioctl(acceptor,  FIONBIO, &mode);
#endif

      assert(result[0] == NO_ERROR);
      assert(result[1] == NO_ERROR);

      if (!acceptor || acceptor == invalid_socket)
            ERR(1, "accept()");
}


struct process_spawn_info {
      size_t const        argc;
      socket_t const      stdin_sock;
      socket_t const      stdout_sock;
      native_descriptor_t err_fd;
      char              **argv;
      std::thread        *thrd;
};


} // namespace


/*======================================================================================*/
/*======================================================================================*/
#ifdef _WIN32
/*======================================================================================*/
/*======================================================================================*/


namespace win32 {

static __forceinline bool
create_process(wchar_t const *args, STARTUPINFOW *start_info, PROCESS_INFORMATION *pi)
{
      return CreateProcessW(nullptr, const_cast<PWSTR>(args), nullptr, nullptr,
                            true, 0, nullptr, nullptr, start_info, pi);
}


template <typename T>
static std::wstring
protect_argv(int const argc, T **argv)
{
      std::wstring str;
      str.reserve(8192);

      for (int i = 0; i < argc; ++i) {
            T    *p           = argv[i];
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
spawn_connection_common(socket_t const      stdin_sock,
                        socket_t const      stdout_sock,
                        HANDLE const        err_fd,
                        UNUSED size_t const argc,
                        char              **argv)
{
      PROCESS_INFORMATION pid;
      SecureZeroMemory(&pid, sizeof pid);

      //HANDLE io_in = static_cast<intptr_t>(stdin_sock) >= 0
      //                ? reinterpret_cast<HANDLE>(stdin_sock)
      //                : CreateFileA("NUL", GENERIC_WRITE, 0, nullptr, OPEN_EXISTING,
      //                              FILE_ATTRIBUTE_NORMAL, nullptr);
      //HANDLE io_out = static_cast<intptr_t>(stdout_sock) >= 0
      //                ? reinterpret_cast<HANDLE>(stdout_sock)
      //                : CreateFileA("NUL", GENERIC_WRITE, 0, nullptr, OPEN_EXISTING,
      //                              FILE_ATTRIBUTE_NORMAL, nullptr);

      AUTOC        safe_argv = protect_argv(static_cast<int>(argc), argv);
      STARTUPINFOW startinfo = {};
      startinfo.cb           = sizeof(startinfo);
      startinfo.dwFlags      = STARTF_USESTDHANDLES;
      startinfo.hStdInput    = (HANDLE)stdin_sock;
      startinfo.hStdOutput   = (HANDLE)stdout_sock;
      startinfo.hStdError    = err_fd;

      //if (!::SetHandleInformation(io_in, HANDLE_FLAG_INHERIT, 1))
      //      util::win32::error_exit(L"IO SetHandleInformation");
      //if (io_in != io_out)
      //      if (!::SetHandleInformation(io_out, HANDLE_FLAG_INHERIT, 1))
      //            util::win32::error_exit(L"IO SetHandleInformation");
      if (!::SetHandleInformation(startinfo.hStdError, HANDLE_FLAG_INHERIT, 1))
            util::win32::error_exit(L"Stderr SetHandleInformation");

      if (!create_process(safe_argv.c_str(), &startinfo, &pid))
            util::win32::error_exit(L"CreateProcessW()");

      //if (io_in != reinterpret_cast<HANDLE>(stdin_sock))
      //      CloseHandle(io_in);
      //if (io_out != reinterpret_cast<HANDLE>(stdout_sock))
      //      CloseHandle(io_out);

      return pid;
}


static procinfo_t
spawn_connection_shim(std::wstring const &addr,
                      HANDLE const err_fd,
                      size_t const argc,
                      char        **argv)
{
      static constexpr wchar_t shim_proc[] = LR"(D:\ass\VisualStudio\Repos\CreateProcessShim\x64\Debug\CreateProcessShim.exe)";
      PROCESS_INFORMATION pid;
      SecureZeroMemory(&pid, sizeof pid);

      auto const   safe_argv = protect_argv(static_cast<int>(argc), argv);
      STARTUPINFOW startinfo = {};
      startinfo.cb           = sizeof(startinfo);
      startinfo.dwFlags      = STARTF_USESTDHANDLES;
      startinfo.hStdError    = err_fd;

      std::vector<wchar_t const *> full_argv;
      full_argv.push_back(shim_proc);
      full_argv.push_back(addr.c_str());
      full_argv.push_back(safe_argv.c_str());

      auto const true_argv = protect_argv(full_argv.size(), full_argv.data());

      if (!::SetHandleInformation(startinfo.hStdError, HANDLE_FLAG_INHERIT, 1))
            util::win32::error_exit(L"Stderr SetHandleInformation");

      //util::eprint(FC("Process argv:\n{}\n\n"), util::recode<char>(true_argv));

      if (!create_process(true_argv.c_str(), &startinfo, &pid))
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

      ZeroMemory(pi, sizeof(PROCESS_INFORMATION));

      if (!CreatePipe(&handles[INPUT_PIPE][READ_FD], &handles[INPUT_PIPE][WRITE_FD], &sec_attr, 0))
            util::win32::error_exit(L"Stdin CreatePipe()");
      if (!SetHandleInformation(handles[INPUT_PIPE][WRITE_FD], HANDLE_FLAG_INHERIT, 0))
            util::win32::error_exit(L"Stdin SetHandleInformation() (write end)");

      if (!CreatePipe(&handles[OUTPUT_PIPE][READ_FD], &handles[OUTPUT_PIPE][WRITE_FD], &sec_attr, 0))
            util::win32::error_exit(L"Stdout CreatePipe()");
      if (!SetHandleInformation(handles[OUTPUT_PIPE][READ_FD], HANDLE_FLAG_INHERIT, 0))
            util::win32::error_exit(L"Stdout SetHandleInformation() (read end)");

      start_info.cb         = sizeof start_info;
      start_info.dwFlags    = STARTF_USESTDHANDLES;
      start_info.hStdInput  = handles[INPUT_PIPE][READ_FD];
      start_info.hStdOutput = handles[OUTPUT_PIPE][WRITE_FD];
      start_info.hStdError  = const_cast<HANDLE>(err_handle ? err_handle
                                                            : GetStdHandle(STD_ERROR_HANDLE));

      if (!SetHandleInformation(start_info.hStdError, HANDLE_FLAG_INHERIT, 1))
            util::win32::error_exit(L"Stderr SetHandleInformation");

      pipefds[WRITE_FD] = handles[INPUT_PIPE][WRITE_FD];
      pipefds[READ_FD]  = handles[OUTPUT_PIPE][READ_FD];

      if (!create_process(argv, &start_info, pi))
            util::win32::error_exit(L"CreateProcess() failed");

      CloseHandle(handles[INPUT_PIPE][READ_FD]);
      CloseHandle(handles[OUTPUT_PIPE][WRITE_FD]);

      return 0;
}


} // namespace win32


using win32::spawn_connection_common;


/****************************************************************************************/
/* Pipe Connection */


void pipe_connection_impl::close()
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
            CloseHandle(err_handle);

      read_  = _open_osfhandle(intptr_t(pipefds[0]), _O_RDONLY | _O_BINARY);
      write_ = _open_osfhandle(intptr_t(pipefds[1]), _O_WRONLY | _O_BINARY);

      set_initialized();
      return pid;
}


/****************************************************************************************/
/* HANDLE based pipe connection */


void
pipe_handle_connection_impl::close()
{
      if (read_ != INVALID_HANDLE_VALUE && read_ != GetStdHandle(STD_INPUT_HANDLE))
            ::CloseHandle(read_);
      read_ = INVALID_HANDLE_VALUE;
      if (write_ != INVALID_HANDLE_VALUE && write_ != GetStdHandle(STD_OUTPUT_HANDLE))
            ::CloseHandle(write_);
      write_ = INVALID_HANDLE_VALUE;
}

procinfo_t
pipe_handle_connection_impl::do_spawn_connection(size_t const argc, char **argv)
{
      throw_if_initialized(typeid(*this));

      HANDLE pipefds[2];
      procinfo_t pid{};

      AUTOC err_handle = get_err_handle();
      AUTOC cmdline    = win32::protect_argv(int(argc), argv);

      win32::start_process_with_pipe(argv[0], cmdline.c_str(),
                                     pipefds, err_handle, &pid);

      if (err_sink_type_ != sink_type::DEFAULT)
            CloseHandle(err_handle);

      read_  = pipefds[0];
      write_ = pipefds[1];

      set_initialized();
      return pid;
}

ssize_t
pipe_handle_connection_impl::read(void *buf, ssize_t const nbytes, int const flags)
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
            } while (ssize_t(n) != SSIZE_C(-1) && (total += n) < nbytes);
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
pipe_handle_connection_impl::write(void const *buf, ssize_t const nbytes, UNUSED int const flags)
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
      } while (ssize_t(n) != SSIZE_C(-1) && (total += n) < nbytes);

      return total;
}


/****************************************************************************************/
/* WIN32 Named Pipe connection */


# ifdef WIN32_USE_PIPE_IMPL

static void
named_pipe_spawn_connection_helper(HANDLE const         err_handle,
                                   PROCESS_INFORMATION *pi,
                                   std::wstring const  &pipe_path,
                                   std::wstring const  &argv)
{
      SECURITY_ATTRIBUTES sec_attr = {
          .nLength              = sizeof(SECURITY_ATTRIBUTES),
          .lpSecurityDescriptor = nullptr,
          .bInheritHandle       = TRUE,
      };

      //if (!SetHandleInformation(pipe, HANDLE_FLAG_INHERIT, 0))
      //      util::win32::error_exit(L"Named pipe SetHandleInformation");

      HANDLE client_pipe = CreateFileW(pipe_path.c_str(), GENERIC_READ | GENERIC_WRITE, 0,
                                       &sec_attr, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, nullptr);

      if (client_pipe == INVALID_HANDLE_VALUE)
            util::win32::error_exit(L"CreateFileW");

      ZeroMemory(pi, sizeof(PROCESS_INFORMATION));
      STARTUPINFOW start_info = {};
      start_info.cb           = sizeof(STARTUPINFOW);
      start_info.dwFlags      = STARTF_USESTDHANDLES;
      start_info.hStdInput    = client_pipe;
      start_info.hStdOutput   = client_pipe;
      start_info.hStdError    = err_handle ? err_handle : GetStdHandle(STD_ERROR_HANDLE);

      if (!SetHandleInformation(client_pipe, HANDLE_FLAG_INHERIT, 1))
            util::win32::error_exit(L"Named Pipe SetHandleInformation");
      if (!SetHandleInformation(start_info.hStdError, HANDLE_FLAG_INHERIT, 1))
            util::win32::error_exit(L"Stderr SetHandleInformation");

      if (!win32::create_process(argv.c_str(), &start_info, pi))
            util::win32::error_exit(L"CreateProcess() failed");

      //CloseHandle(client_pipe);
}

#if 0

void
win32_named_pipe_impl::open()
{
      throw_if_initialized(typeid(*this), 1);

      std::string debug_fname;
      {
            char buf[PATH_MAX + 1];
            size_t const len = braindead_tempname(buf, R"(\\.\pipe)", MAIN_PROJECT_NAME "-", "-io_pipe");
            pipe_fname_ = util::unistring::recode<wchar_t>(buf, len);
            debug_fname = {buf, len};
      }

      pipe_ = CreateNamedPipeW(
           pipe_fname_.c_str(),
           PIPE_ACCESS_DUPLEX | WRITE_DAC | FILE_FLAG_FIRST_PIPE_INSTANCE,
           PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_NOWAIT,
           PIPE_UNLIMITED_INSTANCES,
           65536,     // outbound buffer
           65536,     // inbound buffer
           0, nullptr // Default wait time and security attrs.
      );

      fmt::print(stderr, FC("Using pipe at `{}' with raw value `{}'.\n"), debug_fname, pipe_);
      fflush(stderr);
      std::wcerr << L"Wide name: "sv << pipe_fname_ << L'\n';
      std::wcerr.flush();

      SecureZeroMemory(&overlapped_, sizeof overlapped_);
      overlapped_.hEvent = CreateEvent( 
         nullptr,   // default security attribute 
         true,      // manual-reset event 
         true,      // initial state = signaled 
         nullptr);  // unnamed event object

      set_initialized(1);
}

procinfo_t
win32_named_pipe_impl::do_spawn_connection(size_t const argc, char **argv)
{
      throw_if_initialized(typeid(*this), 2);
      if (!check_initialized(1))
            open();

      PROCESS_INFORMATION pi;
      auto const safe_argv  = win32::protect_argv(static_cast<int>(argc), argv);
      HANDLE     err_handle = get_err_handle();
      err_handle            = err_handle ? err_handle : GetStdHandle(STD_ERROR_HANDLE);

      named_pipe_spawn_connection_helper(pipe_, err_handle, &pi, pipe_fname_, safe_argv);

      if (err_sink_type_ != sink_type::DEFAULT && err_handle)
            CloseHandle(err_handle);

      set_initialized(2);
      return pi;
}
#endif

void
win32_named_pipe_impl::open()
{
      throw_if_initialized(typeid(*this), 1);

      {
            char buf[PATH_MAX + 1];
            size_t const len = braindead_tempname(buf, R"(\\.\pipe)", MAIN_PROJECT_NAME "-", "-io_pipe");
            pipe_fname_ = util::unistring::recode<wchar_t>(buf, len);
            pipe_narrow_name_ = {buf, len};
      }
      //uv_pipe_bind(&uvhandle_, pipe_narrow_name_.c_str());
      //pipe_   = uvhandle_.handle;
      //crt_fd_ = _open_osfhandle(intptr_t(pipe_), _O_RDWR | _O_BINARY);

      //uv_pipe_open(&pipe_, nullptr);

      pipe_ = CreateNamedPipeW(
           pipe_fname_.c_str(),
           PIPE_ACCESS_DUPLEX | WRITE_DAC | FILE_FLAG_FIRST_PIPE_INSTANCE | FILE_FLAG_OVERLAPPED,
           PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
           PIPE_UNLIMITED_INSTANCES,
           65536,     // outbound buffer
           65536,     // inbound buffer
           0, nullptr // Default wait time and security attrs.
      );

      fmt::print(stderr, FC("Using pipe at `{}' with raw value `{}'.\n"), pipe_narrow_name_, pipe_);
      fflush(stderr);
      std::wcerr << L"Wide name: "sv << pipe_fname_ << L'\n';
      std::wcerr.flush();

      //SecureZeroMemory(&overlapped_, sizeof overlapped_);
      //overlapped_.hEvent = CreateEvent( 
      //   nullptr,   // default security attribute 
      //   true,      // manual-reset event 
      //   true,      // initial state = signaled 
      //   nullptr);  // unnamed event object

      set_initialized(1);
}

procinfo_t
win32_named_pipe_impl::do_spawn_connection(size_t const argc, char **argv)
{
      throw_if_initialized(typeid(*this), 2);
      if (!check_initialized(1))
            open();

      PROCESS_INFORMATION pi;
      auto const safe_argv  = win32::protect_argv(static_cast<int>(argc), argv);
      HANDLE     err_handle = get_err_handle();
      err_handle            = err_handle ? err_handle : GetStdHandle(STD_ERROR_HANDLE);

      //uv_connect_t con{};
      //uv_pipe_connect(&con, &uvchildhandle_,
      //                pipe_narrow_name_.c_str(),
      //                [](uv_connect_t *conp, int status) {
      //                      if (status != 0)
      //                            util::win32::error_exit(L"uv_pipe_connect fucked something up?");
      //                });
      named_pipe_spawn_connection_helper(err_handle, &pi, pipe_fname_, safe_argv);
      crt_fd_ = _open_osfhandle(intptr_t(pipe_), _O_RDWR | _O_BINARY);
      uv_pipe_open(&uvhandle_, crt_fd_);

      if (err_sink_type_ != sink_type::DEFAULT && err_handle)
            CloseHandle(err_handle);

      set_initialized(2);
      return pi;
}


#if 0
void
win32_named_pipe_impl::open()
{
      throw_if_initialized(typeid(*this), 1);

      uv_pipe_open(&pipe_);

      set_initialized(1);
}

procinfo_t
win32_named_pipe_impl::do_spawn_connection(size_t const argc, char **argv)
{
      throw_if_initialized(typeid(*this), 2);
      if (!check_initialized(1))
            open();

      //PROCESS_INFORMATION pi;
      //auto const safe_argv  = win32::protect_argv(static_cast<int>(argc), argv);
      //HANDLE     err_handle = get_err_handle();
      //err_handle            = err_handle ? err_handle : GetStdHandle(STD_ERROR_HANDLE);

#ifdef _WIN32
      constexpr int uvflags[2] = {UV_CREATE_PIPE | UV_READABLE_PIPE | UV_OVERLAPPED_PIPE,
                                  UV_CREATE_PIPE | UV_READABLE_PIPE | UV_WRITABLE_PIPE | UV_OVERLAPPED_PIPE};
#endif

      uv_stdio_container_t cont[] = {
            {.flags = static_cast<uv_stdio_flags>(uvflags[0]),
             .data = { .stream = reinterpret_cast<uv_stream_t *>(&pipe_), }},
            {.flags = static_cast<uv_stdio_flags>(uvflags[1]),
             .data = { .stream = reinterpret_cast<uv_stream_t *>(&pipe_), }},
            {.flags = static_cast<uv_stdio_flags>(UV_INHERIT_FD), .data = { .fd = 2, }},
      };
      uv_process_options_t opts = {
          .file = argv[0],
          .args = argv,
          .flags = UV_PROCESS_WINDOWS_HIDE,
          .stdio_count = std::size(cont),
          .stdio = cont,
      };


      //named_pipe_spawn_connection_helper(pipe_, err_handle, &pi, pipe_fname_, safe_argv);

      //if (err_sink_type_ != sink_type::DEFAULT && err_handle)
      //      CloseHandle(err_handle);

      set_initialized(2);
      return pi;
}
#endif

#if 0
ssize_t
win32_named_pipe_impl::read(void *buf, ssize_t const nbytes, int const flags)
{
      auto    lock  = std::lock_guard{read_mtx_};
      ssize_t total = 0;
      DWORD  n      = 0;
      int    ret;
      //memset(&overlapped_, 0, sizeof overlapped_);
      //overlapped_.hEvent = CreateEvent(nullptr, true, true, nullptr);
      //assert(overlapped_.hEvent != nullptr);

      if (flags & MSG_WAITALL) {
            do {
                  //ret = ReadFile(pipe_, static_cast<char *>(buf) + total, nbytes - total, &n, &overlapped_);
                  //assert(overlapped_.hEvent != nullptr);
                  //if (ret < 0)
                  //      ret = WaitForSingleObject(overlapped_.hEvent, INFINITE);

                  //ret = ReadFile(pipe_, static_cast<char *>(buf) + total, nbytes - total, &n, &ov);
                  //ResetEvent(ov.hEvent);
                  ret = ReadFile(pipe_, static_cast<char *>(buf) + total, nbytes - total, &n, nullptr);
                  if (ret == 0 && n == 0)
                        break;
            } while (ret >= 0 && n != static_cast<uint32_t>(-1) && (total += n) < nbytes);
      } else {
            do {
                  //ret = ReadFile(pipe_, buf, nbytes, &n, &overlapped_);
                  //assert(overlapped_.hEvent != nullptr);
                  //ret = ReadFile(pipe_, static_cast<char *>(buf) + total, nbytes - total, &n, &ov);
                  //if (!ret)
                  //      ret = WaitForSingleObject(overlapped_.hEvent, INFINITE);
                  //ResetEvent(ov.hEvent);
                  ret = ReadFile(pipe_, buf, nbytes, &n, nullptr);
            } while (ret >=0 && n == 0);
            total = n;
      }

      //ResetEvent(overlapped_.hEvent);
      //AUTOC e = GetLastError();
      //if (!ret && e != ERROR_MORE_DATA)
      //if (!ret || e)
            //util::win32::error_exit_explicit(L"ReadFile", e);

      return total;
}

ssize_t
win32_named_pipe_impl::write(void const *buf, ssize_t const nbytes, int const flags)
{
      auto    lock  = std::lock_guard{write_mtx_};
      ssize_t total = 0;
      DWORD   n     = 0;
      int     ret;

      do {
            //ret = WriteFile(pipe_, static_cast<char const *>(buf) + total, nbytes - total, &n, &overlapped_);
            //ret = WaitForSingleObject(overlapped_.hEvent, INFINITE);
            //ResetEvent(overlapped_.hEvent);
            ret = WriteFile(pipe_, static_cast<char const *>(buf) + total, nbytes - total, &n, nullptr);
      } while (ret >= 0 && n != static_cast<uint32_t>(-1) && (total += n) < nbytes);



      total = n;
      return total;
}
#endif


ssize_t
win32_named_pipe_impl::read(void *buf, ssize_t const nbytes, int const flags)
{
      auto    lock  = std::lock_guard{read_mtx_};
      ssize_t total = 0;
      DWORD   n;

      if (flags & MSG_WAITALL) {
            do {
                  if (pipe_ == INVALID_HANDLE_VALUE) [[unlikely]]
                        throw except::connection_closed();

                  bool b = ::ReadFile(pipe_, static_cast<char *>(buf) + total,
                                      DWORD(nbytes - total), &n, nullptr);
                  if (!b)
                        util::win32::error_exit(L"ReadFile()");
            } while (n != SSIZE_C(-1) && (total += n) < nbytes);
      } else {
            if (pipe_ == INVALID_HANDLE_VALUE) [[unlikely]]
                  throw except::connection_closed();
            bool b = ::ReadFile(pipe_, buf, DWORD(nbytes), &n, nullptr);
            if (!b)
                  util::win32::error_exit(L"ReadFile()");
            total = n;
      }

      return total;
}

#if 0
ssize_t
win32_named_pipe_impl::write(void const *buf, ssize_t const nbytes, UNUSED int const flags)
{
      auto    lock  = std::lock_guard{write_mtx_};
      ssize_t total = 0;
      DWORD n;

      do {
            if (pipe_ == reinterpret_cast<HANDLE>(-1)) [[unlikely]]
                  throw except::connection_closed();
            bool b = ::WriteFile(pipe_, static_cast<char const *>(buf) + total, static_cast<size_t>(nbytes - total), &n, nullptr);
            if (!b)
                  util::win32::error_exit(L"ReadFile()");
      } while (n != SSIZE_C(-1) && (total += n) < nbytes);

      return total;
}
#endif

ssize_t
win32_named_pipe_impl::write(void const *buf, ssize_t const nbytes, UNUSED int const flags)
{
      auto    lock  = std::lock_guard{write_mtx_};
      ssize_t total = 0;
      ssize_t n;

      do {
            if (pipe_ == INVALID_HANDLE_VALUE) [[unlikely]]
                  throw except::connection_closed();
            n = ::write(crt_fd_, static_cast<char const *>(buf) + total,
                        static_cast<size_t>(nbytes - total));
      } while (n != SSIZE_C(-1) && (total += n) < nbytes);

      return total;
}

# endif


/*--------------------------------------------------------------------------------------*/


#if 0
static void
open_pipeish_socket(socket_t us[2], socket_t peer[2])
{
      socket_t    listener;
      sockaddr_un addr{};
      std::string path = util::get_temporary_filename(nullptr, ".sock").string();

            if (path.length() + 1 > sizeof(addr.sun_path))
            throw std::logic_error(
                fmt::format(FC("The file path \"{}\" (len: {}) is too large to "
                               "fit into a UNIX socket address (size: {})"),
                            path, path.size() + 1, sizeof(addr.sun_path)));

      memcpy(addr.sun_path, path.c_str(), path.length() + 1);
      addr.sun_family = AF_UNIX;

      listener = ::WSASocketW(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC,
                              0, nullptr, 0, WSA_FLAG_OVERLAPPED);

      if (auto const e = ::WSAGetLastError(); e != 0)
            util::win32::error_exit_explicit(L"socket()", e);

      if (::bind(listener, (sockaddr *)&addr, sizeof addr) < 0)
            util::win32::error_exit_wsa(L"bind()");

      if (::listen(listener, 2) < 0)
            util::win32::error_exit_wsa(L"listen");

      for (int i = 0; i < 2; ++i) {
            auto info         = socket_info{path, listener, {}, {}, true};
            // auto start_thread = std::thread{async_connect_to_unix_socket, &info};
            async_connect_to_unix_socket(&info);

            sockaddr_un con_addr = {};
            int         len      = sizeof(con_addr);
            socket_t    acceptor = ::accept(listener, (sockaddr *)&con_addr, &len);
            if (!acceptor || acceptor == uintptr_t(-1))
                  util::win32::error_exit_wsa(L"accept()");

            // start_thread.join();
            us[i]   = acceptor;
            peer[i] = info.connected_sock;
      }

      closesocket(listener);
}


procinfo_t
dual_socket_connection_impl::do_spawn_connection(size_t argc, char **argv)
{
      throw_if_initialized(typeid(*this), 2);

      socket_t our_socks[2];
      socket_t peer_socks[2];

      // for (int i = 0; i < 2; ++i)
      open_pipeish_socket(our_socks, peer_socks);

      write_           = our_socks[0];
      read_            = our_socks[1];
      AUTOC err_handle = get_err_handle();
      AUTOC pid        = spawn_connection_common(peer_socks[0], peer_socks[1], err_handle,
                                                 static_cast<int>(argc), argv);
      closesocket(peer_socks[0]);
      closesocket(peer_socks[1]);

      if (err_sink_type_ == sink_type::FILENAME)
            ::CloseHandle(err_handle);

      set_initialized(2);
      return pid;
}

void
dual_socket_connection_impl::close()
{
      if (intptr_t(write_) != (-1)) {
            ::shutdown(write_, 2);
            closesocket(write_);
      }
      if (intptr_t(read_) != (-1)) {
            ::shutdown(read_, 2);
            closesocket(read_);
      }

      read_ = write_ = static_cast<SOCKET>(-1);
}

ssize_t
dual_socket_connection_impl::read(void *buf, ssize_t nbytes, int flags)
{
      auto    lock  = std::lock_guard{read_mtx_};
      ssize_t total = 0;
      ssize_t n     = 0;

      if (flags & MSG_WAITALL) {
            flags &= ~MSG_WAITALL;
            do {
                  n = ::recv(read_, static_cast<char *>(buf) + total,
                             nbytes - total, flags);
            } while ((n != (-1)) && (total += n) < nbytes);
      } else {
            for (;;) {
                  n = ::recv(read_, static_cast<char *>(buf), nbytes, flags);
                  if (n != 0)
                        break;
                  std::this_thread::sleep_for(10ms);
            }
            total = n;
      }

      if (n == (-1)) [[unlikely]] {
            if (WSAGetLastError() == WSAEBADF)
                  throw except::connection_closed();
             util::win32::error_exit_wsa(L"recv()");
      }

      return total;
}

ssize_t
dual_socket_connection_impl::write(void const *buf, ssize_t nbytes, int flags)
{
      auto    lock  = std::lock_guard{write_mtx_};
      ssize_t total = 0;
      ssize_t n;

      do {
            n = ::send(write_, static_cast<char const *>(buf) + total,
                       nbytes - total, flags);
      } while (n != (-1) && (total += n) < nbytes);

      if (n == (-1)) [[unlikely]] {
            if (WSAGetLastError() == WSAEBADF)
                  throw except::connection_closed();
            util::win32::error_exit_wsa(L"send()");
      }

      return total;
}
#endif


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
      err_nothrow(1, "aborting");
}


static procinfo_t
spawn_connection_common(socket_t const      stdout_sock,
                        socket_t const      stdin_sock,
                        int const           err_fd,
                        UNUSED size_t const argc,
                        char              **argv)
{
      pid_t const this_id = ::getpid();
      procinfo_t  pid;

      if ((pid = ::fork()) == 0) {
            int const io_in  = stdin_sock >= 0
                             ? stdin_sock
                             : ::open("/dev/null", O_RDWR | O_BINARY | O_APPEND, 0600);
            int const io_out = stdout_sock >= 0
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

            ::execvp(argv[0], const_cast<char **>(argv));
            patricide(this_id, "execvp() failed");
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
            err(1, "socketpair()");
      acc_read_  = fds[0];
      con_read_ = fds[1];

      if (use_dual_sockets()) {
            if (socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, fds) == (-1))
                  err(1, "socketpair()");
            acc_write_ = fds[0];
            con_write_ = fds[1];
      } else {
            acc_write_ = acc_read_;
            con_write_ = con_read_;
      }

      return fds[0];
}

socket_t
socketpair_connection_impl::connect_to_socket()
{
      return con_read_; /* ugh */
}

# endif


/****************************************************************************************/
/* Pipe Connection */


void
pipe_connection_impl::close()
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
      if (::pipe(fds) == (-1))
            err(1, "pipe2()");

# ifdef F_SETPIPE_SZ
      if (::fcntl(fds[0], F_SETPIPE_SZ, 65536) == (-1))
            err(2, "fcntl(F_SETPIPE_SZ)");
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
                  patricide(this_id, "execvp() failed");
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

/*======================================================================================*/
/*======================================================================================*/
#endif // defined _WIN32
/*======================================================================================*/
/*======================================================================================*/


/* Not system specific. */

ssize_t
pipe_connection_impl::read(void *buf, ssize_t const nbytes, int const flags)
{
      auto    lock  = std::lock_guard{read_mtx_};
      ssize_t total = 0;

      // unsigned file_flags = ::fcntl(read_, F_GETFL);
      // ::fcntl(read_, F_SETFL, file_flags & static_cast<unsigned>(~O_NONBLOCK));

      if (flags & MSG_WAITALL) {
            ssize_t n;
            do {
                  if (read_ == (-1)) [[unlikely]]
                        throw except::connection_closed();
                  n = ::read(read_, static_cast<char *>(buf) + total,
                             static_cast<size_t>(nbytes - total));
            } while (n != SSIZE_C(-1) && (total += n) < nbytes);
      } else {
            if (read_ == (-1)) [[unlikely]]
                  throw except::connection_closed();
            total = ::read(read_, buf, static_cast<size_t>(nbytes));
      }

      return total;
}

ssize_t
pipe_connection_impl::write(void const *buf, ssize_t const nbytes, UNUSED int const flags)
{
      auto    lock  = std::lock_guard{write_mtx_};
      ssize_t total = 0;
      ssize_t n;

      do {
            if (write_ == (-1)) [[unlikely]]
                  throw except::connection_closed();
            n = ::write(write_, static_cast<char const *>(buf) + total,
                        static_cast<size_t>(nbytes - total));
      } while (n != SSIZE_C(-1) && (total += n) < nbytes);

      return total;
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

      if (should_connect()) {
            do_open_unix_socket_connection(listener_, acc_read_, con_read_, path_);
            if (use_dual_sockets()) {
                  do_open_unix_socket_connection(listener_, acc_write_, con_write_, path_);
            } else {
                  con_write_ = con_read_;
                  acc_write_ = acc_read_;
            }
      }

      set_initialized(1);
}

procinfo_t
unix_socket_connection_impl::do_spawn_connection(size_t const argc, char **argv)
{
      throw_if_initialized(typeid(*this), 2);
      if (!check_initialized(1))
            open();

      err_fd_ = get_err_handle();

      procinfo_t const pid =
#ifdef _WIN32
          use_shim_ ? win32::spawn_connection_shim(util::recode<wchar_t>(path_), err_fd_, argc, argv)
                    :
#endif
                    spawn_connection_common(con_read_, con_write_, err_fd_, argc, argv);

      util::close_descriptor(con_read_);
      util::close_descriptor(con_write_);

      if (err_sink_type_ == sink_type::FILENAME)
            util::close_descriptor(err_fd_);

      set_initialized(2);
      return pid;
}

socket_t
unix_socket_connection_impl::open_new_socket()
{
      if (path_.length() + 1 > sizeof(addr_.sun_path))
            throw std::logic_error(
                fmt::format(FC("The file path \"{}\" (len: {}) is too large to "
                               "fit into a UNIX socket address (size: {})"),
                            path_, path_.size()+1, sizeof(addr_.sun_path)));

      memcpy(addr_.sun_path, path_.c_str(), path_.length() + 1);
      addr_.sun_family = AF_UNIX;

      socket_t listener = (-1);

      if (should_open_listener()) {
            listener = call_socket_listener(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);

            if (listener == (-1))
                  err(1, "socket");

            AUTOC ret = ::bind(listener, (sockaddr *)&addr_, sizeof addr_);
            if (ret == (-1))
                  err(1, "bind");

            if (::listen(listener, 255) == (-1))
                  err(1, "listen");
      }

      return listener;
}

socket_t
unix_socket_connection_impl::connect_to_socket()
{
      return acc_read_;
}

void
unix_socket_connection_impl::set_listener(socket_t const sock, sockaddr_un const &addr)
{
      should_close_listnener(false);
      listener_ = sock;
      addr_     = addr;
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

      socket_t const connector = ::socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);

      if (connector == invalid_socket) [[unlikely]]
            ERR(1, "socket");
      if (::connect(connector, (sockaddr const *)&addr_, sizeof addr_) == invalid_socket)
            ERR(1, "connect");

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
      socklen_t   len      = sizeof(con_addr);
      acc_read_ = ::accept(listener_, (sockaddr *)&con_addr, &len);
      if (acc_read_ == invalid_socket)
            ERR(1, "accept()");

      if (use_dual_sockets()) {
            acc_write_ = ::accept(listener_, (sockaddr *)&con_addr, &len);
            if (acc_read_ == invalid_socket)
                  ERR(1, "accept()");
      } else {
            acc_write_ = acc_read_;
      }

      return acc_read_;
}


/*======================================================================================*/
/* Inet Connection */


static int
lazy_getsockname(socket_t const listener, sockaddr *addr, socklen_t const size)
{
      socklen_t size_local = size;
      int ret  = ::getsockname(listener, addr, &size_local);
      if (ret != 0 || size != size_local)
            ERR(1, "getsockname()");

      return 0;
}

static socket_t
open_new_inet_socket(sockaddr const *addr,
                     socklen_t const size,
                     int const       type,
                     int const       queue    = 255,
                     int const       protocol = 0)
{
      socket_t const listener = call_socket_listener(type, SOCK_STREAM, protocol);
      if (listener == invalid_socket) [[unlikely]]
            ERR(1, "socket()");

      int ret = ::bind(listener, addr, size);
      if (ret == -1) [[unlikely]]
            ERR(1, "bind()");

      ret = ::listen(listener, queue);
      if (ret == (-1)) [[unlikely]]
            ERR(1, "listen()");

      /* We want to find out the port number. */
      // lazy_getsockname(listener, addr, size);
      return listener;
}

socket_t
connect_to_inet_socket(sockaddr const *addr,
                       socklen_t const size,
                       int const       type,
                       int const       protocol)
{
      socket_t const connector = call_socket_connect(type, SOCK_STREAM, protocol);
      if (connector == invalid_socket)
            ERR(1, "socket()");

      int const ret = ::connect(connector, addr, size);
      if (ret == -1)
            ERR(1, "connect()");

      return connector;
}


/*--------------------------------------------------------------------------------------*/
/* Either */


inet_any_socket_connection_impl::inet_any_socket_connection_impl() 
{
      addr_ = nullptr;
}

inet_any_socket_connection_impl::~inet_any_socket_connection_impl()
{
      ::free(this->addr_);  //NOLINT
}

void
inet_any_socket_connection_impl::open()
{
      throw_if_initialized(typeid(*this), 1);
      connect_to_socket();

      if (should_connect()) {
            socklen_t size = addr_length_;
            acc_read_      = ::accept(listener_, addr_, &size);
            if (acc_read_ == invalid_socket || size != addr_length_)
                  ERR(1, "accept");
            if (use_dual_sockets()) {
                  acc_write_ = ::accept(listener_, addr_, &size);
                  if (acc_write_ == invalid_socket || size != addr_length_)
                        ERR(1, "accept");
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

      err_fd_   = get_err_handle();
      AUTOC ret = spawn_connection_common(con_read_, con_write_, err_fd_, argc, argv);
      util::close_descriptor(con_read_);
      util::close_descriptor(con_write_);

      set_initialized(2);
      return ret;
}

socket_t
inet_any_socket_connection_impl::open_new_socket()
{
      if (!addr_init_) {
            addr_length_ = sizeof(sockaddr_in6);
            type_        = AF_INET6;

            auto *addr        = ::util::xcalloc<sockaddr_in6>();
            addr->sin6_addr   = IN6ADDR_LOOPBACK_INIT;
            addr->sin6_family = type_;  // Default to ipv6 I guess
            addr->sin6_port   = 0;
            addr_             = reinterpret_cast<sockaddr *>(addr);
            addr_init_        = true;
      }

      if (should_open_listener())
            listener_ = open_new_inet_socket(addr_, addr_length_, type_);

      addr_init_ = true;
      return listener_;
}

socket_t
inet_any_socket_connection_impl::connect_to_socket()
{
      if (intptr_t(listener_) > 0) {
            if (!addr_init_)
                  addr_init_ = true;
            /* else { nothing to do, listener is open and addr_ is set up. } */
      } else if (should_open_listener()) {
            listener_ = open_new_socket();
      }

      memset(addr_, 0, addr_length_);
      lazy_getsockname(listener_, addr_, addr_length_);

      if (addr_string_.empty()) {
            char buf[128];
            if (type_ == AF_INET) {
                  auto *a = reinterpret_cast<sockaddr_in *>(addr_);
                  inet_ntop(AF_INET, &a->sin_addr, buf, sizeof buf);
                  addr_string_ = buf;
            } else if (type_ == AF_INET6) {
                  auto *a = reinterpret_cast<sockaddr_in6 *>(addr_);
                  inet_ntop(AF_INET6, &a->sin6_addr, buf, sizeof buf);
                  addr_string_ = buf;
            } else {
                  errx(1, "Invalid family %s", type_);
            }
      }

      if (type_ == AF_INET) {
            hport_ = ntohs(reinterpret_cast<sockaddr_in *>(addr_)->sin_port);
      } else if (type_ == AF_INET6) {
            hport_ = ntohs(reinterpret_cast<sockaddr_in6 *>(addr_)->sin6_port);
      } else {
            errx(1, "Invalid family %s", type_);
      }

      if (should_connect()) {
            con_read_ = connect_to_inet_socket(addr_, addr_length_, type_);
            if (use_dual_sockets())
                  con_write_ = connect_to_inet_socket(addr_, addr_length_, type_);
            else
                  con_write_ = con_read_;
      }

      return con_read_;
}


void inet_any_socket_connection_impl::assign_from_addrinfo(addrinfo const *info)
{
      if (addr_)
            free(addr_); //NOLINT
      type_        = info->ai_family;
      addr_length_ = static_cast<socklen_t>(info->ai_addrlen);
      addr_        = util::xcalloc<sockaddr>(1, addr_length_);
      memcpy(addr_, info->ai_addr, info->ai_addrlen);
      addr_init_ = true;
      // set_initialized(1);
}


int inet_any_socket_connection_impl::resolve(char const *server_name, char const *port)
{
      auto *info = ::util::ipc::resolve_addr(server_name, port);
      if (!info)
            return (-1);
      // addr_string_ = server_name;
      assign_from_addrinfo(info);
      freeaddrinfo(info);
      return 0;
}


int inet_any_socket_connection_impl::resolve(char const *const server_and_port)
{
#define BAD_ADDRESS(ipver)      \
      throw std::runtime_error( \
          fmt::format(FC("Invalid IPv" ipver " address \"{}\" - @(line {})"), \
                      server_and_port, __LINE__));

      char buf[48];
      char const *server;
      char const *port;

      if (server_and_port[0] == '[') {
            /* IPv6 address, in brackets, hopefully with port.
             * e.g. [::1]:12345 */
            if (!*(server = server_and_port + 1))
                  BAD_ADDRESS("6");
            if (!(port = strchr(server, ']')))
                  BAD_ADDRESS("6");
            size_t const size = port - server;
            if (size > SIZE_C(39) || port[1] != ':' || !isdigit(port[2]))
                  BAD_ADDRESS("6");

            memcpy(buf, server, size);
            buf[size] = '\0';
            server    = buf;
            port     += 2;
      } else {
            /* Assume it's an ipv4 address with port.
             * e.g. 127.0.0.1:12345 */
            port              = strchr(server_and_port, ':');
            size_t const size = port - server_and_port;
            memcpy(buf, server_and_port, size);
            buf[size] = '\0';
            server    = buf;

            /* Ensure it's valid, and that the colon is the only one in the string in
             * case we've been given a bare IPv6 address. */
            if (!port || !isdigit(port[1]) || strchr(port + 1, ':'))
                  BAD_ADDRESS("4");
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
      connect_to_socket();

      if (should_connect()) {
            socklen_t size = sizeof(addr_);
            acc_read_ = ::accept(listener_, (sockaddr *)&addr_, &size);
            if (acc_read_ == invalid_socket || size != sizeof(addr_))
                  ERR(1, "accept");
            if (use_dual_sockets()) {
                  acc_write_ = ::accept(listener_, (sockaddr *)&addr_, &size);
                  if (acc_write_ == invalid_socket || size != sizeof(addr_))
                        ERR(1, "accept");
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

      err_fd_    = get_err_handle();
      AUTOC ret  = spawn_connection_common(con_read_, con_write_, err_fd_, argc, argv);
      util::close_descriptor(con_read_);
      util::close_descriptor(con_write_);

      set_initialized(2);
      return ret;
}

socket_t
inet_ipv4_socket_connection_impl::open_new_socket()
{
      if (!addr_init_) {
            memset(&addr_, 0, sizeof(addr_));
            //addr_.sin_addr.s_addr = (0x0100'007F); // 127.0.0.1
            addr_.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // 127.0.0.1
            addr_.sin_family      = AF_INET;
            addr_.sin_port        = 0;
            addr_string_          = "127.0.0.1"s;
      }

      listener_  = open_new_inet_socket((sockaddr *)&addr_, sizeof(addr_), AF_INET);
      memset(&addr_, 0, sizeof(addr_));
      lazy_getsockname(listener_, (sockaddr *)&addr_, sizeof(addr_));
      addr_init_ = true;

      if (addr_string_.empty()) {
            char buf[128];
            addr_string_ = inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof buf);
      }
      hport_ = ntohs(addr_.sin_port);

      return listener_;
}

socket_t
inet_ipv4_socket_connection_impl::connect_to_socket()
{
      if (intptr_t(listener_) > 0) {
            if (!addr_init_) {
                  lazy_getsockname(listener_, (sockaddr *)&addr_, sizeof(addr_));
                  addr_init_ = true;
            }
            /* else { nothing to do, listener is open and addr_ is set up. } */
      } else if (should_open_listener()) {
            listener_ = open_new_socket();
      }

      if (should_connect()) {
            con_read_ = connect_to_inet_socket((sockaddr *)&addr_,
                                                sizeof(addr_), AF_INET);
            if (use_dual_sockets())
                  con_write_ = connect_to_inet_socket((sockaddr *)&addr_,
                                                       sizeof(addr_), AF_INET);
            else
                  con_write_ = con_read_;
      }

      return con_read_;
}


/*--------------------------------------------------------------------------------------*/
/* IPv6 only */


void
inet_ipv6_socket_connection_impl::open()
{
      throw_if_initialized(typeid(*this), 1);
      connect_to_socket();

      if (should_connect()) {
            socklen_t size = sizeof(addr_);
            acc_read_ = ::accept(listener_, (sockaddr *)&addr_, &size);
            if (acc_read_ == invalid_socket || size != sizeof(addr_))
                  ERR(1, "accept");
            if (use_dual_sockets()) {
                  acc_write_ = ::accept(listener_, (sockaddr *)&addr_, &size);
                  if (acc_write_ == invalid_socket || size != sizeof(addr_))
                        ERR(1, "accept");
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

      err_fd_   = get_err_handle();
      AUTOC ret = spawn_connection_common(con_read_, con_write_, err_fd_, argc, argv);
      util::close_descriptor(con_read_);
      util::close_descriptor(con_write_);

      set_initialized(2);
      return ret;
}

socket_t
inet_ipv6_socket_connection_impl::open_new_socket()
{
      if (!addr_init_) {
            memset(&addr_, 0, sizeof(addr_));
            addr_.sin6_addr   = IN6ADDR_LOOPBACK_INIT;
            addr_.sin6_family = AF_INET6;  // Default to ipv6 I guess
            addr_.sin6_port   = 0;
            addr_string_      = "[::1]"s;
      }

      if (should_open_listener()) {
            listener_ = open_new_inet_socket((sockaddr *)&addr_,
                                             sizeof(addr_), AF_INET6);
            memset(&addr_, 0, sizeof(addr_));
            lazy_getsockname(listener_, (sockaddr *)&addr_, sizeof(addr_));
      }

      addr_init_ = true;
      if (addr_string_.empty()) {
            char buf[128];
            addr_string_ = inet_ntop(AF_INET6, &addr_.sin6_addr, buf, sizeof buf);
      }
      hport_ = ntohs(addr_.sin6_port);
      return listener_;
}

socket_t
inet_ipv6_socket_connection_impl::connect_to_socket()
{
      if (intptr_t(listener_) > 0) {
            if (!addr_init_) {
                  lazy_getsockname(listener_, (sockaddr *)&addr_, sizeof(addr_));
                  addr_init_ = true;
            }
            /* else { nothing to do, listener is open and addr_ is set up. } */
      } else if (should_open_listener()) {
            listener_ = open_new_socket();
      }

      con_read_ = connect_to_inet_socket((sockaddr *)&addr_, sizeof(addr_), AF_INET6);
      if (use_dual_sockets())
            con_write_ = connect_to_inet_socket((sockaddr *)&addr_, sizeof(addr_), AF_INET6);
      else
            con_write_ = con_read_;

      return con_read_;
}


/****************************************************************************************/
} // namespace ipc::detail
} // namespace emlsp
