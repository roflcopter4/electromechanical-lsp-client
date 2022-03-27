#pragma once
#ifndef HGUARD__IPC__RPC__LSP___LSP_CONNECTION_HH_
#define HGUARD__IPC__RPC__LSP___LSP_CONNECTION_HH_ //NOLINT
/****************************************************************************************/
#include "Common.hh"
#include "ipc/basic_rpc_connection.hh"

inline namespace emlsp {
namespace ipc::rpc {
/****************************************************************************************/


template <typename Connection>
      REQUIRES(ipc::IsBasicConnectionVariant<Connection>;)
class lsp_conection final
    : public ipc::basic_rpc_connection<Connection, ipc::io::ms_jsonrpc2_wrapper>
{
      using this_type = lsp_conection<Connection>;
      using base_type = ipc::basic_rpc_connection<Connection,
                                                  ipc::io::ms_jsonrpc2_wrapper>;

    public:
      using connection_type = Connection;
      using io_wrapper_type = ipc::io::ms_jsonrpc2_wrapper<Connection>;
      using value_type      = typename base_type::value_type;

    protected:
      lsp_conection() = default;

    public:
      ~lsp_conection() override = default;

      lsp_conection(lsp_conection const &)                = delete;
      lsp_conection(lsp_conection &&) noexcept            = delete;
      lsp_conection &operator=(lsp_conection const &)     = delete;
      lsp_conection &operator=(lsp_conection &&) noexcept = delete;

      //-------------------------------------------------------------------------------

      static auto new_unique() { return std::unique_ptr<this_type>(new this_type()); }
      static auto new_shared() { return std::shared_ptr<this_type>(new this_type()); }

      //-------------------------------------------------------------------------------

      int notification() override
      {
            return 0;
      }

      int request() override
      {
            return 0;
      }

      int response() override
      {
            return 0;
      }

      int error_response() override
      {
            return 0;
      }

      ND uv_poll_cb  poll_callback()  const override { return poll_callback_wrapper; }
      ND uv_timer_cb timer_callback() const override { return [](uv_timer_t *) {}; }

      ND void *data() override
      {
            return this;
      }


    private:
      static void poll_callback_wrapper(uv_poll_t *handle, int status, int events)
      {
            this_type *self = static_cast<this_type *>(handle->data);
            self->real_poll_callback(handle, status, events);
      }

      void
      real_poll_callback(UNUSED uv_poll_t *handle, UNUSED int status, UNUSED int events)
      {

      }
};


/*======================================================================================*/



/****************************************************************************************/
} // namespace ipc::rpc
} // namespace emlsp
#endif
