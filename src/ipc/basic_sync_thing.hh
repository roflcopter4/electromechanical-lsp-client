#pragma once
#ifndef HGUARD__IPC__BASIC_SYNC_THING_HH_
#define HGUARD__IPC__BASIC_SYNC_THING_HH_ //NOLINT
/****************************************************************************************/
#include "Common.hh"

#include "ipc/io/basic_io_wrapper.hh"
#include "ipc/io/ms_jsonrpc_io_wrapper.hh"
#include "ipc/io/msgpack_io_wrapper.hh"

inline namespace emlsp {
namespace ipc {
/****************************************************************************************/


// template <typename Connection, template <class> typename IOWrapper>
      // REQUIRES (BasicConnectionVariant<Connection> &&
      //           io::WrapperVariant<IOWrapper<Connection>>)
class basic_sync_thing //: public IOWrapper<Connection>
{
      // using this_type       = io_sync_wrapper<Connection, IOWrapper>;
      // using connection_type = Connection;
      // using io_wrapper_type = IOWrapper<Connection>;
      // using io_packer_type  = typename io_wrapper_type::packer_type;

    public:
      // explicit io_sync_wrapper(connection_type *con)
      //     : IOWrapper<Connection>(con)
      // {}
      // ~io_sync_wrapper() override = default;

      basic_sync_thing()          = default;
      virtual ~basic_sync_thing() = default;

      DELETE_ALL_CTORS(basic_sync_thing);

    private:
      class my_cond
      {
            std::mutex              mtx_{};
            std::condition_variable cnd_{};
            std::atomic_uint cnt_ = 0;
            std::atomic_flag flag_{};

          public:
            my_cond() = default;

            bool wait()
            {
                  std::unique_lock<std::mutex> lock(mtx_);
                  cnd_.wait(lock);
                  return false;
            }

            template <typename Rep, typename Period>
            bool wait(std::chrono::duration<Rep, Period> const &dur)
            {
                  auto lock = std::unique_lock{mtx_};
                  auto ret  = cnd_.wait_for(lock, dur);
                  return ret == std::cv_status::timeout;
            }

            void notify_one() noexcept { cnd_.notify_one(); }
            void notify_all() noexcept { cnd_.notify_all(); }
      };

      my_cond cond_{};

    public:
      void notify_one() noexcept { cond_.notify_one(); }
      void notify_all() noexcept { cond_.notify_all(); }
      bool wait()                { return cond_.wait(); }

      template <typename Rep, typename Period>
      bool wait(std::chrono::duration<Rep, Period> const &dur)
      {
            return cond_.wait(dur);
      }
};


#if 0
namespace connections {

template <typename Connection>
using msgpack_connection = io_sync_wrapper<Connection, io::msgpack_wrapper>;

template <typename Connection>
using ms_jsonrpc_connection = io_sync_wrapper<Connection, io::ms_jsonrpc_wrapper>;

} // namespace connections
#endif


/****************************************************************************************/
} // namespace ipc
} // namespace emlsp
#endif
