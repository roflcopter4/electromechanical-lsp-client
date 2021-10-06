#pragma once
#ifndef HGUARD_d_LSP_PROTOCOL_d_LSP_PROTOCOL_HH_
#define HGUARD_d_LSP_PROTOCOL_d_LSP_PROTOCOL_HH_
/****************************************************************************************/

#include "Common.hh"
#include "client.hh"
//#include <nlohmann/json.hpp>


namespace emlsp::rpc::lsp {

namespace detail {

struct pipe_connection;
struct unix_socket_connection;

} // namespace detail


#if 0
class client : public emlsp::client
{
      friend class message;

    public:
      client() : emlsp::client(client_type::LSP) {}
      ~client() = default;

      template <typename... Types>
      int spawn_connection(connection_type contype, Types &&...args)
      {
            char const *unpack[] = {args...};
            set_connection_type(contype);
            return spawn_connection(sizeof(unpack)/sizeof(unpack[0]), const_cast<char **>(unpack));
      }

      template <typename... Types>
      int spawn_connection(Types &&...args)
      {
            char const *unpack[] = {args...};
            return spawn_connection(sizeof(unpack)/sizeof(unpack[0]), const_cast<char **>(unpack));
      }

      int spawn_connection(size_t argc, char **argv) override __attribute__((nonnull));
};
#endif

class pipe_connection : public emlsp::rpc::pipe_connection
{
      void foo() {
            argv_ = nullptr;
      }
};


} // namespace emlsp::rpc::lsp


/****************************************************************************************/
#endif // lsp-protocol.hh
