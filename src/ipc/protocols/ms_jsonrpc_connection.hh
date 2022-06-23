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
class read_buffer final
{
      static constexpr size_t default_buffer_size = SIZE_C(1) << 23;

      char            *buf_;
      std::align_val_t buf_align_;

      size_t max_;
      size_t used_{};
      size_t offset_{};

    public:
      explicit read_buffer(size_t const size      = default_buffer_size,
                           size_t const alignment = 4096)
          : buf_(new (std::align_val_t{alignment}) char[size]),
            buf_align_(std::align_val_t{alignment}),
            max_(size)
      {
            memset(buf_, 0, size);
      }

      ~read_buffer()
      {
            operator delete[](buf_, max_, buf_align_);
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
                  char        *tmp    = new (buf_align_) char[max_ += delta];
                  memcpy(tmp, buf_, used_);
                  operator delete[](buf_, oldmax, buf_align_);
                  buf_ = tmp;
            }
      }
};
} // namespace detail


/****************************************************************************************/


template <typename Connection>
      REQUIRES(ipc::BasicConnectionVariant<Connection>)
class alignas(4096) connection final
    : public ipc::basic_protocol_connection<Connection, ipc::io::ms_jsonrpc_wrapper>
// : public Connection,
//   public ipc::basic_io_wrapper<Connection, ipc::io::ms_jsonrpc_wrapper>,
//   public ipc::basic_protocol_interface
{
      std::string key_name{};

    public:
      using this_type = connection<Connection>;
      using base_type = ipc::basic_protocol_interface;

      using connection_type = Connection;
      using io_wrapper_type = ipc::io::ms_jsonrpc_wrapper<Connection>;
      using value_type      = typename io_wrapper_type::value_type;

      using io_wrapper_type::read_object;
      using io_wrapper_type::parse_buffer;

    protected:

    public:
      connection() = default;
      virtual ~connection() override = default;

      connection(connection const &)                = delete;
      connection(connection &&) noexcept            = delete;
      connection &operator=(connection const &)     = delete;
      connection &operator=(connection &&) noexcept = delete;

      //-------------------------------------------------------------------------------

      NOINLINE static std::unique_ptr<connection<Connection>> new_unique() { return std::unique_ptr<connection<Connection>>(new connection<Connection>()); }
      NOINLINE static std::shared_ptr<connection<Connection>> new_shared() { return std::shared_ptr<connection<Connection>>(new connection<Connection>()); }

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

      ND uv_poll_cb  poll_callback()       const override { return poll_callback_wrapper; }
      ND uv_timer_cb timer_callback()      const override { return [](uv_timer_t *) {}; }
      ND uv_alloc_cb pipe_alloc_callback() const override { return alloc_callback_wrapper; }
      ND uv_read_cb  pipe_read_callback()  const override { return read_callback_wrapper; }
      ND void       *data()                      override { return this; }

    private:
      using base_type::want_close_;

      static void timer_callback_wrapper(uv_timer_t *timer)
      {
            auto *self = static_cast<this_type *>(timer->data);
            self->true_timer_callback(timer);
      }

      static void poll_callback_wrapper(uv_poll_t *handle, int status, int events)
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

            // try {
            //       self->true_read_callback(handle, nread, buf);
            // } catch (...) {
            //       if (self->unparsed_data_.first != buf->base)
            //             delete[] buf->base;
            //       throw;
            // }
            // if (self->unparsed_data_.first != buf->base)
            //       delete[] buf->base;
      }

      void
      real_poll_callback(UU uv_poll_t *handle, int status, int events)
      {
            static std::mutex mtx_{};
            auto lock = std::lock_guard(mtx_);

            if (events & UV_DISCONNECT) {
            // disconnect:

                  util::eprint(
                      FC("({}): Got disconnect signal, status {}, for fd [UNKNOWN], within  -- ignoring\n"),
                      this->raw_descriptor(), status);

                  //throw ipc::except::connection_closed();

//                  uv_poll_stop(handle);
//                  auto const *const stopper = static_cast<std::function<void()> *>(base->data);
//                  (*stopper)();

                  //(*static_cast<std::function<void(std::string const &, bool)> *>(
                  //    handle->loop->data))(key_name, false);

            } else if (events & UV_READABLE) {
                  util::eprint(
                      FC("({}): Descriptor {} is readable from within real_poll_callback\n"),
                      this->raw_descriptor(), "[NO FUCKING CLUE]");

                  std::unique_ptr<rapidjson::Document> doc;

                  while (auto rd = this->available() > 0) {
                        if (rdbuf_.avail() == 0)
                              rdbuf_.reserve(rdbuf_.max() + rd);
                        size_t const nread = this->raw_read(rdbuf_.end(), rdbuf_.avail(), 0);
                        rdbuf_.consume(nread);
                  }

                  //try {
                  //      doc = read_object();
                  //      this->notify_all();
                  //} catch (ipc::except::connection_closed &) {
                  //      //goto disconnect;
                  //      return;
                  //}

                  while (rdbuf_.unparsed_size() > 20) {
                        // while () {
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

                        fmt::print(FC("\n\n\033[1;35mRead {} bytes <<_EOF_\n"
                                      "\033[0;33m{}\n\033[1;35m_EOF_\033[0m\n\n"),
                                   sb.GetSize(),
                                   std::string_view{sb.GetString(), sb.GetSize()});
                  }
            } else {
                  util::eprint(FC("Unexpected event:  0x{:X}\n"), events);
            }
      }

      void
      true_timer_callback(uv_timer_t *timer)
      {
            if (want_close_)
                  uv_timer_stop(timer);
      }

      detail::read_buffer rdbuf_{};

      void
      true_alloc_callback(UU uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf)
      {
            rdbuf_.reserve(rdbuf_.max() + suggested_size);
            buf->base = rdbuf_.end();
            buf->len  = static_cast<decltype(buf->len)>(rdbuf_.avail());
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

      void
      true_read_callback(UU uv_stream_t *handle, ssize_t nread, UU uv_buf_t const *buf)
      {
            if (nread == 0)
                  return;
            if (nread < 0) {
                  //::uv_read_stop(handle);
                  //(*static_cast<std::function<void(std::string const &, bool)> *>(
                  //    handle->loop->data))(key_name, false);
                  // throw ipc::except::connection_closed();
                  return;
            }

            rdbuf_.consume(nread);

            while (rdbuf_.unparsed_size() > 20) {
            //while () {
                  std::unique_ptr<rapidjson::Document> doc = true_read_callback_helper();
                  if (!doc)
                        break;
                  auto thrd = std::thread{[this]() { this->notify_one(); }};
                  thrd.detach();

                  auto sb = rapidjson::GenericStringBuffer<
                     rapidjson::UTF8<>,
                     std::remove_reference_t<decltype(doc->GetAllocator())>>{
                         &doc->GetAllocator()};
                  // auto sb = rapidjson::StringBuffer();
                  auto writer = rapidjson::Writer{sb};
                  doc->Accept(writer);

                  util::eprint(FC("\n\n\033[1;35mRead {} bytes <<_EOF_\n"
                                  "\033[0;33m{}\n\033[1;35m_EOF_\033[0m\n\n"),
                               sb.GetSize(),
                               std::string_view{sb.GetString(), sb.GetSize()});
            }
      }

      // std::pair<char *, size_t> unparsed_data_ = {nullptr, 0};

    public:
      void set_key(std::string key)
      {
            key_name = std::move(key);
      }
};


/****************************************************************************************/
} // namespace ipc::protocols::MsJsonrpc
} // namespace emlsp
#endif
