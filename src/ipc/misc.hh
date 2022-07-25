#pragma once
#ifndef HGUARD__IPC__MISC_HH_
#define HGUARD__IPC__MISC_HH_ //NOLINT

#include "Common.hh"
#ifndef _WIN32
#  include <netdb.h>
#  include <netinet/in.h>
#endif

inline namespace emlsp {
namespace ipc {  //NOLINT(modernize-concat-nested-namespaces)
/****************************************************************************************/


namespace except {

class connection_closed final : public std::runtime_error
{
      connection_closed(std::string const &message, char const *function)
          : std::runtime_error(message + " : "s + function)
      {

      }
      connection_closed(char const *message, char const *function)
          : connection_closed(std::string(message), function)
      {}
    public:
      connection_closed() : connection_closed("Connection closed"s, FUNCTION_NAME)
      {}
      explicit connection_closed(char const *message)
          : connection_closed(message, FUNCTION_NAME)
      {}
      explicit connection_closed(std::string const &message)
          : connection_closed(message, FUNCTION_NAME)
      {}
};

} // namespace except


namespace detail {

template<typename T>
concept IsInetSockaddr = std::same_as<T, sockaddr_in> ||
                         std::same_as<T, sockaddr_in6>;

using ::emlsp::util::detail::procinfo_t;

#ifdef _WIN32

using file_handle_t   = ::HANDLE;
using iovec           = ::WSABUF;
using iovec_size_type = ::ULONG;
using descriptor_type = ::intptr_t;

__forceinline constexpr void
init_iovec(iovec &vec, char *buf, ULONG const size)
{
      vec.buf = buf;
      vec.len = size;
}

#else

using file_handle_t   = int;
using iovec           = struct ::iovec;
using iovec_size_type = size_t;
using descriptor_type = int;

__forceinline constexpr void
init_iovec(iovec &vec, char *buf, size_t const size)
{
      vec.iov_base = buf;
      vec.iov_len  = size;
}

#endif

#if 0
#ifdef _WIN32
# define iov_len  len
# define iov_base buf
# define IOVEC_TYPE ::_WSABUF
#else
# define IOVEC_TYPE ::iovec
#endif

class iovec : public IOVEC_TYPE
{

    public:
      explicit iovec() : IOVEC_TYPE() {}

      // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
      explicit iovec(char *buffer, size_t const length)
      {
            iov_base = buffer;
            iov_len  = static_cast<decltype(len)>(length);
      }

      // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
      explicit iovec(char *buffer)
      {
            iov_base = buffer;
            iov_len  = static_cast<decltype(len)>(__builtin_strlen(buffer));
      }

      auto const *const &base() const   & { return iov_base; }
      auto const        &length() const & { return iov_len; }

      auto *&base()   & { return iov_base; }
      auto  &length() & { return iov_len; }

#ifdef _WIN32
# undef iov_len
# undef iov_base
#endif

      //friend constexpr auto iov_len (iovec const &vec) { return vec.len;  }
      //friend constexpr auto iov_len (iovec const *vec) { return vec->len; }
      //friend constexpr auto iov_base(iovec const &vec) { return vec.buf;  }
      //friend constexpr auto iov_base(iovec const *vec) { return vec->buf; }
};

#undef IOVEC_TYPE
#endif

constexpr auto invalid_socket = static_cast<socket_t>(-1);

} // namespace detail


/****************************************************************************************/
} // namespace ipc
} // namespace emlsp
#endif // misc.hh
