#include "Common.hh"
#include "ipc/connection_impl.hh"
#include "util/exceptions.hh"

#ifndef WEXITSTATUS
#  define WEXITSTATUS(status) (((status)&0xFF00) >> 8)
#endif

inline namespace emlsp {
namespace ipc::detail {
/****************************************************************************************/

int
kill_impl(procinfo_t const &pid)
{
      int status = 0;

#ifdef _WIN32
      if (pid.hProcess) {
            ::TerminateProcess(pid.hProcess, 0);
            ::WaitForSingleObject(pid.hProcess, INFINITE);
            ::GetExitCodeProcess(pid.hProcess, reinterpret_cast<DWORD *>(&status));
            ::CloseHandle(pid.hThread);
            ::CloseHandle(pid.hProcess);
      }
#else // not _WIN32
      if (pid) {
            ::kill(pid, SIGTERM);
            ::waitpid(pid, &status, 0);
            status = WEXITSTATUS(status);
      }
#endif

      return status;
}

/****************************************************************************************/
} // namespace ipc::detail
} // namespace emlsp
