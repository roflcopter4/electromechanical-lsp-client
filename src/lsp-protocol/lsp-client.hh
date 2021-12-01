#pragma once
#ifndef HGUARD_d_LSP_PROTOCOL_d_LSP_CLIENT_HH_
#define HGUARD_d_LSP_PROTOCOL_d_LSP_CLIENT_HH_
/****************************************************************************************/

#include "Common.hh"
#include "client.hh"
#include "lsp-protocol/lsp-connection.hh"
#include <thread>
#include <mutex>

namespace emlsp::ipc::lsp {

#if 0
template <typename T>
concept ConnectionVariant = std::same_as<T, emlsp::ipc::lsp::unix_socket_connection> ||
                            std::same_as<T, emlsp::ipc::lsp::pipe_connection>;
#endif

#if 0
template <ConnectionVariant ConnectionType>
#endif
class client
{
      unix_socket_connection connection_;

      std::thread loop_ = {};
      std::mutex  mtx_  = {};

    public:
      client()  = default;
      ~client() = default;
      explicit client(unix_socket_connection &con) : connection_(std::move(con)) {}

      unix_socket_connection &con() { return connection_; }
      unix_socket_connection &operator()() { return connection_; }

      DELETE_COPY_CTORS(client);
      DELETE_MOVE_CTORS(client);
};

} // namespace emlsp::ipc::lsp

/****************************************************************************************/
#endif // lsp-client.hh
