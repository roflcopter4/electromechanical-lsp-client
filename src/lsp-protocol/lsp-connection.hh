#pragma once
#ifndef HGUARD_d_LSP_PROTOCOL_d_LSP_CONNECTION_HH_
#define HGUARD_d_LSP_PROTOCOL_d_LSP_CONNECTION_HH_
/****************************************************************************************/

#include "Common.hh"
#include "client.hh"

#include "rapid.hh"
#include <nlohmann/json.hpp>

#define MIN_OF(a, b) (((a) < (b)) ? (a) : (b))

namespace emlsp::rpc::lsp {


/**
 * TODO: Documentation
 */
template <typename ConnectionType>
class base_connection : public emlsp::rpc::base_connection<ConnectionType>
{
      using Base = emlsp::rpc::base_connection<ConnectionType>;

    public:
      using Base::connection;
      using Base::pid;
      using Base::raw_read;
      using Base::raw_write;
      using Base::spawn_connection;

    private:
      inline size_t get_content_length()
      {
            size_t len;
            char   buf[40];
            char  *ptr = buf;
            char   ch;

            raw_read(buf, 16); // Discard
            raw_read(&ch, 1);

            while (isdigit(ch)) {
                  *ptr++ = ch;
                  raw_read(&ch, 1);
            }

            *ptr = '\0';
            len  = strtoull(buf, &ptr, 10);
            raw_read(buf, 3); // Discard

            if (ptr == buf || len == 0) [[unlikely]] {
                  throw std::runtime_error(
                      fmt::format(FMT_COMPILE("Invalid JSON-RPC: no non-zero number "
                                              "found in header at \"{}\"."),
                                  buf));
            }
            return len;
      }

      inline void write_jsonrpc_header(size_t const len)
      {
            /* It's safe to go with sprintf because we know for certain that a single
             * 64 bit integer will fit in this buffer along with the format. */
            char buf[60];
            int  buflen = ::sprintf(buf, "Content-Length: %zu\r\n\r\n", len);
            raw_write(buf, buflen);
      }

    /*------------------------------------------------------------------------------------*/
    public:
      base_connection()  = default;
      ~base_connection() = default;

      template <typename ...Types>
      explicit base_connection(Types &&...args)
      {
            spawn_connection(args...);
      }

      __attribute__((nonnull)) size_t
      read_message(_Notnull_ char **buf) override
      {
            auto len = get_content_length();
            *buf     = new char[len + 1];
            raw_read(*buf, len);
            (*buf)[len] = '\0';
            return len;
      }

      std::vector<char> read_message()
      {
            auto len = get_content_length();
            auto msg = std::vector<char>();
            emlsp::util::resize_vector_hack(msg, len + 1);
            raw_read(msg.data(), len);
            msg[len] = '\0';
            return msg;
      }

      __attribute__((nonnull)) size_t
      read_message(_Notnull_ void **buf) override
      {
            return read_message(reinterpret_cast<char **>(buf));
      }

      template <typename CharT>
      size_t read_message(CharT *&buf)
      {
            char *msg;
            auto  len = read_message(&msg);
            buf       = static_cast<char *>(msg);
            return len;
      }

      size_t read_message(std::unique_ptr<char[]> &buf)
      {
            char *msg;
            auto  len = read_message(&msg);
            buf       = std::unique_ptr<char[]>(msg);
            return len;
      }

      size_t discard_message()
      {
            auto len = get_content_length();
            char msg[65536];

            for (auto n = static_cast<ssize_t>(len); n > 0; n -= 65536)
                  raw_read(msg, MIN_OF(n, 65536));

            return len;
      }

      /* None of the `write_message` overloads take only a C string with no length
       * argument. If you really failed to keep track of the length then call strlen
       * yourself. Consider it penence. */

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
            doc["jsonrpc"] = "2.0"s;
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


} // namespace emlsp::rpc::lsp

#undef MIN_OF

/****************************************************************************************/
#endif // lsp-connection.hh
