#pragma once
#ifndef HGUARD__IPC__PROTOCOLS__MSGPACK_CONNECTION_HH_
#define HGUARD__IPC__PROTOCOLS__MSGPACK_CONNECTION_HH_ // NOLINT
/****************************************************************************************/

#include "Common.hh"
#include "ipc/basic_protocol_connection.hh"

#include "msgpack/dumper.hh"

inline namespace emlsp {
namespace ipc::protocols::Msgpack {
/****************************************************************************************/


template <typename Connection>
      REQUIRES(ipc::BasicConnectionVariant<Connection>)
class alignas(4096) connection final
    : public ipc::basic_protocol_connection<Connection, ipc::io::msgpack_wrapper>
{
    public:
      using this_type = connection<Connection>;
      using base_type = ipc::basic_protocol_interface;

      using connection_type = Connection;
      using io_wrapper_type = ipc::io::msgpack_wrapper<Connection>;
      using value_type      = typename io_wrapper_type::value_type;

      using io_wrapper_type::read_object;
      using io_wrapper_type::get_unpacker;

    protected:
      connection() = default;

    public:
      virtual ~connection() override = default;

      connection(connection const &)                = delete;
      connection(connection &&) noexcept            = delete;
      connection &operator=(connection const &)     = delete;
      connection &operator=(connection &&) noexcept = delete;

      //-------------------------------------------------------------------------------

      NOINLINE static auto new_unique() { return std::unique_ptr<this_type>(new this_type()); }
      NOINLINE static auto new_shared() { return std::shared_ptr<this_type>(new this_type()); }

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

      ND constexpr uv_poll_cb  poll_callback()       const override { return poll_callback_wrapper; }
      ND constexpr uv_alloc_cb pipe_alloc_callback() const override { return alloc_callback_wrapper; }
      ND constexpr uv_read_cb  pipe_read_callback()  const override { return read_callback_wrapper; }
      ND constexpr uv_timer_cb timer_callback()      const override { return timer_callback_wrapper; }
      ND constexpr void const  *data()               const override { return this; }
      ND constexpr void        *data()                     override { return this; }

    private:
      std::mutex mtx_{};

      static void timer_callback_wrapper(uv_timer_t *timer)
      {
            auto *self = static_cast<this_type *>(timer->data);
            self->true_timer_callback(timer);
      }

      static void poll_callback_wrapper(uv_poll_t *handle, int status, int events)
      {
            auto *self = static_cast<this_type *>(handle->data);
            self->true_poll_callback(handle, status, events);
      }

      static void alloc_callback_wrapper(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf)
      {
            auto *self = static_cast<this_type *>(handle->data);
            self->true_alloc_callback(handle, suggested_size, buf);
      }

      static void read_callback_wrapper(uv_stream_t *handle, ssize_t nread, uv_buf_t const *buf)
      {
            auto *self = static_cast<this_type *>(handle->data);
            self->true_read_callback(handle, nread, buf);
      }

      //-------------------------------------------------------------------------------

      void
      true_poll_callback(uv_poll_t *handle, int status, int events)
      {
            std::lock_guard lock(mtx_);
            size_t          avail;

            util::eprint(
                FC("real_poll_callback called for descriptor {} (tag: {}) -- ({}, {})\n"),
                this->raw_descriptor(), this->get_key(), status, events);

            if (!(events & (UV_DISCONNECT | UV_READABLE))) {
                  util::eprint(FC("Unexpected event(s):  0x{:X}\n"), events);
                  return;
            }

            if (events & UV_READABLE && (avail = this->available()) > 0) {
                  try {
                        this->just_read();
                  } catch (std::exception const &e) {
                        DUMP_EXCEPTION(e);
                        return;
                  }

                  msgpack::object_handle obj;

                  while (get_unpacker().next(obj))
                        util::mpack::dumper(obj.get(), std::cout);
            }

            if (events & UV_DISCONNECT) {
                  util::eprint(FC("Got disconnect signal, status {}, for fd {}, key '{}' -- ({})\n"),
                               status, this->raw_descriptor(), this->get_key(), FUNCTION_NAME);

                  auto const &key_deleter = cast_deleter(handle->loop->data);
                  this->close();
                  key_deleter(this->get_key(), false);
            }
      }

      //-------------------------------------------------------------------------------

      void
      true_alloc_callback(UU uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf)
      {
            auto &unpacker = get_unpacker();
            if (unpacker.buffer_capacity() == 0)
                  unpacker.reserve_buffer(suggested_size);
            buf->base = unpacker.buffer();
            buf->len  = static_cast<decltype(buf->len)>(unpacker.buffer_capacity());
      }

      void
      true_read_callback(uv_stream_t *handle, ssize_t nread, UU uv_buf_t const *buf)
      {
            if (nread == 0)
                  return;
            if (nread < 0) {
                  auto const &key_deleter = cast_deleter(handle->loop->data);
                  util::eprint(
                      FC("({}): Got negative read (disconnect?), for {} aka '{}, of type {}. ({} -> '{}'?)\n"),
                      this->raw_descriptor(), static_cast<void const *>(handle),
                      this->get_key(), handle->type, nread,
                      uv_strerror(static_cast<int>(nread))
                  );
                  key_deleter(this->get_key(), false);
                  return;
            }
            util::eprint(FC("Read {} bytes...\n"), nread);

            auto  obj      = msgpack::object_handle{};
            auto &unpacker = get_unpacker();
            unpacker.buffer_consumed(nread);

            try {
                  while (unpacker.next(obj)) {
                        auto thrd = std::thread{[this]() { this->notify_one(); }};
                        thrd.detach();
                        util::mpack::dumper(obj.get(), std::cout);
                  }
            } catch (std::exception const &e) {
                  err_nothrow("fuck %s", e.what());
            }

            if (unpacker.nonparsed_size())
                  util::eprint(FC("Warning: Incomplete msgpack object read: "
                                  "{} bytes still unparsed.\n"),
                               unpacker.nonparsed_size());
      }

      //-------------------------------------------------------------------------------

      void
      true_timer_callback(UU uv_timer_t *timer)
      {
      }

      __forceinline __attribute__((__artificial__))
      static constexpr auto const &cast_deleter(void const *data_ptr)
      {
            using deleter_type = std::function<void(std::string const &, bool)>;
            return *static_cast<deleter_type const *>(data_ptr);
      }
};


/*======================================================================================*/


/****************************************************************************************/
} // namespace ipc::protocols::Msgpack
} // namespace emlsp
#endif
