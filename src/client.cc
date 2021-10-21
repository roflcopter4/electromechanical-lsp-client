#include "Common.hh"
#include "client.hh"
#include "util/exceptions.hh"

#ifdef _WIN32
  constexpr char const dev_null[] = "NUL";
#else
# include <glib.h>
  constexpr char const dev_null[] = "/dev/null";
#endif
#ifndef O_BINARY
# define O_BINARY 0
#endif

namespace emlsp::rpc {
namespace detail {
/****************************************************************************************/

#ifdef _WIN32
namespace win32 {

# define READ_FD     0
# define WRITE_FD    1
# define INPUT_PIPE  0
# define OUTPUT_PIPE 1

static int
start_process_with_pipe(UNUSED char const   *exe,
                        char                *argv,
                        int                  pipefds[2],
                        HANDLE const         err_handle,
                        PROCESS_INFORMATION *pi)
{
      HANDLE              handles[2][2] = {};
      STARTUPINFOA        start_info    = {};
      SECURITY_ATTRIBUTES sec_attr      = {
          .nLength              = sizeof(SECURITY_ATTRIBUTES),
          .lpSecurityDescriptor = nullptr,
          .bInheritHandle       = TRUE,
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
      pipefds[READ_FD]  = _open_osfhandle(reinterpret_cast<intptr_t>(handles[OUTPUT_PIPE][READ_FD]),  _O_RDONLY);

      if (!CreateProcessA(nullptr, argv, nullptr, nullptr, TRUE, 0, nullptr, nullptr, &start_info, pi))
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
      std::string path;
      std::string cmdline;
      socket_t    main_sock;
      socket_t    connected_sock;
      socket_t    accepted_sock;
      HANDLE      err_handle;
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
static std::string
protect_argv(int const argc, char **argv)
{
      std::string str;

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
                  if (*p == ' ' || *p == '\t')
                        need_dblquotes = true;
                  ++p;
            }

            p = argv[i];

            if (need_dblquotes)
                  str += '"';

            /* Only quotes and backslashes preceding quotes are escaped:
             * see "Parsing C Command-Line Arguments" at
             * https://docs.microsoft.com/en-us/cpp/c-language/parsing-c-command-line-arguments */
            while (*p) {
                  if (*p == '"') {
                        /* Add backslash for escaping quote itself */
                        str += '\\';
                        /* Add backslash for every preceding backslash for escaping it */
                        for (; pre_bslash > 0; --pre_bslash)
                              str += '\\';
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
                        str += '\\';
                  str += '"';
            }

            str += ' ';
      }

      return str;
}

NORETURN static int
start_process_with_pipe_wrapper(void *varg)
{
      auto *info = static_cast<socket_info *>(varg);

      try {
            info->connected_sock = util::rpc::connect_to_socket(info->path.c_str());
      } catch (std::exception &e) {
            std::cerr << "Caught exception \"" << e.what()
                      << "\" while attempting to connect to socket at "
                      << info->path << std::endl;
            thrd_exit(WSAGetLastError());
      }

      STARTUPINFOA startinfo = {};
      startinfo.cb           = sizeof(startinfo);
      startinfo.dwFlags      = STARTF_USESTDHANDLES;
      startinfo.hStdInput    = reinterpret_cast<HANDLE>(info->connected_sock); //NOLINT
      startinfo.hStdOutput   = reinterpret_cast<HANDLE>(info->connected_sock); //NOLINT
      startinfo.hStdError    = info->err_handle;

      if (!SetHandleInformation(startinfo.hStdError, HANDLE_FLAG_INHERIT, 1))
            util::win32::error_exit(L"Stderr SetHandleInformation");

      bool b = CreateProcessA(nullptr, const_cast<LPSTR>(info->cmdline.c_str()),
                              nullptr, nullptr, TRUE,
                              0, nullptr, nullptr,
                              &startinfo, &info->pid);
      auto const error = GetLastError();
      thrd_exit(static_cast<int>(b ? 0LU : error));
}

} // namespace win32

HANDLE
base_connection_interface::get_err_handle() const
{
      HANDLE err_handle;

      switch (err_sink_type_) {
      case sink_type::DEFAULT:
            err_handle = GetStdHandle(STD_ERROR_HANDLE);
            break;
      case sink_type::DEVNULL:
            err_handle = CreateFileA(dev_null, GENERIC_WRITE, 0, nullptr, OPEN_EXISTING,
                                     FILE_ATTRIBUTE_NORMAL, nullptr);
            break;
      case sink_type::FILENAME:
            err_handle = CreateFileA(fname_.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                                     FILE_ATTRIBUTE_NORMAL, nullptr);
            break;
      case sink_type::FD:
            throw except::not_implemented("FIXME");
      default:
            throw std::logic_error(fmt::format(FMT_COMPILE(
                  "Invalid value for err_sink_type_ {}"), err_sink_type_));
      }

      return err_handle;
}

#endif // defined _WIN32

/****************************************************************************************/
/* Unix Socket Connection */

procinfo_t
unix_socket_connection_impl::do_spawn_connection(UNUSED size_t const argc, char **argv)
{
      procinfo_t pid;
      path_ = util::get_temporary_filename("sock.").string();
      root_ = open_new_socket();

#ifdef _WIN32
      auto info = win32::socket_info{path_, win32::protect_argv(argc, argv),
                                     root_, {}, {}, get_err_handle(), {}};

      thrd_t start_thread{};
      thrd_create(&start_thread, win32::start_process_with_pipe_wrapper, &info);

      sockaddr_un con_addr{};
      int         len            = sizeof(con_addr);
      auto const  connected_sock = ::accept(root_, reinterpret_cast<sockaddr *>(&con_addr), &len);

      if (!connected_sock || connected_sock == static_cast<uintptr_t>(-1))
            err(1, "accept");

      int e;
      thrd_join(start_thread, &e);
      if (e != 0)
            err(e, "Process creation failed.");
      if (err_sink_type_ != sink_type::DEFAULT)
            CloseHandle(info.err_handle);

      pid = info.pid;
      connected_ = info.connected_sock;

#else // Not Windows

      if ((pid = fork()) == 0) {
            socket_t dsock = connect_to_socket();
            ::dup2(dsock, 0);
            ::dup2(dsock, 1);
            ::close(dsock);
            assert(argv[0] != nullptr);

            int fd = (-1);
            switch (err_sink_type_) {
            case sink_type::DEVNULL:
                  fd = ::open(dev_null, O_WRONLY|O_APPEND|O_BINARY);
                  break;
            case sink_type::FILENAME:
                  fd = ::open(fname_.c_str(), O_WRONLY|O_CREAT|O_TRUNC|O_BINARY, 0644);
                  break;
            default:
                  break;
            }
            if (fd > 0) {
                  ::dup2(fd, 2);
                  ::close(fd);
            }

            ::execvp(argv[0], const_cast<char **>(argv));
            err(1, "exec failed");
      }

      socket_t connected_sock;
      {
            sockaddr_un con_addr{};
            socklen_t len = sizeof(con_addr);
            connected_sock = ::accept(root_, reinterpret_cast<sockaddr *>(&con_addr), &len);
            if (connected_sock == (-1))
                  err(1, "accept");
      }
#endif

      accepted_ = connected_sock;
      return pid;
}

socket_t
unix_socket_connection_impl::open_new_socket()
{
      ::memcpy(addr_.sun_path, path_.c_str(), path_.length() + 1);
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

      auto const ret =
          ::bind(connection_sock, reinterpret_cast<sockaddr *>(&addr_), sizeof addr_);
      if (ret == (-1))
           err(1, "bind");
      if (::listen(connection_sock, 0) == (-1))
           err(1, "listen");

      return connection_sock;
}

socket_t
unix_socket_connection_impl::connect_to_socket()
{
      socket_t const data_sock = ::socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);

#ifdef _WIN32
      if (auto const e = ::WSAGetLastError(); e != 0)
            err(1, "socket(): %d", e);
#else
      if (data_sock == (-1))
           err(1, "socket");
      int tmp = ::fcntl(data_sock, F_GETFL);
      ::fcntl(data_sock, F_SETFL, tmp | O_CLOEXEC);
#endif

      auto const ret =
          ::connect(data_sock, reinterpret_cast<sockaddr const *>(&addr_), sizeof addr_);
      if (ret == (-1))
            err(1, "connect");

      return data_sock;
}

/*--------------------------------------------------------------------------------------*/
/* Pipe Connection */

#ifdef _WIN32

procinfo_t
pipe_connection_impl::do_spawn_connection(size_t const argc, char **argv)
{
      int pipefds[2];
      procinfo_t pid{};

      HANDLE     err_handle = get_err_handle();
      auto const cmdline    = win32::protect_argv(static_cast<int>(argc), argv);

      win32::start_process_with_pipe(argv[0], const_cast<char *>(cmdline.c_str()),
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

procinfo_t
pipe_connection_impl::do_spawn_connection(UNUSED size_t const argc, char **argv)
{
      GError *gerr = nullptr;
      GPid    pid;
      int flags = GSpawnFlags(G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD);

      //int fd = (-1);

      switch (err_sink_type_) {
      case sink_type::DEVNULL:
            flags |= G_SPAWN_STDERR_TO_DEV_NULL;
            break;
      case sink_type::FILENAME:
            //fd = open(fname_.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, 0644);
            break;
      default:
            break;
      }

      ::g_spawn_async_with_pipes(
             nullptr,
             const_cast<char **>(argv),
             environ,
             static_cast<GSpawnFlags>(flags),
             nullptr,
             nullptr,
             &pid,
             &write_,
             &read_,
             nullptr,
             &gerr
      );

      if (gerr != nullptr)
            throw std::runtime_error(fmt::format(FMT_COMPILE("glib error: {}"), gerr->message));

      return (procinfo_t)pid;
}

#endif

/*--------------------------------------------------------------------------------------*/

#ifdef _WIN32

namespace win32 {

struct named_pipe_info {
      HANDLE      base_handle;
      HANDLE      err_handle;
      char const *path;
      std::string argv;
      PROCESS_INFORMATION pi;
};

static __inline DWORD
start_process_with_named_pipe(char                *argv,
                              char const          *pipe_path,
                              HANDLE const         pipe_handle,
                              HANDLE const         err_handle,
                              PROCESS_INFORMATION *pi)
{
      SECURITY_ATTRIBUTES sec_attr  = {
            .nLength = sizeof(SECURITY_ATTRIBUTES),
            .lpSecurityDescriptor = nullptr,
            .bInheritHandle = TRUE,
      };

      if (!SetHandleInformation(pipe_handle, HANDLE_FLAG_INHERIT, 0))
            util::win32::error_exit(L"Named pipe SetHandleInformation");

      HANDLE connection = CreateFileA(pipe_path,
                                      GENERIC_READ | GENERIC_WRITE,
                                      FILE_SHARE_READ | FILE_SHARE_WRITE,
                                      &sec_attr,
                                      OPEN_EXISTING,
                                      FILE_ATTRIBUTE_NORMAL,
                                      nullptr);

      if (connection == INVALID_HANDLE_VALUE)
            util::win32::error_exit(L"CreateFileA");

      STARTUPINFOA start_info;
      memset(&start_info, 0, sizeof start_info);
      memset(pi, 0, sizeof *pi);
      start_info.cb           = sizeof(STARTUPINFOA);
      start_info.dwFlags      = STARTF_USESTDHANDLES;
      start_info.hStdInput    = connection;
      start_info.hStdOutput   = connection;
      start_info.hStdError    = err_handle ? err_handle : GetStdHandle(STD_ERROR_HANDLE);

      if (!SetHandleInformation(connection, HANDLE_FLAG_INHERIT, 1))
            util::win32::error_exit(L"Named Pipe SetHandleInformation");
      if (!SetHandleInformation(start_info.hStdError, HANDLE_FLAG_INHERIT, 1))
            util::win32::error_exit(L"Stderr SetHandleInformation");

      if (!CreateProcessA(nullptr, argv, nullptr, nullptr, TRUE, 0, nullptr, nullptr, &start_info, pi))
            util::win32::error_exit(L"CreateProcess() failed");

      return 0;
}

static int
start_process_with_named_pipe_wrapper(void *vdata)
{
      auto *info = static_cast<named_pipe_info *>(vdata);
      auto const ret =
          start_process_with_named_pipe(const_cast<char *>(info->argv.c_str()), info->path,
                                        info->base_handle, info->err_handle, &info->pi);

      std::cerr << "Process (" << info->pi.dwProcessId << ") created successfully.\n";
      std::cerr.flush();

      thrd_exit(static_cast<int>(ret));
}

} // namespace win32

procinfo_t
win32_named_pipe_impl::do_spawn_connection(size_t const argc, char **argv)
{
      {
            char buf[PATH_MAX + 1];
            fname_ = braindead_tempname(buf, R"(\\.\pipe)", MAIN_PROJECT_NAME ".", ".io_pipe");
      }

      pipe_ = CreateNamedPipeA(
           fname_.c_str(),
           PIPE_ACCESS_DUPLEX,
           PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
           1,
           4096,      // outbound buffer
           4096,      // inbound buffer
           0, nullptr // Default wait time and security attrs.
      );

      std::cerr << "Using pipe at " << fname_ << " with raw value " << pipe_ << '\n';
      std::cerr.flush();

      win32::named_pipe_info info = {pipe_,
                                     get_err_handle(),
                                     fname_.c_str(),
                                     win32::protect_argv(static_cast<int>(argc), argv),
                                     {}};

      thrd_t start_thread{};
      thrd_create(&start_thread, win32::start_process_with_named_pipe_wrapper, &info);

      if (!ConnectNamedPipe(pipe_, nullptr))
            err(1, "ConnectNamedPipe()");

      int e;
      thrd_join(start_thread, &e);
      if (e != 0)
            err(e, "Process creation failed. -> %d", e);
      if (err_sink_type_ != sink_type::DEFAULT)
            CloseHandle(info.err_handle);

      return info.pi;
}

#endif


/****************************************************************************************/
} // namespace detail
} // namespace emlsp::rpc
