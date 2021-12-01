#pragma once
#ifndef HGUARD_d_EVENT_d_LOOP_HH_
#define HGUARD_d_EVENT_d_LOOP_HH_
/****************************************************************************************/

#include <Common.hh>
#include "client.hh"
#include "lsp-protocol/lsp-connection.hh"

/*--------------------------------------------------------------------------------------*/

namespace emlsp::ipc {

namespace detail {

template <typename T>
concept BaseConnectionSubclass =
    std::derived_from<T, emlsp::ipc::base_connection<typename T::connection_type>>;

} // namespace detail


// wtf
// template <detail::BaseConnectionSubclass ConnectionType, typename MsgType>


// template <
//     class ConnectionType,
//     class MsgType,
//     std::enable_if_t<
//         std::is_base_of<base_connection<typename ConnectionType::connection_type>,
//                         ConnectionType>::value &&
//             std::is_convertible<const volatile ConnectionType *,
//                                 const volatile base_connection<
//                                     typename ConnectionType::connection_type> *>::value,
//         bool> = true>

// template <detail::BaseConnectionSubclass ConnectionType, typename MsgType>


template <typename ConnectionType, typename MsgType, typename UserDataType = void *>
class base_client
{
      using atomic_queue = std::queue<std::atomic<MsgType *>>;

    public:
      using connection_type = ConnectionType;
      using user_data_type  = UserDataType;

    private:
      atomic_queue    queue_{};
      connection_type con_;

      std::mutex                   mtx_ {};
      std::unique_lock<std::mutex> lock_ {mtx_};
      std::condition_variable      condition_ {};

      UserDataType user_data_{};

    public:
      base_client() : lock_(mtx_) {}
      ~base_client() = default;
      explicit base_client(connection_type connection) : base_client()
      {
            con_ = std::move(connection);
      }

      DELETE_COPY_CTORS(base_client);
      DEFAULT_MOVE_CTORS(base_client);

      template <typename ...Types>
      void write_message(Types &&...args) { con_.write_message(args...); }

      template <typename ...Types>
      auto read_message(Types &&...args) { return con_.read_message(args...); }

      ND connection_type       &con()       { return con_; }
      ND connection_type const &con() const { return con_; }
      ND connection_type       &operator()()       { return con_; }
      ND connection_type const &operator()() const { return con_; }

      ND UserDataType       &data()       { return user_data_; }
      ND UserDataType const &data() const { return user_data_; }

      ND auto &mtx()         { return mtx_; }
      ND auto &unique_lock() { return lock_; }
      ND auto &condition()   { return condition_; }

      void cond_wait() { condition_.wait(lock_); }
};

namespace lsp {

template <typename T = void *>
using rapidjson_pipe_client = base_client<pipe_connection, rapidjson::Document, T>;

template <typename T = void *>
using rapidjson_unix_socket_client = base_client<unix_socket_connection, rapidjson::Document, T>;

} // namespace lsp

} // namespace emlsp::ipc

/****************************************************************************************/
#endif // loop.hh
