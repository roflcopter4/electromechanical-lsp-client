#include "Common.hh"
#include "ipc/connection_impl.hh"

#include "util/charconv.hh"

#ifndef O_BINARY
# define O_BINARY 0
#endif
#ifndef SOCK_CLOEXEC
# define SOCK_CLOEXEC 0
#endif

#define AUTOC auto const

inline namespace emlsp {
namespace ipc::detail {
/****************************************************************************************/


#ifdef _WIN32
namespace win32 {

# define READ_FD     0
# define WRITE_FD    1
# define INPUT_PIPE  0
# define OUTPUT_PIPE 1

static int
start_process_with_pipe(UNUSED char const   *exe,
                        wchar_t const       *argv,
                        int                  pipefds[2],
                        HANDLE const         err_handle,
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

      if (!CreatePipe(&handles[INPUT_PIPE][READ_FD], &handles[INPUT_PIPE][WRITE_FD], &sec_attr, 0))
            util::win32::error_exit(L"Stdin CreatePipe()");
      if (!SetHandleInformation(handles[INPUT_PIPE][WRITE_FD], HANDLE_FLAG_INHERIT, 0))
            util::win32::error_exit(L"Stdin SetHandleInformation() (write end)");

      if (!CreatePipe(&handles[OUTPUT_PIPE][READ_FD], &handles[OUTPUT_PIPE][WRITE_FD], &sec_attr, 0))
            util::win32::error_exit(L"Stdout CreatePipe()");
      if (!SetHandleInformation(handles[OUTPUT_PIPE][READ_FD], HANDLE_FLAG_INHERIT, 0))
            util::win32::error_exit(L"Stdout SetHandleInformation() (read end)");

      start_info.cb         = sizeof(STARTUPINFOA);
      start_info.dwFlags    = STARTF_USESTDHANDLES;
      start_info.hStdInput  = handles[INPUT_PIPE][READ_FD];
      start_info.hStdOutput = handles[OUTPUT_PIPE][WRITE_FD];
      start_info.hStdError  = err_handle ? err_handle : GetStdHandle(STD_ERROR_HANDLE);

      if (!SetHandleInformation(start_info.hStdError, HANDLE_FLAG_INHERIT, 1))
            util::win32::error_exit(L"Stderr SetHandleInformation");

      /* _open_osfhandle() ought to transfer ownership of the handle to the file
       * descriptor. The original HANDLE isn't leaked when it goes out of scope. */
      pipefds[WRITE_FD] = _open_osfhandle(reinterpret_cast<intptr_t>(handles[INPUT_PIPE][WRITE_FD]), _O_APPEND);
      pipefds[READ_FD]  = _open_osfhandle(reinterpret_cast<intptr_t>(handles[OUTPUT_PIPE][READ_FD]), _O_RDONLY);

      if (!CreateProcessW(nullptr, const_cast<LPWSTR>(argv), nullptr, nullptr,
                          TRUE, 0, nullptr, nullptr, &start_info, pi))
            util::win32::error_exit(L"CreateProcess() failed");

      CloseHandle(handles[INPUT_PIPE][READ_FD]);
      CloseHandle(handles[OUTPUT_PIPE][WRITE_FD]);

      return 0;
}

# undef READ_FD
# undef WRITE_FD
# undef INPUT_PIPE
# undef OUTPUT_PIPE

struct socket_info {
      std::string  path;
      std::wstring cmdline;
      socket_t     listener_sock;
      socket_t     connected_sock;
      socket_t     accepted_sock;
      HANDLE       err_handle;
      PROCESS_INFORMATION pid;
};

/*
 * XXX
 * This function "borrowed" from glib, with some modifications.
 * XXX
 */
/* gspawn-win32-helper.c - Helper program for process launching on Win32.
 *
 *  Copyright 2000 Red Hat, Inc.
 *  Copyright 2000 Tor Lillqvist
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, see <http://www.gnu.org/licenses/>.
 */
static std::wstring
protect_argv(int const argc, char **argv)
{
      std::wstring str;

      /* Quote each argv element if necessary, so that it will get
       * reconstructed correctly in the C runtime startup code.  Note that
       * the unquoting algorithm in the C runtime is really weird, and
       * rather different than what Unix shells do. See stdargv.c in the C
       * runtime sources (in the Platform SDK, in src/crt).
       *
       * Note that a new_wargv[0] constructed by this function should
       * *not* be passed as the filename argument to a _wspawn* or _wexec*
       * family function. That argument should be the real file name
       * without any quoting.
       */
      for (int i = 0; i < argc; ++i) {
            char *p              = argv[i];
            int   pre_bslash     = 0;
            bool  need_dblquotes = false;

            while (*p) {
                  if (*p == L' ' || *p == L'\t')
                        need_dblquotes = true;
                  ++p;
            }

            p = argv[i];
            if (need_dblquotes)
                  str += L'"';

            while (*p) {
                  if (*p == L'"') {
                        /* Add backslash for escaping quote itself */
                        str += L'\\';
                        /* Add backslash for every preceding backslash for escaping it */
                        for (; pre_bslash > 0; --pre_bslash)
                              str += L'\\';
                  }

                  /* Count length of continuous sequence of preceding backslashes. */
                  if (*p == '\\')
                        ++pre_bslash;
                  else
                        pre_bslash = 0;

                  str += *p++;
            }

            if (need_dblquotes) {
                  /* Add backslash for every preceding backslash for escaping it,
                   * do NOT escape quote itself. */
                  for (; pre_bslash > 0; --pre_bslash)
                        str += L'\\';
                  str += L'"';
            }

            str += L' ';
      }

      return str;
}

static void
start_process_with_socket_wrapper(socket_info *info, int *errval)
{
      try {
            info->connected_sock = util::ipc::connect_to_socket(info->path.c_str());
      } catch (std::exception &e) {
            fmt::print(stderr,
                       FC("Caught exception \"{}\" while attempting to connect to socket at \"{}\".\n"),
                       e.what(), info->path);
            *errval = ::WSAGetLastError();
            return;
      }

      STARTUPINFOW startinfo = {};
      startinfo.cb           = sizeof(startinfo);
      startinfo.dwFlags      = STARTF_USESTDHANDLES;
      startinfo.hStdInput    = reinterpret_cast<HANDLE>(info->connected_sock); //NOLINT
      startinfo.hStdOutput   = reinterpret_cast<HANDLE>(info->connected_sock); //NOLINT
      startinfo.hStdError    = info->err_handle;

      if (!::SetHandleInformation(startinfo.hStdError, HANDLE_FLAG_INHERIT, 1))
            util::win32::error_exit(L"Stderr SetHandleInformation");

      bool b = ::CreateProcessW(nullptr, const_cast<LPWSTR>(info->cmdline.c_str()), nullptr,
                                nullptr, true, 0, nullptr, nullptr, &startinfo, &info->pid);

      AUTOC error = ::GetLastError();
      *errval     = static_cast<int>(b ? 0LU : error);
}

} // namespace win32

#endif // defined _WIN32

/****************************************************************************************/
/* Unix Socket Connection */

procinfo_t
unix_socket_connection_impl::do_spawn_connection(UNUSED size_t const argc, char **argv)
{
      throw_if_initialized(typeid(*this));

      if (listener_ == static_cast<socket_t>(-1)) {
            path_     = util::get_temporary_filename("sock.").string();
            listener_ = open_new_socket();
      }

#ifdef _WIN32
      int  errval = 0;
      auto info = win32::socket_info{path_, win32::protect_argv(argc, argv),
                                     listener_, {}, {}, get_err_handle(), {}};
      auto start_thread = std::thread{win32::start_process_with_socket_wrapper,
                                      &info, &errval};

      sockaddr_un con_addr = {};
      int         len      = sizeof(con_addr);
      AUTOC acceptor       = ::accept(listener_, reinterpret_cast<sockaddr *>(&con_addr), &len);

      if (!acceptor || acceptor == static_cast<uintptr_t>(-1))
            err(1, "accept()");

      start_thread.join();
      if (errval != 0)
            errx(errval, "Process creation failed -> %d.", errval);
      if (err_sink_type_ != sink_type::DEFAULT)
            ::CloseHandle(info.err_handle);

      procinfo_t const *pid_ptr = &info.pid;

#else // Not Windows

      procinfo_t pid;
      err_fd_ = get_err_handle();

      if ((pid = ::fork()) == 0) {
            socket_t dsock = connect_to_socket();
            ::dup2(dsock, STDOUT_FILENO);
            ::dup2(dsock, STDIN_FILENO);
            ::close(dsock);
            assert(argv[0] != nullptr);

            if (err_fd_ > 2) {
                  ::dup2(err_fd_, 2);
                  ::close(err_fd_);
            }

            ::execvp(argv[0], const_cast<char **>(argv));
            err(1, "exec failed");
      }

      socket_t connected_sock;
      {
            sockaddr_un con_addr{};
            socklen_t   len = sizeof(con_addr);
            connected_sock  = ::accept(listener_, reinterpret_cast<sockaddr *>(&con_addr), &len);
            if (connected_sock == (-1))
                  err(1, "accept");
      }

      procinfo_t const *pid_ptr = &pid;
#endif

      acceptor_ = acceptor;
      set_to_initialized();
      return *pid_ptr;
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

      socket_t const connection_sock = ::socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);

#ifdef _WIN32
      if (auto const e = ::WSAGetLastError(); e != 0)
            err(1, "socket(): %d", e);
#else
      if (connection_sock == (-1))
           err(1, "socket");
      int tmp = ::fcntl(connection_sock, F_GETFL);
      ::fcntl(connection_sock, F_SETFL, tmp | O_CLOEXEC);
#endif

      AUTOC ret = ::bind(connection_sock, reinterpret_cast<sockaddr *>(&addr_), sizeof addr_);
      if (ret == (-1)) {
            auto const e = fmt::system_error(errno, "bind() @({}:{})", __FILE__, __LINE__);
            std::cerr << e.what() << std::endl;
            abort();
      }
           // err(1, "bind");
      if (::listen(connection_sock, 0) == (-1))
           err(1, "listen");

      return connection_sock;
}

socket_t
unix_socket_connection_impl::connect_to_socket()
{
      socket_t const connector = ::socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);

#ifdef _WIN32
      if (auto const e = ::WSAGetLastError(); e != 0)
            err(1, "socket(): %d", e);
#else
      if (data_sock == (-1))
           err(1, "socket");
      int tmp = ::fcntl(data_sock, F_GETFL);
      ::fcntl(data_sock, F_SETFL, tmp | O_CLOEXEC);
#endif

      AUTOC ret = ::connect(connector, reinterpret_cast<sockaddr const *>(&addr_), sizeof addr_);
      if (ret == (-1))
            err(1, "connect");

      return connector;
}

/*--------------------------------------------------------------------------------------*/

#ifdef HAVE_SOCKETPAIR

procinfo_t
socketpair_connection_impl::do_spawn_connection(UNUSED size_t const argc, char **argv)
{
      throw_if_initialized(typeid(*this));

      procinfo_t pid;
      acceptor_ = open_new_socket();
      err_fd_       = get_err_handle();

# ifdef _WIN32 // Maybe someday windows will get socketpair()?...
      auto const cmdline = win32::protect_argv(argc, argv);
      socket_t   dsock   = connect_to_socket();

      STARTUPINFOA startinfo = {};
      startinfo.cb           = sizeof(startinfo);
      startinfo.dwFlags      = STARTF_USESTDHANDLES;
      startinfo.hStdInput    = reinterpret_cast<HANDLE>(dsock); //NOLINT
      startinfo.hStdOutput   = reinterpret_cast<HANDLE>(dsock); //NOLINT
      startinfo.hStdError    = err_fd_;

      if (!::SetHandleInformation(startinfo.hStdError, HANDLE_FLAG_INHERIT, 1))
            util::win32::error_exit(L"Stderr SetHandleInformation");

      ::CreateProcessA(nullptr, const_cast<LPSTR>(info->cmdline.c_str()), nullptr,
                       nullptr, true, 0, nullptr, nullptr, &startinfo, &info->pid);
      AUTOC errval = ::GetLastError();

      if (errval != 0)
            errx(errval, "Process creation failed -> %d.", errval);
      if (err_sink_type_ != sink_type::DEFAULT)
            ::CloseHandle(err_handle);

      pid = info.pid;

# else // Not Windows

      if ((pid = ::fork()) == 0) {
            socket_t dsock = connect_to_socket();
            ::dup2(dsock, STDOUT_FILENO);
            ::dup2(dsock, STDIN_FILENO);
            ::close(dsock);
            assert(argv[0] != nullptr);

            if (err_fd_ > 2) {
                  ::dup2(err_fd_, 2);
                  ::close(err_fd_);
            }

            ::execvp(argv[0], const_cast<char **>(argv));
            err(1, "exec failed");
      }
#endif

      set_to_initialized();
      return pid;
}

socket_t
socketpair_connection_impl::open_new_socket()
{
      if (socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, fds_) == (-1))
            err(1, "socketpair()");
      return fds_[0];
}

socket_t
socketpair_connection_impl::connect_to_socket()
{
      return fds_[1]; /* ugh */
}

#endif

/*--------------------------------------------------------------------------------------*/
/* Pipe Connection */

#ifdef _WIN32

procinfo_t
pipe_connection_impl::do_spawn_connection(size_t const argc, char **argv)
{
      int pipefds[2];
      procinfo_t pid{};

      HANDLE err_handle = get_err_handle();
      AUTOC  cmdline    = win32::protect_argv(static_cast<int>(argc), argv);

      win32::start_process_with_pipe(argv[0], cmdline.c_str(),
                                     pipefds, err_handle, &pid);

      if (err_sink_type_ != sink_type::DEFAULT)
            CloseHandle(err_handle);

      read_  = pipefds[0];
      write_ = pipefds[1];

      if (_setmode(write_, _O_BINARY) == (-1))
            util::win32::error_exit(L"_setmode(write_)");
      if (_setmode(read_, _O_BINARY) == (-1))
            util::win32::error_exit(L"_setmode(read_)");

      return pid;
}

#else // Not Windows

# define READ_FD  (0)
# define WRITE_FD (1)

static void
open_pipe(int fds[2])
{
# ifdef HAVE_PIPE2
      if (::pipe2(fds, O_CLOEXEC) == (-1))
            err(1, "pipe2()");
# else
      int flg;
      if (::pipe(fds) == (-1))
            err(1, "pipe()");
      for (int i = 0; i < 2; ++i) {
            if ((flg = ::fcntl(fds[i], F_GETFL)) == (-1))
                  err(3 + i, "fcntl(F_GETFL)");
            if (::fcntl(fds[i], F_SETFL, flg | O_CLOEXEC) == (-1))
                  err(5 + i, "fcntl(F_SETFL)");
      }
# endif
# ifdef __linux__ /* Can't do this on the BSDs. */
      if (::fcntl(fds[0], F_SETPIPE_SZ, 16384) == (-1))
            err(2, "fcntl(F_SETPIPE_SZ)");
# endif
}

procinfo_t
pipe_connection_impl::do_spawn_connection(UNUSED size_t argc, char **argv)
{
      throw_if_initialized(typeid(*this));

      int        fds[2][2];
      procinfo_t pid;

      err_fd_ = get_err_handle();

      open_pipe(fds[0]);
      open_pipe(fds[1]);

      if ((pid = fork()) == 0) {
            if (::dup2(fds[0][READ_FD], STDIN_FILENO) == (-1))
                  err(1, "dup2() failed\n");
            if (::dup2(fds[1][WRITE_FD], STDOUT_FILENO) == (-1))
                  err(1, "dup2() failed\n");
            if (err_fd_ > 2) {
                  if (::dup2(err_fd_, STDERR_FILENO) == (-1))
                        err(1, "dup2() failed\n");
                  ::close(err_fd_);
            }

            ::close(fds[0][0]);
            ::close(fds[0][1]);
            ::close(fds[1][0]);
            ::close(fds[1][1]);

            if (::execvp(argv[0], argv) == (-1))
                  err(1, "exec() failed\n");
      }

      ::close(fds[0][READ_FD]);
      ::close(fds[1][WRITE_FD]);
      if (err_fd_ > 2)
            ::close(err_fd_);

      write_ = fds[0][WRITE_FD];
      read_  = fds[1][READ_FD];

      set_to_initialized();
      return pid;
}

#endif // ifdef WIN32

void pipe_connection_impl::close()
{
      if (read_ > 2)
            ::close(read_);
      if (write_ > 2)
            ::close(write_);
      read_  = (-1);
      write_ = (-1);
}

ssize_t
pipe_connection_impl::read(void *buf, ssize_t const nbytes, int const flags)
{
      ssize_t total = 0;

      if (flags & MSG_WAITALL) {
            ssize_t n;
            do {
                  if (read_ == (-1)) [[unlikely]]
                        throw except::connection_closed();
                  n = ::read(read_, static_cast<char *>(buf) + total, static_cast<size_t>(nbytes - total));
            } while (n != SSIZE_C(-1) && (total += n) < nbytes);
      } else {
            if (read_ == (-1)) [[unlikely]]
                  throw except::connection_closed();
            total = ::read(read_, buf, static_cast<size_t>(nbytes));
      }

      return static_cast<ssize_t>(total);
}

ssize_t
pipe_connection_impl::write(void const *buf, ssize_t const nbytes, UNUSED int const flags)
{
      ssize_t total = 0;
      ssize_t n;

      do {
            if (write_ == (-1)) [[unlikely]]
                  throw except::connection_closed();
            n = ::write(write_, static_cast<char const *>(buf) + total, static_cast<size_t>(nbytes - total));
      } while (n != SSIZE_C(-1) && (total += n) < nbytes);

      return total;
}

/*--------------------------------------------------------------------------------------*/

#ifdef _WIN32
# ifdef WIN32_USE_PIPE_IMPL

static void
named_pipe_spawn_connection_helper(HANDLE               pipe,
                                   HANDLE               err_handle,
                                   PROCESS_INFORMATION *pi,
                                   std::wstring const  &pipe_path,
                                   std::wstring const  &argv)
{
      SECURITY_ATTRIBUTES sec_attr = {
          .nLength              = sizeof(SECURITY_ATTRIBUTES),
          .lpSecurityDescriptor = nullptr,
          .bInheritHandle       = TRUE,
      };

      if (!SetHandleInformation(pipe, HANDLE_FLAG_INHERIT, 0))
            util::win32::error_exit(L"Named pipe SetHandleInformation");


      HANDLE client_pipe =
          CreateFileW(pipe_path.c_str(), GENERIC_READ | GENERIC_WRITE | WRITE_DAC,
                      /* FILE_SHARE_READ | FILE_SHARE_WRITE */ 0, &sec_attr,
                      OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

      if (client_pipe == INVALID_HANDLE_VALUE)
            util::win32::error_exit(L"CreateFileW");

      memset(pi, 0, sizeof(PROCESS_INFORMATION));
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

      if (!CreateProcessW(nullptr, const_cast<wchar_t *>(argv.c_str()),
                          nullptr, nullptr, true, 0, nullptr,
                          nullptr, &start_info, pi))
            util::win32::error_exit(L"CreateProcess() failed");

      //CloseHandle(client_pipe);
}

procinfo_t
win32_named_pipe_impl::do_spawn_connection(size_t const argc, char **argv)
{
      throw_if_initialized(typeid(*this));

      {
            char buf[PATH_MAX + 1];
            size_t const len = braindead_tempname(buf, R"(\\.\pipe)", MAIN_PROJECT_NAME "-", "-io_pipe");
            pipe_fname_ = {buf, len};
      }

      auto const wide_fname = util::unistring::charconv<wchar_t>(pipe_fname_);

      pipe_ = CreateNamedPipeW(
           wide_fname.c_str(),
           PIPE_ACCESS_DUPLEX | WRITE_DAC | FILE_FLAG_FIRST_PIPE_INSTANCE,
           PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_NOWAIT,
           1,
           65536,     // outbound buffer
           65536,     // inbound buffer
           0, nullptr // Default wait time and security attrs.
      );

      fmt::print(stderr, FC("Using pipe at `{}' with raw value `{}'.\n"), pipe_fname_, pipe_);
      std::wcerr << L"Wide name: "sv << wide_fname << L'\n';
      fflush(stderr);
      std::wcerr.flush();

      PROCESS_INFORMATION pi;
      auto const safe_argv  = win32::protect_argv(static_cast<int>(argc), argv);
      HANDLE     err_handle = get_err_handle();
      err_handle            = err_handle ? err_handle : GetStdHandle(STD_ERROR_HANDLE);

      named_pipe_spawn_connection_helper(pipe_, err_handle, &pi, wide_fname, safe_argv);

      if (err_sink_type_ != sink_type::DEFAULT && err_handle)
            CloseHandle(err_handle);

      set_to_initialized();
      return pi;
}

ssize_t
win32_named_pipe_impl::read(void *buf, ssize_t const nbytes, int const flags)
{
      ssize_t total = 0;
      DWORD  n      = 0;
      int    ret;

      mtx_.lock();

      if (flags & MSG_WAITALL) {
            do {
                  //ret = ReadFile(pipe_, static_cast<char *>(buf) + total, nbytes - total, &n, &overlapped_);;
                  ret = ReadFile(pipe_, static_cast<char *>(buf) + total, nbytes - total, &n, nullptr);;
            } while (ret && n != UINT32_C(-1) && (total += n) < nbytes);
      } else {
            do {
                  //ret = ReadFile(pipe_, buf, nbytes, &n, &overlapped_);
                  ret = ReadFile(pipe_, buf, nbytes, &n, nullptr);
            } while (ret >=0 && n == 0);
            total = n;
      }

      AUTOC e = GetLastError();
      //if (!ret && e != ERROR_MORE_DATA)
      if (!ret || e)
            util::win32::error_exit_explicit(L"ReadFile", e);

      mtx_.unlock();
      return total;
}

ssize_t
win32_named_pipe_impl::write(void const *buf, ssize_t const nbytes, int const flags)
{
      ssize_t total = 0;
      DWORD   n     = 0;
      int     ret;

      mtx_.lock();

      do {
            //ret = WriteFile(pipe_, static_cast<char const *>(buf) + total, nbytes - total, &n, &overlapped_);;
            ret = WriteFile(pipe_, static_cast<char const *>(buf) + total, nbytes - total, &n, nullptr);;
      } while (ret >= 0 && n != UINT32_C(-1) && (total += n) < nbytes);

      mtx_.unlock();
      total = n;
      return total;
}

# endif
#endif


/****************************************************************************************/
} // namespace ipc::detail
} // namespace emlsp
