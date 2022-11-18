#pragma once
#ifndef HGUARD__IPC__PROTOCOLS__MS_JSONRPC_CONNECTION_HH_
#define HGUARD__IPC__PROTOCOLS__MS_JSONRPC_CONNECTION_HH_ //NOLINT
/****************************************************************************************/

#include "Common.hh"
#include "ipc/basic_protocol_connection.hh"

inline namespace emlsp {
namespace ipc::protocols::MsJsonrpc {
/****************************************************************************************/


namespace detail {

/**
 * \brief Buffer. For reading, mostly.
 */
class read_buffer final
{
      static constexpr size_t default_buffer_size  = SIZE_C(1) << 23;
      static constexpr size_t default_buffer_align = 4096;

      std::align_val_t buf_align_;
      size_t  max_;
      size_t  used_{};
      size_t  offset_{};
      char   *buf_;

    public:
      explicit read_buffer(size_t const size      = default_buffer_size,
                           size_t const alignment = default_buffer_align)
          : buf_align_(std::align_val_t{alignment}),
            max_(size),
          /* This stupidity is required to appease MSVC, which inexplicably
           * considers using the placement new operator to be an error. */
            buf_(static_cast<char *>(operator new[](max_, buf_align_)))
      {
            memset(buf_, 0, size);
      }

      ~read_buffer()
      {
#if defined __MINGW64__ && defined __GNUG__ && defined __clang__
            /* On MinGW, using libc++, clang doesn't seem to recognize
             * operator delete[] with a size argument. */
            operator delete[](buf_, buf_align_);
#else
            operator delete[](buf_, max_, buf_align_);
#endif
      }

      read_buffer(read_buffer const &)                = delete;
      read_buffer(read_buffer &&) noexcept            = default;
      read_buffer &operator=(read_buffer const &)     = delete;
      read_buffer &operator=(read_buffer &&) noexcept = default;

      //--------------------------------------------------------------------------

      ND constexpr char const *raw_buf() const { return buf_; }
      ND constexpr size_t      max()     const { return max_; }

      ND constexpr char *end() const __attribute__((__pure__))
      {
            assert(used_ < max_);
            return buf_ + used_;
      }

      ND constexpr size_t avail() const __attribute__((__pure__))
      {
            assert(max_ >= used_);
            return max_ - used_;
      }

      ND constexpr char *unparsed_start() const __attribute__((__pure__))
      {
            assert(max_ >= offset_);
            return buf_ + offset_;
      }

      ND constexpr size_t unparsed_size() const __attribute__((__pure__))
      {
            assert(used_ >= offset_);
            return used_ - offset_;
      }

      void consume(size_t const delta)
      {
            used_ += delta;
            assert(used_ < max_);
      }

      void release(size_t const delta)
      {
            offset_ += delta;
            if (offset_ == used_)
                  offset_ = used_ = 0;
      }

      void resize(size_t const new_size)
      {
            offset_ = used_ - new_size;
      }

      void reserve(size_t const delta)
      {
            if (used_ + delta < max_) {
                  size_t const oldmax = max_;
                  void *const  tmp    = operator new[]((max_ += delta), buf_align_);
                  memcpy(tmp, buf_, used_);
#if defined __MINGW64__ && defined __GNUG__ && defined __clang__
                  operator delete[](buf_, buf_align_);
#else
                  operator delete[](buf_, oldmax, buf_align_);
#endif
                  buf_ = static_cast<char *>(tmp);
            }
      }
};

} // namespace detail


/****************************************************************************************/


template <typename Connection>
      REQUIRES(ipc::BasicConnectionVariant<Connection>)
class connection
    : public ipc::basic_protocol_connection<Connection, ipc::io::ms_jsonrpc_wrapper>
{
      std::mutex          read_mtx_;
      detail::read_buffer rdbuf_{};

    public:
      using this_type = connection<Connection>;
      using base_type = ipc::basic_protocol_interface;

      using connection_type = Connection;
      using io_wrapper_type = ipc::io::ms_jsonrpc_wrapper<Connection>;
      using value_type      = typename io_wrapper_type::value_type;

      using io_wrapper_type::read_object;
      using io_wrapper_type::parse_buffer;

      //-------------------------------------------------------------------------------

      connection()           = default;
      ~connection() override = default;

      connection(connection const &)                = delete;
      connection(connection &&) noexcept            = delete;
      connection &operator=(connection const &)     = delete;
      connection &operator=(connection &&) noexcept = delete;

      //-------------------------------------------------------------------------------

      static std::unique_ptr<connection<Connection>> new_unique()
      {
            return std::unique_ptr<connection<Connection>>(new connection<Connection>());
      }

      static std::shared_ptr<connection<Connection>> new_shared()
      {
            return std::shared_ptr<connection<Connection>>(new connection<Connection>());
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

      ND constexpr uv_poll_cb  poll_callback()       const noexcept final { return poll_callback_wrapper; }
      ND constexpr uv_alloc_cb pipe_alloc_callback() const noexcept final { return alloc_callback_wrapper; }
      ND constexpr uv_read_cb  pipe_read_callback()  const noexcept final { return read_callback_wrapper; }
      ND constexpr uv_timer_cb timer_callback()      const noexcept final { return timer_callback_wrapper; }
      ND constexpr void const *data()                const noexcept final { return this; }
      ND constexpr void       *data()                      noexcept final { return this; }

      //-------------------------------------------------------------------------------

    private:
      static void poll_callback_wrapper(uv_poll_t *handle, intc status, intc events)
      {
            auto *self = static_cast<this_type *>(handle->data);
            self->real_poll_callback(handle, status, events);
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

      static void timer_callback_wrapper(uv_timer_t *timer)
      {
            auto *self = static_cast<this_type *>(timer->data);
            self->true_timer_callback(timer);
      }

      //-------------------------------------------------------------------------------

      void
      real_poll_callback(UU uv_poll_t *handle, intc status, intc events)
      {
            std::lock_guard lock{read_mtx_};

            util::eprint(
                FC("real_poll_callback called for descriptor {} (tag: {}) -- ({}, {})\n"),
                this->raw_descriptor(), this->get_key(), status, events);

            if (!(events & (UV_DISCONNECT | UV_READABLE))) {
                  util::eprint(FC("Unexpected event(s):  0x{:X}\n"), events);
                  return;
            }

            if (status || events == 0) {
                  eprintf("Bugger: %d %d\n", status, events);
                  fflush(stderr);
            }

            if (events & UV_READABLE && this->available() > 0) {
                  try {
                        while (auto rd = this->available() > 0) {
                              if (rdbuf_.avail() == 0)
                                    rdbuf_.reserve(rdbuf_.max() + rd);
                              size_t const nread = this->raw_read(rdbuf_.end(), rdbuf_.avail(), 0);
                              rdbuf_.consume(nread);
                        }
                  } catch (std::exception const &e) {
                        DUMP_EXCEPTION(e);
                        return;
                  }

                  while (rdbuf_.unparsed_size() > 20) {
                        std::unique_ptr<rapidjson::Document> doc = true_read_callback_helper();
                        if (!doc)
                              break;

                        this->notify_all();

                        auto sb = rapidjson::GenericStringBuffer<
                            rapidjson::UTF8<>,
                            std::remove_reference_t<decltype(doc->GetAllocator())>>{
                            &doc->GetAllocator()};
                        auto writer = rapidjson::Writer{sb};
                        doc->Accept(writer);

                        _lock_file(stderr);
                        fmt::print(FC("\n\n\033[1;35mRead {} bytes <<_EOF_\n"
                                      "\033[0;33m{}\n\033[1;35m_EOF_\033[0m\n\n"),
                                   sb.GetSize(),
                                   std::string_view{sb.GetString(), sb.GetSize()});
                        fflush(stdout);
                        _unlock_file(stderr);
                  }
            }

            if (events & UV_DISCONNECT) {
                  util::eprint(FC("Got disconnect signal, status {}, for fd {}, key '{}'\n"),
                               status, this->raw_descriptor(), this->get_key());

                  if (!handle->loop)
                        return;
                  auto const &key_deleter = cast_deleter(handle->loop->data);
                  this->close();
                  key_deleter(this->get_key(), false);
            }
      }

      //-------------------------------------------------------------------------------

      void
      true_alloc_callback(UU uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf)
      {
            std::lock_guard lock{read_mtx_};
            rdbuf_.reserve(rdbuf_.max() + suggested_size);
            buf->base = rdbuf_.end();
            buf->len  = static_cast<decltype(buf->len)>(rdbuf_.avail());
      }

      void
      true_read_callback(uv_stream_t *handle, ssize_t nread, UU uv_buf_t const *buf)
      {
            std::lock_guard lock{read_mtx_};

            if (nread == 0)
                  return;
            if (nread < 0) {
                  util::eprint(
                      FC("({}): Got negative read (disconnect?), for {} aka '{}', of type {}. ({} -> '{}'?)\n"),
                      this->raw_descriptor(),
                      static_cast<void const *>(handle), this->get_key(),
                      handle->type, nread, uv_strerror(static_cast<int>(nread)));
                  auto const &key_deleter = cast_deleter(handle->loop->data);
                  key_deleter(this->get_key(), false);
                  return;
            }

            rdbuf_.consume(nread);

            while (rdbuf_.unparsed_size() > 20) {
                  std::unique_ptr<rapidjson::Document> doc = true_read_callback_helper();
                  if (!doc)
                        break;
                  auto thrd = std::thread{[this]() { this->notify_one(); }};
                  thrd.detach();

                  auto sb = rapidjson::GenericStringBuffer<
                     rapidjson::UTF8<>,
                     std::remove_reference_t<decltype(doc->GetAllocator())>>{
                         &doc->GetAllocator()};
                  auto writer = rapidjson::Writer{sb};
                  doc->Accept(writer);

                  util::eprint(FC("\n\n\033[1;35mRead {} bytes <<_EOF_\n"
                                  "\033[0;33m{}\n\033[1;35m_EOF_\033[0m\n\n"),
                               sb.GetSize(),
                               std::string_view{sb.GetString(), sb.GetSize()});
            }
      }

      std::unique_ptr<rapidjson::Document>
      true_read_callback_helper()
      {
            char const *buffer     = rdbuf_.unparsed_start();
            size_t      buffer_len = rdbuf_.unparsed_size();

            std::pair<std::unique_ptr<rapidjson::Document>, int>
                  parse_output = parse_buffer(buffer, buffer_len);

            auto &doc = parse_output.first;

            if (parse_output.second) {
                  util::eprint(FC("Giving up due to invalid header.\n"));
                  return nullptr;
            }
            if (auto const parse_err = doc->GetParseError();
                parse_err != rapidjson::kParseErrorNone &&
                parse_err != rapidjson::kParseErrorDocumentRootNotSingular)
            {
                  util::eprint(FC("Parsing error {}\n"), static_cast<int>(parse_err));
                  return nullptr;
            }

            rdbuf_.release(buffer - rdbuf_.unparsed_start());
            return std::move(doc);
      }

      //-------------------------------------------------------------------------------

      void
      true_timer_callback(UU uv_timer_t *timer)
      {
      }

      using deleter_type = std::function<void(std::string const &, bool)>;

      /* A monument to laziness. */
      __forceinline __attribute__((__artificial__))
      static deleter_type const &cast_deleter(void const *data_ptr)
      {
            return * static_cast<deleter_type const *>(data_ptr);
      }
};


/****************************************************************************************/
} // namespace ipc::protocols::MsJsonrpc
} // namespace emlsp
#endif
