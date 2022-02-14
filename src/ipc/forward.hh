#pragma once
#ifndef HGUARD__IPC__FORWARD_HH_
#define HGUARD__IPC__FORWARD_HH_ //NOLINT

#include "Common.hh"
#include "ipc/connection_impl.hh"
#include <uv.h>

inline namespace emlsp {
namespace ipc {
/****************************************************************************************/


namespace loops {
namespace detail {
class root_loop_selector {};
} // namespace detail 

class libevent : public detail::root_loop_selector {};
class libuv : public detail::root_loop_selector {};
class asio : public detail::root_loop_selector {};
} // namespace loops 


/*--------------------------------------------------------------------------------------*/

template <typename ConnectionImpl>
      REQUIRES (detail::IsConnectionImplVariant<ConnectionImpl>)
class base_connection;


template <typename T>
concept IsConnectionVariant = std::derived_from<T, base_connection<typename T::connection_impl_type>>;

template <typename T>
concept IsLoopVariant = std::derived_from<T, loops::detail::root_loop_selector>;

/*--------------------------------------------------------------------------------------*/

template <typename LoopVariant, typename ConnectionType, typename MsgType, typename UserDataType>
      REQUIRES (IsConnectionVariant<ConnectionType>)
class base_client;

template <typename LoopVariant, typename ConnectionType, typename MsgType, typename UserDataType>
      REQUIRES (IsLoopVariant<LoopVariant>)
class base_loop_template;

template <typename LoopVariant, typename ConnectionType, typename MsgType, typename UserDataType>
class base_loop;

/*--------------------------------------------------------------------------------------*/

template <typename ConnectionImpl>
      REQUIRES (detail::IsConnectionImplVariant<ConnectionImpl>)
class basic_dialer;

template <typename ConnectionImpl>
      REQUIRES (detail::IsConnectionImplVariant<ConnectionImpl>)
class spawn_dialer;

template <typename T>
concept IsDialerVariant = std::derived_from<T, basic_dialer<typename T::connection_impl_type>>;


#if 0
template <typename DialerVariant>
class basic_connection;
#endif

template <typename DialerVariant>
      REQUIRES (IsDialerVariant<DialerVariant>)
class basic_connection;

/*--------------------------------------------------------------------------------------*/

namespace detail {
template <typename ConnectionType, typename MsgType, typename UserDataType = void *>
void loop_graceful_signal_cb(uv_signal_t *handle, int signum);
[[noreturn]] void loop_signal_cb(uv_signal_t *handle, int signum);
} // namespace detail

/*--------------------------------------------------------------------------------------*/

#if 0
inline void
get_last_error(asio::error_code &ec, bool is_error_condition)
{
      if (!is_error_condition) {
            ec.assign(0, ec.category());
      } else {
#if defined(ASIO_WINDOWS) || defined(__CYGWIN__)
            ec = asio::error_code(WSAGetLastError(), asio::error::get_system_category());
#else
            ec = asio::error_code(errno, asio::error::get_system_category());
#endif
      }
}
#endif


/****************************************************************************************/
} // namespace ipc
} // namespace emlsp
#endif // forward.hh
