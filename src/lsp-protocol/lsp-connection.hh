#pragma once
#ifndef HGUARD_d_LSP_PROTOCOL_d_LSP_CONNECTION_HH_
#define HGUARD_d_LSP_PROTOCOL_d_LSP_CONNECTION_HH_
/****************************************************************************************/

#include "Common.hh"
#include "client.hh"

#include "rapid.hh"
#include <nlohmann/json.hpp>


namespace emlsp::rpc::lsp {
namespace detail {}


/**
 * TODO: Documentation
 */
template <typename ConnectionType>
class base_connection : public emlsp::rpc::base_connection<ConnectionType>
{
#ifdef __INTELLISENSE__
      static constexpr ssize_t discard_buffer_size = SSIZE_C(16383);
#else
      static constexpr ssize_t discard_buffer_size = SSIZE_C(65536);
#endif
      using base = emlsp::rpc::base_connection<ConnectionType>;

    public:
      using base::connection;
      using base::pid;
      using base::raw_read;
      using base::raw_write;
      using base::spawn_connection;

    private:
      size_t get_content_length()
      {
            /* Exactly 64 bytes assuming an 8 byte pointer size. Three cheers for
             * pointless "optimizations". */
            char    buf[47];
            char   *ptr = buf;
            size_t  len;
            uint8_t ch;

            raw_read(buf, 16); // Discard
            raw_read(&ch, 1);

            while (::isdigit(ch)) {
                  *ptr++ = static_cast<char>(ch);
                  raw_read(&ch, 1);
            }

            *ptr = '\0';
            len  = ::strtoull(buf, &ptr, 10);
            raw_read(buf, 3); // Discard

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
            raw_write(buf, buflen);
      }

    /*------------------------------------------------------------------------------------*/
    public:
      base_connection()           = default;
      ~base_connection() override = default;

      template <typename ...Types>
      explicit base_connection(Types &&...args)
      {
            spawn_connection(args...);
      }

      std::string read_message_string() override
      {
            auto len = get_content_length();
            auto msg = std::string(len + 1, '\0');
            raw_read(msg.data(), len);
            return msg;
      }

      /**
       * \brief Read an rpc message into a buffer contained in an std::vector.
       */
      std::vector<char> read_message() override
      {
            auto len = get_content_length();
            auto msg = std::vector<char>();
            util::resize_vector_hack(msg, len + 1);
            raw_read(msg.data(), len);
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
            raw_read(*buf, len);
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
       *        supplied pointer.
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
       * \brief Read and ignore an entire RPC message.
       * \return Number of bytes read.
       */
      size_t discard_message() override
      {
            ssize_t const len = get_content_length();
            char          msg[discard_buffer_size];

            for (auto n = len; n > 0; n -= discard_buffer_size)
                  raw_read(msg, std::min(n, discard_buffer_size));

            return len;
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

      void write_message(rapidjson::Document &doc)
      {
            doc.AddMember("jsonrpc", "2.0", doc.GetAllocator());
            rapidjson::MemoryBuffer ss;
            rapidjson::Writer       writer(ss);
            doc.Accept(writer);
            write_message(ss.GetBuffer(), ss.GetSize());
      }

      void write_message(nlohmann::json &doc)
      {
            doc ["jsonrpc"] = "2.0";
            std::stringstream buf;
            buf << doc;
            auto const msg = buf.str();
            write_message(msg.data(), msg.size());
      }

      DELETE_COPY_CTORS(base_connection);
      DEFAULT_MOVE_CTORS(base_connection);
};


using unix_socket_connection = base_connection<emlsp::rpc::detail::unix_socket_connection_impl>;
using pipe_connection        = base_connection<emlsp::rpc::detail::pipe_connection_impl>;
#ifdef _WIN32
using named_pipe_connection  = base_connection<emlsp::rpc::detail::win32_named_pipe_impl>;
#endif


} // namespace emlsp::rpc::lsp

/****************************************************************************************/
#endif // lsp-connection.hh
