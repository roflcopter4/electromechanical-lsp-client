#pragma once
#ifndef HGUARD_d_LSP_PROTOCOL_d_LSP_PROTOCOL_HH_
#define HGUARD_d_LSP_PROTOCOL_d_LSP_PROTOCOL_HH_
/****************************************************************************************/

#include "Common.hh"
#include "client.hh"
#include <nlohmann/json.hpp>


namespace emlsp::lsp::protocol {

namespace detail {} // namespace detail

class client;
class message;
using message_ptr = std::unique_ptr<message>;

class client : public emlsp::client
{
    public:
      client() : emlsp::client(Type::LSP) {}

      message_ptr new_message();
};

class message
{
      client const *client_;

    public:
      explicit message(client const *cl) : client_(cl) {}
};


} // namespace emlsp::lsp::protocol


/****************************************************************************************/
#endif // lsp-protocol.hh
