#pragma once
#ifndef HGUARD__IPC__LSP__LSP_CLIENT_HH_
#define HGUARD__IPC__LSP__LSP_CLIENT_HH_ //NOLINT

#include "Common.hh"
#include "ipc/base_client.hh"
#include "ipc/base_connection.hh"
#include "ipc/base_wait_loop.hh"
#include "ipc/lsp/lsp-connection.hh"

#include <msgpack.hpp>
#include <mutex>
#include <thread>

#ifdef __TAG_HIGHLIGHT__
#  define requires(...)
#endif

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
concept ConnectionVariant = std::derived_from<T, ipc::connection<ConnectionImpl>>;
#endif

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
              std::is_base_of<ipc::connection<typename ConnectionType::connection_type>,
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

template <typename T> // IsPointer is redundant with IsTrivial, but whatever
concept ValidUserDataType = util::concepts::IsPointer<T> ||
                            util::concepts::IsTrivial<T> ||
                            util::concepts::IsUniquePtr<T>;


template <typename LoopVariant, typename ConnectionType, typename UserDataType = void *>
      REQUIRES (ipc::IsConnectionVariant<ConnectionType>;) //&& ValidUserDataType<UserDataType>
class client
    : public ipc::base_client<
          // ipc::wait_loop_libevent2<ConnectionType, rapidjson::Document, UserDataType>,
          // ipc::loops::libevent,
          LoopVariant,
          ConnectionType, rapidjson::Document, UserDataType>
{
    public:
      using connection_type = ConnectionType;
      using message_type    = rapidjson::Document;
      using user_data_type  = UserDataType;
      using base_type       = ipc::base_client<
          // ipc::wait_loop_libevent2<ConnectionType, rapidjson::Document, UserDataType>,
          // ipc::loops::libevent,
          LoopVariant,
          ConnectionType, rapidjson::Document, UserDataType>;

    private:
      // using base_type::con_;
      // using base_type::condition_;
      // using base_type::lock_;
      // using base_type::mtx_;
      // using base_type::queue_;
      UserDataType userdata_;

    public:
      client()           = default;
      ~client() override = default;

      void foo () {
            if (this->queue_.empty()) {
                  fmt::print(stderr, "it is empty");
            }
      }

#if 0
      explicit client(UserDataType &d) : userdata_(std::move(d)) {}
      explicit client(UserDataType &&d) requires(is_unique_ptr<UserDataType>)
          : userdata_(std::move(d))
      {}
      explicit client(UserDataType const &d) requires(is_trivial<UserDataType>)
          : userdata_(d)
      {}

      ND auto       &userdata()       { return userdata_; }
      ND auto const &userdata() const { return userdata_; }
#endif

      DELETE_COPY_CTORS(client);
      DEFAULT_MOVE_CTORS(client);
};

template <typename LoopVariant = ipc::loops::libevent, typename UserDataType = void *>
using unix_socket_client = client<LoopVariant, unix_socket_connection, UserDataType>;
template <typename LoopVariant = ipc::loops::libevent, typename UserDataType = void *>
using pipe_client = client<LoopVariant, pipe_connection, UserDataType>;


/****************************************************************************************/
} // namespace ipc::lsp
} // namespace emlsp
#endif // lsp-client.hh
