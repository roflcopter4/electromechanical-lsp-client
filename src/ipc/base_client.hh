#pragma once
#ifndef HGUARD__IPC__BASE_CLIENT_HH_
#define HGUARD__IPC__BASE_CLIENT_HH_ //NOLINT

#include "Common.hh"
#include "ipc/base_connection.hh"

#include <event2/event.h>
#include <uv.h>

#include "ipc/base_wait_loop.hh"

inline namespace emlsp {
namespace ipc {
/****************************************************************************************/


template <typename LoopVariant, typename ConnectionType, typename MsgType, typename UserDataType = void *>
      REQUIRES (IsConnectionVariant<ConnectionType>)
class base_client : public ConnectionType
{
      using this_type       = base_client<LoopVariant, ConnectionType, MsgType, UserDataType>;
      using base_type       = ConnectionType;
      using user_data_type  = UserDataType;
      using event_loop_type = base_loop<LoopVariant, ConnectionType, MsgType, UserDataType>;
      using atomic_queue    = std::queue<std::atomic<MsgType *>>;

      UserDataType user_data_{};

    public:
      using loop_variant    = LoopVariant;
      using connection_type = ConnectionType;
      using message_type    = MsgType;

    protected:
      atomic_queue    queue_{};
      event_loop_type loop_{this};

      std::mutex                   mtx_ {};
      std::condition_variable      condition_ {};
      std::unique_lock<std::mutex> unique_lock_ {mtx_};

      this_type &get_self() & { return *this; }

    public:
      base_client() = default;
      ~base_client() override
      {
            loop_.loopbreak();
            this->kill(true);
      }

      DELETE_COPY_CTORS(base_client);
      DEFAULT_MOVE_CTORS(base_client);

      ND auto &userdata()    & { return user_data_; }
      ND auto &loop()        & { return loop_; }
      ND auto &mtx()         & { return mtx_; }
      ND auto &unique_lock() & { return unique_lock_; }
      ND auto &condition()   & { return condition_; }

      void cond_notify_one() { condition_.notify_one(); }
      void cond_wait()       { condition_.wait(unique_lock_); }
};


/****************************************************************************************/
} // namespace ipc
} // namespace emlsp
#endif // base_client.hh
