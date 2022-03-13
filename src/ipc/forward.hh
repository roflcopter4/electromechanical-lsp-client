#pragma once
#ifndef HGUARD__IPC__FORWARD_HH_
#define HGUARD__IPC__FORWARD_HH_ //NOLINT

#include "Common.hh"
#include "ipc/connection_impl.hh"
#include <uv.h>

inline namespace emlsp {
namespace ipc {
/****************************************************************************************/


template <typename ConnectionImpl>
      REQUIRES (detail::IsConnectionImplVariant<ConnectionImpl>;)
class basic_dialer;

template <typename ConnectionImpl>
      REQUIRES (detail::IsConnectionImplVariant<ConnectionImpl>;)
class spawn_dialer;

template <typename T>
concept IsDialerVariant = std::derived_from<T, basic_dialer<typename T::connection_impl_type>>;

template <typename DialerVariant>
      REQUIRES (IsDialerVariant<DialerVariant>;)
class basic_connection;


/****************************************************************************************/
} // namespace ipc
} // namespace emlsp
#endif // forward.hh
