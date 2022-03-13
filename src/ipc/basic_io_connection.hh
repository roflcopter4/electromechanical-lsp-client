#pragma once
#ifndef HGUARD__IPC__BASIC_IO_CONNECTION_HH_
#define HGUARD__IPC__BASIC_IO_CONNECTION_HH_ //NOLINT
/****************************************************************************************/
#include "Common.hh"

#include "ipc/basic_connection.hh"
#include "ipc/basic_wrapper.hh"

inline namespace emlsp {
namespace ipc::rpc {
/****************************************************************************************/


template <typename Connection, template <typename> typename IOWrapper>
      REQUIRES(ipc::IsBasicConnectionVariant<Connection>;
               rpc::IsIOWrapperVariant<IOWrapper<Connection>>;)
class basic_io_connection : public Connection,
                            public IOWrapper<Connection>
{
      using this_type       = basic_io_connection<Connection, IOWrapper>;
      using connection_type = Connection;
      using io_wrapper_type = IOWrapper<Connection>;
      using io_packer_type  = typename io_wrapper_type::packer_type;

    public:
      basic_io_connection()
          : Connection(), IOWrapper<Connection>(dynamic_cast<Connection *>(this))
      {}

      ~basic_io_connection() override = default;

      DELETE_COPY_CTORS(basic_io_connection);
      DEFAULT_MOVE_CTORS(basic_io_connection);

    private:
      class my_cond
      {
            std::mutex              mtx_{};
            std::condition_variable cnd_{};

          public:
            my_cond() = default;

            void wait()
            {
                  auto lock = std::unique_lock{mtx_};
                  cnd_.wait(lock);
            }

            void notify_one() { cnd_.notify_one(); }
            void notify_all() { cnd_.notify_all(); }
      };

      my_cond cond_{};

    public:
      void wait()       { cond_.wait(); }
      void notify_one() { cond_.notify_one(); }
      void notify_all() { cond_.notify_all(); }
};


/****************************************************************************************/
} // namespace ipc::rpc
} // namespace emlsp
#endif
