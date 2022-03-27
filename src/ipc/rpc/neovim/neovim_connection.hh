#pragma once
#ifndef HGUARD__IPC__RPC__NEOVIM___NEOVIM_CONNECTION_HH_
#define HGUARD__IPC__RPC__NEOVIM___NEOVIM_CONNECTION_HH_ // NOLINT
/****************************************************************************************/
#include "Common.hh"
#include "ipc/rpc/neovim.hh"
#include "msgpack/dumper.hh"

inline namespace emlsp {
namespace ipc::rpc {
/****************************************************************************************/


template <typename Connection>
      REQUIRES(ipc::IsBasicConnectionVariant<Connection>;)
class neovim_conection final
    : public ipc::basic_rpc_connection<Connection,
                                       ipc::io::msgpack_wrapper>
{
      using this_type = neovim_conection<Connection>;
      using base_type = ipc::basic_rpc_connection<Connection,
                                                  ipc::io::msgpack_wrapper>;

    public:
      using connection_type = Connection;
      using io_wrapper_type = ipc::io::msgpack_wrapper<Connection>;
      using value_type      = typename base_type::value_type;

      using io_wrapper_type::read_object;

    protected:
      neovim_conection() = default;

    public:
      ~neovim_conection() override = default;

      neovim_conection(neovim_conection const &)                = delete;
      neovim_conection(neovim_conection &&) noexcept            = delete;
      neovim_conection &operator=(neovim_conection const &)     = delete;
      neovim_conection &operator=(neovim_conection &&) noexcept = delete;

      //-------------------------------------------------------------------------------

      static auto new_unique()
      {
            return std::unique_ptr<this_type>(new this_type());
      }
      static auto new_shared()
      {
            return std::shared_ptr<this_type>(new this_type());
      }

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
      ND uv_timer_cb timer_callback() const override { return timer_callback_wrapper; }

      ND void *data() override
      {
            return this;
      }

    private:
      static void timer_callback_wrapper(uv_timer_t *timer)
      {
            this_type *self = static_cast<this_type *>(timer->data);
            self->true_timer_callback(timer);
      }

      static void poll_callback_wrapper(uv_poll_t *handle, int status, int events)
      {
            this_type *self = static_cast<this_type *>(handle->data);
            self->true_poll_callback(handle, status, events);
      }

      bool want_close_ = false;

      void
      true_poll_callback(UNUSED uv_poll_t *handle, UNUSED int status, UNUSED int events)
      {
            if (events & UV_DISCONNECT) {
                  uv_loop_t *base = handle->loop;

                  util::eprint(FC("Got disconnect signal, status {}\n"), status);
                  fflush(stderr);
                  // uv_poll_stop(handle);
                  // want_close_ = true;
                  auto *stopper = static_cast<std::function<void()> *>(base->data);
                  (*stopper)();
            } else if (events & UV_READABLE) {
                  msgpack::object_handle objhand = read_object();
                  // auto implicit = objhand->convert();
                  // util::eprint(FC("Got type {}\n"), typeid(implicit).name());

                  util::mpack::dumper(objhand.get(), std::cerr);
            } else {
                  util::eprint(FC("Unexpected event:  0x{:X}\n"), events);
            }

      /*
            if (events & UV_DISCONNECT) {
                  uv_poll_stop(handle);
            } else if (events & UV_READABLE) {
                  //if (uv_fileno((uv_handle_t const *)handle, &data.fd))
                  //      err(1, "uv_fileno()");

                  struct userdata *user = handle->data;
                  struct event_data data;
                  data.fd  = (int)user->fd;
                  data.obj = mpack_decode_stream(data.fd);
                  talloc_steal(CTX, data.obj);
                  handle_nvim_message(&data);

                  //struct event_data *data = calloc(1, sizeof(struct event_data));
                  //data->obj = talloc_steal(data, mpack_decode_stream(data->fd));
                  //START_DETACHED_PTHREAD(handle_nvim_message_wrapper, data);
                  //handle_nvim_message(data);
            }
      */
      }


      void
      true_timer_callback(uv_timer_t *timer)
      {
            uv_timer_stop(timer);

            // if (want_close_ && last_handle_) {
            //       uv_poll_stop(last_handle_);
            // }
      }
};


/*======================================================================================*/


/****************************************************************************************/
} // namespace ipc::rpc
} // namespace emlsp
#endif
