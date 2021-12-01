#pragma once
#ifndef HGUARD_d_CLIENT_DETAILS_HH_
#define HGUARD_d_CLIENT_DETAILS_HH_
/****************************************************************************************/

#include "client.hh"

namespace emlsp::ipc::detail {

template <typename AddrType>
socket_connection_base_impl<AddrType>::~socket_connection_base_impl()
{
      if (accepted_ != (-1)) {
            shutdown(accepted_, 2);
            close_socket(accepted_);
      }
      if (root_ != (-1)) {
            shutdown(root_, 2);
            close_socket(root_);
      }
}

template <typename AddrType>
ssize_t
socket_connection_base_impl<AddrType>::read(void *buf, size_t const nbytes, int const flags)
{
      size_t total = 0, n;

      if (flags & MSG_WAITALL) {
            do {
                  n = ::recv(accepted_, static_cast<char *>(buf) + total,
                             nbytes - total, flags);
            } while ((n != (-1)) && (total += n) < static_cast<ssize_t>(nbytes));
      } else {
            n = ::recv(accepted_, static_cast<char *>(buf), nbytes, flags);
      }

      if (n == (-1))
#if defined _WIN32
            util::win32::error_exit(L"recv()");
#else
            err(1, "send()");
#endif

      return total;
}

template <typename AddrType>
ssize_t
socket_connection_base_impl<AddrType>::write(void const *buf, size_t const nbytes, int const flags)
{
      ssize_t total = 0, n;

      do {
            n = ::send(accepted_, static_cast<char const *>(buf) + total,
                       nbytes - total, flags);
      } while (n != (-1) && (total += n) < static_cast<ssize_t>(nbytes));

      if (n == (-1))
#if defined _WIN32
            util::win32::error_exit(L"send()");
#else
            err(1, "send()");
#endif

      return total;
}

} // namespace emlsp::ipc::detail

/****************************************************************************************/
#endif // client_details.hh
