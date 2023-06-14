#include "Common.hh"
#include "util/util.hh"


inline namespace MAIN_PACKAGE_NAMESPACE {
namespace util {
/****************************************************************************************/


namespace {

struct signal_to_string_s {
      std::string_view name;
      std::string_view explanation;
};

std::map<int, signal_to_string_s> signal_to_string = {
#ifdef SIGINT
    {SIGINT,    {"SIGINT", "Interactive attention signal."}          },
#endif
#ifdef SIGILL
    {SIGILL,    {"SIGILL", "Illegal instruction."}                   },
#endif
#ifdef SIGABRT
    {SIGABRT,   {"SIGABRT", "Abnormal termination."}                 },
#endif
#ifdef SIGFPE
    {SIGFPE,    {"SIGFPE", "Erroneous arithmetic operation."}        },
#endif
#ifdef SIGSEGV
    {SIGSEGV,   {"SIGSEGV", "Invalid access to storage."}            },
#endif
#ifdef SIGTERM
    {SIGTERM,   {"SIGTERM", "Termination request."}                  },
#endif
#ifdef SIGHUP
    {SIGHUP,    {"SIGHUP", "Hangup."}                                },
#endif
#ifdef SIGQUIT
    {SIGQUIT,   {"SIGQUIT", "Quit."}                                 },
#endif
#ifdef SIGTRAP
    {SIGTRAP,   {"SIGTRAP", "Trace/breakpoint trap."}                },
#endif
#ifdef SIGKILL
    {SIGKILL,   {"SIGKILL", "Killed."}                               },
#endif
#ifdef SIGPIPE
    {SIGPIPE,   {"SIGPIPE", "Broken pipe."}                          },
#endif
#ifdef SIGALRM
    {SIGALRM,   {"SIGALRM", "Alarm clock."}                          },
#endif
#ifdef SIGSTKFLT
    {SIGSTKFLT, {"SIGSTKFLT", "Stack fault (obsolete)."}             },
#endif
#ifdef SIGPWR
    {SIGPWR,    {"SIGPWR", "Power failure imminent."}                },
#endif
#ifdef SIGBUS
    {SIGBUS,    {"SIGBUS", "Bus error."}                             },
#endif
#ifdef SIGSYS
    {SIGSYS,    {"SIGSYS", "Bad system call."}                       },
#endif
#ifdef SIGURG
    {SIGURG,    {"SIGURG", "Urgent data is available at a socket."}  },
#endif
#ifdef SIGSTOP
    {SIGSTOP,   {"SIGSTOP", "Stop, unblockable."}                    },
#endif
#ifdef SIGTSTP
    {SIGTSTP,   {"SIGTSTP", "Keyboard stop."}                        },
#endif
#ifdef SIGCONT
    {SIGCONT,   {"SIGCONT", "Continue."}                             },
#endif
#ifdef SIGCHLD
    {SIGCHLD,   {"SIGCHLD", "Child terminated or stopped."}          },
#endif
#ifdef SIGTTIN
    {SIGTTIN,   {"SIGTTIN", "Background read from control terminal."}},
#endif
#ifdef SIGTTOU
    {SIGTTOU,   {"SIGTTOU", "Background write to control terminal."} },
#endif
#ifdef SIGPOLL
    {SIGPOLL,   {"SIGPOLL", "Pollable event occurred (System V)."}   },
#endif
#ifdef SIGXFSZ
    {SIGXFSZ,   {"SIGXFSZ", "File size limit exceeded."}             },
#endif
#ifdef SIGXCPU
    {SIGXCPU,   {"SIGXCPU", "CPU time limit exceeded."}              },
#endif
#ifdef SIGVTALRM
    {SIGVTALRM, {"SIGVTALRM", "Virtual timer expired."}              },
#endif
#ifdef SIGPROF
    {SIGPROF,   {"SIGPROF", "Profiling timer expired."}              },
#endif
#ifdef SIGUSR1
    {SIGUSR1,   {"SIGUSR1", "User-defined signal 1."}                },
#endif
#ifdef SIGUSR2
    {SIGUSR2,   {"SIGUSR2", "User-defined signal 2."}                },
#endif
#ifdef SIGWINCH
    {SIGWINCH,  {"SIGWINCH", "Window size change (4.3 BSD, Sun)."}   },
#endif
};

} // namespace


std::string_view const &
get_signal_name(int const signum)
{
      return signal_to_string[signum].name;
}

std::string_view const &
get_signal_explanation(int const signum)
{
      return signal_to_string[signum].explanation;
}


/****************************************************************************************/
} // namespace util
} // namespace MAIN_PACKAGE_NAMESPACE
