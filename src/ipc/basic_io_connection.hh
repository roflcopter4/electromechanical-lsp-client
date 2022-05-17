#pragma once
#ifndef HGUARD__IPC__BASIC_IO_CONNECTION_HH_
#define HGUARD__IPC__BASIC_IO_CONNECTION_HH_ //NOLINT
/****************************************************************************************/
#include "Common.hh"

#include "ipc/basic_connection.hh"
#include "ipc/basic_dialer.hh"

#include "ipc/io/basic_io_wrapper.hh"
#include "ipc/io/ms_jsonrpc_io_wrapper.hh"
#include "ipc/io/msgpack_io_wrapper.hh"

inline namespace emlsp {
namespace ipc {
/****************************************************************************************/


template <typename Connection, template <class> typename IOWrapper>
      requires (BasicConnectionVariant<Connection> &&
                io::WrapperVariant<IOWrapper<Connection>>)
class basic_io_connection : public Connection,
                            public IOWrapper<Connection>
{
      using this_type       = basic_io_connection<Connection, IOWrapper>;
      using connection_type = Connection;
      using io_wrapper_type = IOWrapper<Connection>;
      using io_packer_type  = typename io_wrapper_type::packer_type;

    public:
      basic_io_connection() noexcept
          : Connection(), IOWrapper<Connection>(dynamic_cast<Connection *>(this))
      {}

      virtual ~basic_io_connection() override = default;

      basic_io_connection(basic_io_connection const &)                = delete;
      basic_io_connection &operator=(basic_io_connection const &)     = delete;
      basic_io_connection(basic_io_connection &&) noexcept            = default;
      basic_io_connection &operator=(basic_io_connection &&) noexcept = default;

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
                  // flag_.clear();
                  cnd_.wait(lock);
                  // flag_.test_and_set();
                  // flag_.wait(true);
                  return false;
            }

            template <typename Rep, typename Period>
            bool wait(std::chrono::duration<Rep, Period> const &dur)
            {
                  auto lock = std::unique_lock{mtx_};
                  auto ret  = cnd_.wait_for(lock, dur);
                  return ret == std::cv_status::timeout;
            }

            void notify_one() noexcept
            { 
                  cnd_.notify_one();
                  // do cnd_.notify_one();
                  // while (!flag_.test());
                  // flag_.clear();
                  // flag_.notify_one();
            }

            // void notify_one() noexcept { cnd_.notify_one(); }
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


namespace connections {

template <typename Connection>
using msgpack_connection = basic_io_connection<Connection, io::msgpack_wrapper>;

template <typename Connection>
using ms_jsonrpc_connection = basic_io_connection<Connection, io::ms_jsonrpc_wrapper>;

} // namespace connections


/****************************************************************************************/
} // namespace ipc
} // namespace emlsp
#endif
