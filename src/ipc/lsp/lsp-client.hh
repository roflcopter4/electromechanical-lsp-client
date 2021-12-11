#pragma once
#ifndef HGUARD__IPC__LSP__LSP_CLIENT_HH_
#define HGUARD__IPC__LSP__LSP_CLIENT_HH_

#include "Common.hh"
#include "ipc/base_client.hh"
#include "ipc/base_connection.hh"
#include "ipc/lsp/lsp-connection.hh"

#include <msgpack.hpp>
#include <mutex>
#include <thread>

inline namespace emlsp {
namespace ipc::lsp {

/****************************************************************************************/

#if 0
template <typename T>
concept ConnectionVariant = std::same_as<T, emlsp::ipc::lsp::unix_socket_connection> ||
                            std::same_as<T, emlsp::ipc::lsp::pipe_connection>;
#endif

#if 0
template <ConnectionVariant ConnectionType>
#endif
#if 0
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
#endif

#if 0
template <typename T>
concept ConnectionVariant = std::same_as<T, emlsp::ipc::lsp::unix_socket_connection> ||
                            std::same_as<T, emlsp::ipc::lsp::pipe_connection>;
#endif

#if 0
template <typename T, typename ConnectionImpl>
concept ConnectionVariant = std::derived_from<T, ipc::base_connection<ConnectionImpl>>;
#endif

template <typename T>
concept ConnectionVariant = std::derived_from<T, ipc::base_connection<typename T::connection_type>>;
template <typename T>
concept RpcObjectVariant = std::same_as<T, rapidjson::Document> ||
                           std::same_as<T, msgpack::unpacker>;

#if 0

#if 0
template <ConnectionVariant ConnectionType, RpcObjectVariant MsgType, typename UserDataType = void *>
#endif
#if 0
template <typename ConnectionType, typename MsgType, typename UserDataType = void *>
      requires ConnectionVariant<ConnectionType> && RpcObjectVariant<MsgType>
#endif
#if 0
template <typename ConnectionType, typename MsgType, typename UserDataType = void *>
#endif

template <typename ConnectionType,
          typename MsgType,
          typename UserDataType = void *,
          typename std::enable_if<
              std::is_base_of<ipc::base_connection<typename ConnectionType::connection_type>,
                                                   ConnectionType>::value,
              bool>::type = true,
          typename std::enable_if<
              std::is_same<MsgType, rapidjson::Document>::value ||
                std::is_same<MsgType, msgpack::unpacker>::value,
              bool>::type = true
>
class client : public ipc::base_client<ConnectionType, MsgType, UserDataType>
{
    public:
      using base_type       = ipc::base_client<ConnectionType, MsgType, UserDataType>;
      using connection_type = ConnectionType;
      using message_type    = MsgType;
      using user_data_type  = UserDataType;

      using base_type::con;
      using base_type::data;

    private:
      using base_type::con_;
      using base_type::condition_;
      using base_type::lock_;
      using base_type::mtx_;
      using base_type::queue_;

      ND ConnectionType       &operator()() { return con_; }
      ND ConnectionType const &operator()() const { return con_; }
};


template <typename T = void *>
using rapidjson_pipe_client = client<pipe_connection, rapidjson::Document, T>;

template <typename T = void *>
using rapidjson_unix_socket_client = client<unix_socket_connection, rapidjson::Document, T>;
#endif

template <typename ConnectionType, typename UserDataType = void *>
#ifndef __TAG_HIGHLIGHT__
      requires ipc::BaseConnectionVariant<ConnectionType>
#endif
class client : public ipc::base_client<ConnectionType, rapidjson::Document, UserDataType>
{
    public:
      using connection_type = ConnectionType;
      using message_type    = rapidjson::Document;
      using user_data_type  = UserDataType;
      using base_type       = ipc::base_client<ConnectionType, message_type, UserDataType>;

    private:
      // using base_type::con_;
      // using base_type::condition_;
      // using base_type::lock_;
      // using base_type::mtx_;
      using base_type::queue_;

    public:
      client()  = default;
      ~client() = default;

#if 0
      ND connection_type       &operator()()       { return this->con_(); }
      ND connection_type const &operator()() const { return this->con_(); }
#endif

      DELETE_COPY_CTORS(client);
      DEFAULT_MOVE_CTORS(client);
};

template <typename T = void *>
using unix_socket_client = client<unix_socket_connection, T>;
template <typename T = void *>
using pipe_client = client<pipe_connection, T>;


/****************************************************************************************/
} // namespace ipc::lsp
} // namespace emlsp
#endif // lsp-client.hh
