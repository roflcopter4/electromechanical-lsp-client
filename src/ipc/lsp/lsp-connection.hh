#pragma once
#ifndef HGUARD__IPC__LSP___CONNECTION_HH_
#define HGUARD__IPC__LSP___CONNECTION_HH_ //NOLINT

#include "Common.hh"
#include "ipc/base_connection.hh"
#include "rapid.hh"
//#include <nlohmann/json.hpp>

inline namespace emlsp {
namespace ipc::lsp {
/****************************************************************************************/


namespace detail {} // namespace detail


/**
 * TODO: Documentation
 */
template <typename ConnectionImpl>
#ifndef __TAG_HIGHLIGHT__
      requires (ipc::detail::IsConnectionImplVariant<ConnectionImpl>)
#endif
class connection : public ipc::base_connection<ConnectionImpl>
{
#ifdef __INTELLISENSE__
      static constexpr ssize_t discard_buffer_size = SSIZE_C(16383);
#else
      static constexpr ssize_t discard_buffer_size = SSIZE_C(65536);
#endif
      using base_type = ipc::base_connection<ConnectionImpl>;

    public:
      using connection_type = ConnectionImpl;
      // using base::connection;
      using base_type::pid;
      using base_type::raw_read;
      using base_type::raw_write;
      using base_type::spawn_connection;

    private:
      ND size_t get_content_length()
      {
            /* Exactly 64 bytes assuming an 8 byte pointer size. Three cheers for
             * pointless "optimizations". */
            char    buf[47];
            char   *ptr = buf;
            size_t  len;
            uint8_t ch;

            (void)raw_read(buf, 16, MSG_WAITALL); // Discard
            (void)raw_read(&ch, 1, MSG_WAITALL);
            while (::isdigit(ch)) {
                  *ptr++ = static_cast<char>(ch);
                  (void)raw_read(&ch, 1, MSG_WAITALL);
            }

            *ptr = '\0';
            len  = ::strtoull(buf, &ptr, 10);

            /* Some "clever" servers send some extra bullshit after the message length.
             * Just search for the mandatory '\r\n\r\n' sequence to ignore it. */
            for (;;) {
                  while (ch != '\r') [[unlikely]] {
                  fail:
                        (void)raw_read(&ch, 1, MSG_WAITALL);
                  }
                  (void)raw_read(&ch, 1, MSG_WAITALL);
                  if (ch != '\n') [[unlikely]]
                        goto fail; //NOLINT
                  (void)raw_read(buf, 2, MSG_WAITALL);
                  if (::strncmp(buf, "\r\n", 2) == 0) [[likely]]
                        break;
            }

            if (ptr == buf || len == 0) [[unlikely]] {
                  throw std::runtime_error(
                      fmt::format(FMT_COMPILE("Invalid JSON-RPC: no non-zero number "
                                              "found in header at \"{}\"."),
                                  buf));
            }
            return len;
      }

      void write_jsonrpc_header(size_t const len)
      {
            /* It's safe to go with sprintf because we know for certain that a single
             * 64 bit integer will fit in this buffer along with the format. */
            char buf[60];
            int  buflen = ::sprintf(buf, "Content-Length: %zu\r\n\r\n", len);
            raw_write(buf, static_cast<size_t>(buflen));
      }

    /*------------------------------------------------------------------------------------*/
    public:
      connection() = default;
      ~connection() override = default;

      /**
       * \brief Read an rpc message into a buffer contained in a std::string.
       */
      std::string read_message_string() override
      {
            auto len = get_content_length();
            auto msg = std::string(len + 1, '\0');

            UNUSED auto const nread = raw_read(msg.data(), len, MSG_WAITALL);
            assert(static_cast<size_t>(nread) == len);

            return msg;
      }

      /**
       * \brief Read an rpc message into a buffer contained in a std::vector.
       */
      std::vector<char> read_message() override
      {
            auto len = get_content_length();
            auto msg = std::vector<char>();
            util::resize_vector_hack(msg, len + 1);

            UNUSED auto const nread = raw_read(msg.data(), len, MSG_WAITALL);
            assert(static_cast<size_t>(nread) == len);

            msg[len] = '\0';
            return msg;
      }

      /**
       * \brief Read an RPC message into an allocated buffer, and assign it to the
       *        supplied double pointer.
       * \param buf Nonnull pointer to a valid (char *) variable.
       * \return Number of bytes read.
       */
      __attribute__((nonnull)) size_t
      read_message(_Notnull_ char **buf) override
      {
            auto len = get_content_length();
            *buf     = new char[len + 1];

            UNUSED auto const nread = raw_read(*buf, len, MSG_WAITALL);
            assert(static_cast<size_t>(nread) == len);

            (*buf)[len] = '\0';
            return len;
      }

      /**
       * \brief Simple wrapper for the (char **) overload for those too lazy to cast.
       */
      __attribute__((nonnull)) size_t
      read_message(_Notnull_ void **buf) override
      {
            return read_message(reinterpret_cast<char **>(buf));
      }

      /**
       * \brief Read an RPC message into an allocated buffer, and assign it to the
       *        supplied pointer reference. For those too lazy to take its address.
       * \param buf Reference to a valid (char *) variable.
       * \return Number of bytes read.
       */
      size_t read_message(char *&buf) override
      {
            char      *msg;
            auto const len = read_message(&msg);
            buf            = msg;
            return len;
      }

      /**
       * \brief Read an RPC message into an allocated buffer, and assign it to the
       *        supplied unique_ptr, which should not be initialized.
       * \param buf Reference to an uninitialized unique_ptr<char[]>.
       * \return Number of bytes read.
       */
      size_t read_message(std::unique_ptr<char[]> &buf) override
      {
            char      *msg;
            auto const len = read_message(&msg);
            buf            = std::unique_ptr<char[]>(msg);
            return len;
      }

      /**
       * \brief Read an RPC message into an allocated buffer, and assign it to the
       *        supplied shared_ptr, which should not be initialized.
       * \param buf Reference to an uninitialized shared_ptr<char[]>.
       * \return Number of bytes read.
       */
      size_t read_message(std::shared_ptr<char[]> &buf) override
      {
            char      *msg;
            auto const len = read_message(&msg);
            buf            = std::shared_ptr<char[]>(msg);
            return len;
      }

      /**
       * \brief Read and ignore an entire RPC message.
       * \return Number of bytes read.
       */
      size_t discard_message() override
      {
            auto const len = static_cast<ssize_t>(get_content_length());
            char       msg[discard_buffer_size];

            for (auto n = len; n > 0; n -= discard_buffer_size) {
                  auto        const toread = std::min(n, discard_buffer_size);
                  UNUSED auto const nread  = raw_read(msg, static_cast<size_t>(toread), MSG_WAITALL);
                  assert(nread == toread);
            }

            return static_cast<size_t>(len);
      }

      __attribute__((nonnull)) void
      write_message(_Notnull_ char const *msg) override
      {
            write_message(msg, strlen(msg));
      }

      __attribute__((nonnull)) void
      write_message(_Notnull_ void const *msg, size_t const len) override
      {
            write_jsonrpc_header(len);
            raw_write(msg, len);
      }

      void write_message(std::string const &msg) override
      {
            write_jsonrpc_header(msg.size());
            raw_write(msg.data(), msg.size());
      }

      void write_message(std::vector<char> const &msg) override
      {
            write_jsonrpc_header(msg.size() - SIZE_C(1));
            raw_write(msg.data(), msg.size() - SIZE_C(1));
      }

      void write_message(rapidjson::Document const &doc)
      {
            // doc.AddMember("jsonrpc", "2.0", doc.GetAllocator());
            rapidjson::MemoryBuffer ss;
            rapidjson::Writer       writer(ss);
            doc.Accept(writer);
            write_message(ss.GetBuffer(), ss.GetSize());
      }

#if 0
      void write_message(nlohmann::json &doc)
      {
            doc ["jsonrpc"] = "2.0";
            std::stringstream buf;
            buf << doc;
            auto const msg = buf.str();
            write_message(msg.data(), msg.size());
      }
#endif

      DELETE_COPY_CTORS(connection);
      DEFAULT_MOVE_CTORS(connection);
};

using unix_socket_connection = connection<ipc::detail::unix_socket_connection_impl>;
using pipe_connection        = connection<ipc::detail::pipe_connection_impl>;

/****************************************************************************************/
} // namespace ipc::lsp
} // namespace emlsp
#endif // lsp-connection.hh
