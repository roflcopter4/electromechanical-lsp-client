#include "Common.hh"
#include "client.hh"
#include "util/exceptions.hh"

#include <glib.h>

namespace emlsp {
namespace rpc {
namespace detail {
/****************************************************************************************/

#ifdef _WIN32
#define READ_FD  (0)
#define WRITE_FD (1)

static int
win32_start_process_with_pipe(char const *exe, char *argv,
                              int pipefds[2], PROCESS_INFORMATION *pi)
{
        HANDLE              handles[2][2];
        STARTUPINFOA        info;
        SECURITY_ATTRIBUTES attr = {.nLength = sizeof(SECURITY_ATTRIBUTES),
                                    .bInheritHandle = TRUE,
                                    .lpSecurityDescriptor = NULL};

        if (!CreatePipe(&handles[0][READ_FD], &handles[0][WRITE_FD], &attr, 0)) 
                win32_error_exit(1, "CreatePipe()", GetLastError());
        if (!CreatePipe(&handles[1][READ_FD], &handles[1][WRITE_FD], &attr, 0)) 
                win32_error_exit(1, "CreatePipe()", GetLastError());
        if (!SetHandleInformation(handles[0][WRITE_FD], HANDLE_FLAG_INHERIT, 0))
                win32_error_exit(1, "Stdin SetHandleInformation", GetLastError());
        if (!SetHandleInformation(handles[1][READ_FD], HANDLE_FLAG_INHERIT, 0))
                win32_error_exit(1, "Stdout SetHandleInformation", GetLastError());

        memset(&info, 0, sizeof(STARTUPINFOA));
        memset(pi, 0, sizeof(PROCESS_INFORMATION));

        info.cb         = sizeof(STARTUPINFOA);
        info.dwFlags    = STARTF_USESTDHANDLES;
        info.hStdInput  = handles[0][READ_FD];
        info.hStdOutput = handles[1][WRITE_FD];
        info.hStdError  = GetStdHandle(STD_ERROR_HANDLE);

        if (!SetHandleInformation(info.hStdError, HANDLE_FLAG_INHERIT, 1))
                win32_error_exit(1, "Stderr SetHandleInformation", GetLastError());

        if (!CreateProcessA(exe, argv, NULL, NULL, TRUE, 0, NULL, NULL, &info, pi))
                win32_error_exit(1, "CreateProcess() failed", GetLastError());
        CloseHandle(handles[0][READ_FD]);
        CloseHandle(handles[1][WRITE_FD]);

        /* _open_osfhandle() ought to transfer ownership of the handle to the file
         * descriptor. The original HANDLE isn't leaked when it goes out of scope. */
        pipefds[WRITE_FD] = _open_osfhandle(handles[0][WRITE_FD], 0);
        pipefds[READ_FD]  = _open_osfhandle(handles[1][READ_FD], 0);
        return 0;
}

struct socket_info {
      std::string path;
      std::string cmdline;
      socket_t    main_sock;
      socket_t    connected_sock;
      socket_t    accepted_sock;
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
win32_protect_argv(int argc, char **argv)
{
      std::stringstream ss;

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
                  ss << '"';

            /* Only quotes and backslashes preceding quotes are escaped:
             * see "Parsing C Command-Line Arguments" at
             * https://docs.microsoft.com/en-us/cpp/c-language/parsing-c-command-line-arguments */
            while (*p) {
                  if (*p == '"') {
                        /* Add backslash for escaping quote itself */
                        ss << '\\';
                        /* Add backslash for every preceding backslash for escaping it */
                        for (; pre_bslash > 0; --pre_bslash)
                              ss << '\\';
                  }

                  /* Count length of continuous sequence of preceding backslashes. */
                  if (*p == '\\')
                        ++pre_bslash;
                  else
                        pre_bslash = 0;

                  ss << *p++;
            }

            if (need_dblquotes) {
                  /* Add backslash for every preceding backslash for escaping it,
                   * do NOT escape quote itself. */
                  for (; pre_bslash > 0; --pre_bslash)
                        ss << '\\';
                  ss << '"';
            }

            ss << ' ';
      }

      return ss.str();
}

NORETURN static int
process_startup_thread(void *varg)
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

      STARTUPINFOA startinfo;
      memset(&startinfo, 0, sizeof(startinfo));
      startinfo.dwFlags    = STARTF_USESTDHANDLES;
      startinfo.hStdInput  = reinterpret_cast<HANDLE>(info->connected_sock); //NOLINT
      startinfo.hStdOutput = reinterpret_cast<HANDLE>(info->connected_sock); //NOLINT

      bool b = CreateProcessA(nullptr, cmdline.c_str(), nullptr, nullptr, true,
                              CREATE_NEW_CONSOLE, nullptr, nullptr,
                              &startinfo, &info->pid);
      auto const error = GetLastError();
      thrd_exit(static_cast<int>(b ? 0LU : error));
}
#endif

/*--------------------------------------------------------------------------------------*/

#ifdef NDEBUG
# undef NDEBUG
#endif
#include <cassert>

procinfo_t
unix_socket_connection_impl::do_spawn_connection(UNUSED size_t const argc, char **argv)
{
      procinfo_t pid;
      path_ = util::get_temporary_filename().string();
      root_ = open_new_socket();

      std::cerr << fmt::format(FMT_COMPILE("(Socket bound to \"{}\" with raw value ({}).\n"),
                               path_, root_);

#ifdef _WIN32
      auto info = socket_info{tmppath.string(), win32_protect_argv(argc, argv)
                              sock, {}, {}, {}};
      thrd_t start_thread{};
      thrd_create(&start_thread, process_startup_thread, &info);

      auto const connected_sock = accept(sock, nullptr, nullptr);

      int e;
      thrd_join(start_thread, &e);
      if (e != 0)
            err(e, "Process creation failed.");

      pid = info.pid;
      connected_ = info.connected_sock;
#else
      std::cerr << "Going to fork-exec with argv of size " << argc << '\n';
      for (char **s = argv; *s; ++s)
            std::cerr << *s << '\n';

      if ((pid = fork()) == 0) {
            socket_t dsock = connect_to_socket();
            ::dup2(dsock, 0);
            ::dup2(dsock, 1);
            ::close(dsock);
            assert(argv[0] != nullptr);
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
      if (auto const e = ::GetLastError(); e != 0)
           err(1, "socket");
#else
      if (connection_sock == (-1))
           err(1, "socket");
      int tmp = ::fcntl(connection_sock, F_GETFL);
      ::fcntl(connection_sock, F_SETFL, tmp | O_CLOEXEC);
#endif

      auto const ret =
          ::bind(connection_sock, reinterpret_cast<sockaddr *>(&addr_), sizeof(addr_));
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
      if (auto const e = ::GetLastError(); e != 0)
            err(1, "socket");
#else
      if (data_sock == (-1))
           err(1, "socket");
      int tmp = ::fcntl(data_sock, F_GETFL);
      ::fcntl(data_sock, F_SETFL, tmp | O_CLOEXEC);
#endif

      auto const ret =
          ::connect(data_sock, reinterpret_cast<sockaddr const *>(&addr_), sizeof(addr_));
      if (ret == (-1))
            err(1, "connect");

      return data_sock;
}

/*--------------------------------------------------------------------------------------*/

procinfo_t
pipe_connection_impl::do_spawn_connection(UNUSED size_t const argc, char **argv)
{
      GError *gerr = nullptr;
      GPid    pid;

      ::g_spawn_async_with_pipes(
             nullptr,
             const_cast<char **>(argv),
             environ,
             GSpawnFlags(G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD),
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

      return pid;
}


/****************************************************************************************/
} // namespace detail
} // namespace rpc
} // namespace emlsp
