#pragma once
#ifndef HGUARD_d_LSP_PROTOCOL_d_LSP_CONNECTION_HH_
#define HGUARD_d_LSP_PROTOCOL_d_LSP_CONNECTION_HH_
/****************************************************************************************/

#include "Common.hh"
#include "client.hh"

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
            char   buf[40];
            char  *ptr = buf;
            size_t len;
            char   ch;

            raw_read(ptr, 16); // Discard
            raw_read(&ch, 1);

            while (isdigit(ch)) {
                  *ptr++ = ch;
                  raw_read(&ch, 1);
            }

            *ptr = '\0';
            len  = strtoull(buf, &ptr, 10);
            raw_read(buf, 3); // Discard

            if (ptr == buf || len == 0) [[unlikely]]
                  throw std::runtime_error(
                      fmt::format(FMT_COMPILE("Invalid JSON-RPC: no non-zero number "
                                              "found in header at \"{}\"."),
                                  buf));

            return len;
      }

      inline size_t read_jsonrpc_message(char *&msg)
      {
            size_t len = get_content_length();
            msg = new char[len + 1];
            raw_read(msg, len);
            msg[len] = '\0';
            return len;
      }

      inline void write_jsonrpc_header(size_t const len)
      {
            /* It's safe to go with sprintf because we know for certain that a single
             * 64 bit integer will fit in this buffer along with the format. */
            char buf[60];
            int  buflen = sprintf(buf, "Content-Length: %zu\r\n\r\n", len);
            raw_write(buf, buflen);
      }

    public:
      base_connection()  = default;
      ~base_connection() = default;

      template <typename... Types>
      explicit base_connection(Types &&...args)
      {
            spawn_connection(args...);
      }

      template <typename CharT>
      ND size_t read_message(CharT *&buf)
      {
            char *msg;
            auto len = read_jsonrpc_message(msg);
            buf      = msg;
            return len;
      }

      ND __attribute__((nonnull)) size_t
      read_message(_Notnull_ char **buf) override
      {
            return read_jsonrpc_message(*buf);
      }

      ND __attribute__((nonnull)) size_t
      read_message(_Notnull_ void **buf) override
      {
            return read_message(reinterpret_cast<char **>(buf));
      }

      __attribute__((nonnull)) void
      write_message(_Notnull_ void const *msg, size_t const len) override
      {
            write_jsonrpc_header(len);
            raw_write(msg, len);
      }

      DELETE_COPY_CTORS(base_connection);
      DEFAULT_MOVE_CTORS(base_connection);
};

using unix_socket_connection = emlsp::rpc::lsp::base_connection<emlsp::rpc::detail::unix_socket_connection_impl>;
using pipe_connection        = emlsp::rpc::lsp::base_connection<emlsp::rpc::detail::pipe_connection_impl>;

} // namespace emlsp::rpc::lsp


/****************************************************************************************/
#endif // lsp-connection.hh
