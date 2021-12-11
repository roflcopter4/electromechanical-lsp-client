#include "Common.hh"
#include "ipc/base_client.hh"
#include "ipc/base_connection.hh"

inline namespace emlsp {
namespace ipc::detail {

void loop_signal_cb(UNUSED uv_signal_t *handle, int const signum)
{
      switch (signum) {
      case SIGTERM:
            fflush(stderr);
            quick_exit(0);
      case SIGHUP:
      case SIGINT:
      default:
            exit(0);
      }
}

} // namespace ipc::detail
} // namespace emlsp
