// ReSharper disable CppClassCanBeFinal
#pragma once
#ifndef HGUARD__IPC__IO__MS_JSONRPC2_HH_
#define HGUARD__IPC__IO__MS_JSONRPC2_HH_ //NOLINT

#include "Common.hh"
#include "ipc/io/basic_io_wrapper.hh"

#define AUTOC auto const

inline namespace emlsp {
namespace ipc::io {
/****************************************************************************************/

namespace detail {

#ifdef _WIN32
using iovec_size_type = DWORD;
#else
using iovec_size_type = size_t;
#endif

class ms_jsonrpc_packer
{
      std::condition_variable &cond_;
      std::mutex               mtx_     = {};
      std::atomic_bool         is_free_ = true;

    public:
      using value_type      = std::unique_ptr<rapidjson::Document>;
      using marshaller_type = ipc::json::rapid_doc<>;

      ms_jsonrpc_packer *get_new_packer()
      {
            std::lock_guard lock(mtx_);
            bool expect = true;
            is_free_.compare_exchange_strong(expect, false,
                                             std::memory_order::seq_cst,
                                             std::memory_order::acquire);
            auto *ret = expect ? this : nullptr;
            return ret;
      }

      void clear()
      {
            std::lock_guard lock(mtx_);
            pk.doc().RemoveAllMembers();
            pk.clear();
            is_free_.store(true, std::memory_order::seq_cst);
            cond_.notify_all();
      }

      explicit ms_jsonrpc_packer(std::condition_variable &cond) : cond_(cond) {}
      ~ms_jsonrpc_packer() = default;

      ms_jsonrpc_packer(ms_jsonrpc_packer &&) noexcept            = delete;
      ms_jsonrpc_packer(ms_jsonrpc_packer const &)                = delete;
      ms_jsonrpc_packer &operator=(ms_jsonrpc_packer &&) noexcept = delete;
      ms_jsonrpc_packer &operator=(ms_jsonrpc_packer const &)     = delete;

      marshaller_type pk = {};
};

} // namespace detail


template <typename Connection>
      REQUIRES (BasicConnectionVariant<Connection>)
class ms_jsonrpc_wrapper
      : public basic_wrapper<Connection, detail::ms_jsonrpc_packer, rapidjson::Document>
{
      using base_type = basic_wrapper<Connection, detail::ms_jsonrpc_packer, rapidjson::Document>;
      using this_type = ms_jsonrpc_wrapper<Connection>;

    public:
      using connection_type = Connection;
      using packer_type     = detail::ms_jsonrpc_packer;
      using unpacker_type   = rapidjson::Document;

      using value_type      = std::unique_ptr<rapidjson::Document>;
      using marshaller_type = ipc::json::rapid_doc<>;

      explicit ms_jsonrpc_wrapper(connection_type *con) : base_type(con)
      {}

      ~ms_jsonrpc_wrapper() override = default;

      ms_jsonrpc_wrapper(ms_jsonrpc_wrapper const &)                = delete;
      ms_jsonrpc_wrapper(ms_jsonrpc_wrapper &&) noexcept            = default;
      ms_jsonrpc_wrapper &operator=(ms_jsonrpc_wrapper const &)     = delete;
      ms_jsonrpc_wrapper &operator=(ms_jsonrpc_wrapper &&) noexcept = default;

    private:
      using base_type::unpacker_;
      using base_type::con_;

      ND size_t get_content_length()
      {
            /* Exactly 64 bytes assuming an 8 byte pointer size. Three cheers for
             * pointless "optimizations". */
            char    buf[47];
            char   *ptr = buf;
            uint8_t ch;

            con_->raw_read(buf, 16, MSG_WAITALL); // Discard
            assert(memcmp(buf, "Content-Length: ", 16) == 0 &&
                   "Invalid JSON-RPC: 'Content-Length' field not found.");
            con_->raw_read(&ch, 1, MSG_WAITALL);

            while (::isdigit(ch)) {
                  *ptr++ = static_cast<char>(ch);
                  con_->raw_read(&ch, 1, MSG_WAITALL);
            }

            *ptr       = '\0';
            AUTOC len  = ::strtoull(buf, &ptr, 10);

            if (ptr == buf || len == 0) [[unlikely]]
                  throw std::runtime_error("Invalid JSON-RPC: 'Content-Length' field "
                                           "does not contain a valid positive integer.");

            /* Some "clever" servers send some extra bullshit after the message length.
             * Just search for the mandatory '\r\n\r\n' sequence to ignore it. */
            for (;;) {
                  while (ch != '\r') [[unlikely]] {
                  fail:
                        con_->raw_read(&ch, 1, MSG_WAITALL);
                  }
                  while (ch == '\r')
                        con_->raw_read(&ch, 1, MSG_WAITALL);
                  if (ch != '\n') [[unlikely]]
                        goto fail;  //NOLINT

                  uint16_t crlf;
                  con_->raw_read(&crlf, 2, MSG_WAITALL);
                  if (::memcmp(&crlf, "\r\n", 2) == 0) [[likely]]
                        break;
                  if (::memcmp(&crlf, "\r\r", 2) == 0) {
                        con_->raw_read(&ch, 1, MSG_WAITALL);
                        if (ch == '\n')
                              break;
                  }
            }

            return len;
      }

      ND size_t get_content_length(char const *&input, size_t &input_len) const
      {
            auto in_read = [&input, &input_len](void *dest, size_t const nbytes)
            {
                  if (nbytes > input_len) [[unlikely]]
                        throw std::runtime_error(
                            fmt::format(FC("Invalid JSON-RPC header: too short")));
                  if (dest)
                        ::memcpy(dest, input, nbytes);
                  input     += nbytes;
                  input_len -= nbytes;
            };

            char    buf[47];
            char   *ptr = buf;
            uint8_t ch;

            in_read(nullptr, 16); // Discard
            in_read(&ch, 1);
            while (::isdigit(ch)) {
                  *ptr++ = static_cast<char>(ch);
                  in_read(&ch, 1);
            }

            *ptr      = '\0';
            AUTOC len = ::strtoull(buf, &ptr, 10);

            if (ptr == buf || len == 0) [[unlikely]] {
                  throw std::runtime_error(
                      fmt::format(FC("Invalid JSON-RPC: no valid number "
                                     "found in header at \"{}\"."),
                                  input));
            }

            for (;;) {
                  while (ch != '\r') [[unlikely]] {
                  fail:
                        in_read(&ch, 1);
                  }
                  while (ch == '\r')
                        in_read(&ch, 1);
                  if (ch != '\n') [[unlikely]]
                        goto fail;  //NOLINT
                  in_read(buf, 2);
                  if (::strncmp(buf, "\r\n", 2) == 0) [[likely]]
                        break;
                  if (::strncmp(buf, "\r\r", 2) == 0) {
                        in_read(&ch, 1);
                        if (ch == '\n')
                              break;
                  }
            }

            return len;
      }

      size_t write_jsonrpc_header(size_t const len)
      {
            char  buf[60];
            char *endp   = fmt::format_to(buf, FC("Content-Length: {}\r\n\r\n"), len);
            AUTOC buflen = endp - static_cast<char const *>(buf);
            *endp        = '\0';
            return con_->raw_write(buf, static_cast<size_t>(buflen));
      }

#if 0
      static void
      obj_read(void *__restrict src, void *__restrict dest, size_t const nbytes)
      {
            bstring *buf = src;
            if (buf->slen < nbytes)
                  errx(
                              "Buffer does not contain a complete msgpack object. Since this code "
                              "is written terribly, this is fatal. (available: %u, need %zu)",
                              buf->slen, nbytes);
            ::memcpy(dest, buf->data, nbytes);
            buf->data += nbytes;
            buf->slen -= nbytes;
      }
#endif

      ssize_t do_write(void const *msg, size_t const size)
      {
            char hbuf[60];
            char  *endp = fmt::format_to(hbuf, FC("Content-Length: {}\r\n\r\n"), size);
            auto  *tmp  = const_cast<char *>(static_cast<char const *>(msg));
            AUTOC  hlen = endp - hbuf;
            *endp       = '\0';

            ipc::detail::iovec vec[2];
            ipc::detail::init_iovec(vec[0], hbuf, static_cast<detail::iovec_size_type>(hlen));
            ipc::detail::init_iovec(vec[1], tmp, static_cast<detail::iovec_size_type>(size));

            return con_->impl().writev(vec, 2);
      }

    public:
      using base_type::write_object;

      size_t write_string(void const *buf, size_t const size)
      {
            std::lock_guard lock(this->write_mtx_);
            return do_write(buf, size);
      }

      __inline size_t write_string(std::string const &str)      { return write_string(str.data(), str.size()); }
      __inline size_t write_string(std::string_view const &str) { return write_string(str.data(), str.size()); }

      size_t write_object(rapidjson::Document &doc)
      {
            std::lock_guard lock(this->write_mtx_);

            rapidjson::StringBuffer sbuf;
            rapidjson::Writer       wr{sbuf, &doc.GetAllocator()};
            doc.Accept(wr);

            return write_string(sbuf.GetString(), sbuf.GetSize());
      }

      __inline size_t write_object(detail::ms_jsonrpc_packer &pk) override
      {
            return write_object(pk.pk.doc());
      }

      __inline size_t write_object(std::unique_ptr<rapidjson::Document> &docp) override
      {
            return write_object(*docp);
      }

      __inline size_t write_object(ipc::json::rapid_doc<> &docp)
      {
            return write_object(docp.doc());
      }

      std::unique_ptr<rapidjson::Document> read_object() override
      {
            std::lock_guard lock(this->read_mtx_);
            AUTOC len  = get_content_length();
            assert(len > 0);
            AUTOC msg  = std::make_unique<char[]>(len + 1);

            UNUSED auto const nread = con_->raw_read(msg.get(), len, MSG_WAITALL);
            assert(static_cast<size_t>(nread) == len);

            msg[len] = '\0';
            auto doc = std::make_unique<rapidjson::Document>();

            doc->Parse</*rapidjson::kParseStopWhenDoneFlag |*/
                       rapidjson::kParseDefaultFlags |
                       rapidjson::kParseFullPrecisionFlag>(msg.get(), len);
            return doc;
      }

      std::pair<std::unique_ptr<rapidjson::Document>, int>
      parse_buffer(char const *&msg, size_t &msg_len)
      {
            std::lock_guard lock(this->read_mtx_);
            size_t len;

            try {
                  len = get_content_length(msg, msg_len);
            } catch (std::exception &e) {
                  util::eprint(FC("Caught exception \"{}\"\n"), e.what());
                  return {nullptr, 1};
            }

            if (len > msg_len)
                  return {nullptr, 2};

            auto doc = std::make_unique<rapidjson::Document>();

            doc->Parse</*rapidjson::kParseStopWhenDoneFlag |*/
                       rapidjson::kParseDefaultFlags |
                       rapidjson::kParseFullPrecisionFlag>(msg, len);

            msg     += len;
            msg_len -= len;
            return {std::move(doc), 0};
      }
};


/****************************************************************************************/
} // namespace ipc::io
} // namespace emlsp
#undef AUTOC
#endif
