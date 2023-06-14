#pragma once
#ifndef HGUARD__UTIL__SOCKET_HH_
#define HGUARD__UTIL__SOCKET_HH_  //NOLINT
/*--------------------------------------------------------------------------------------*/

#include "Common.hh"

inline namespace MAIN_PACKAGE_NAMESPACE {
namespace util::socket {
/****************************************************************************************/


constexpr int default_sock_type = SOCK_STREAM | SOCK_CLOEXEC;

extern socket_t call_socket_listener(int family, int type = default_sock_type, int protocol = 0) noexcept;
extern socket_t call_socket_connect (int family, int type = default_sock_type, int protocol = 0) noexcept;

extern socket_t open_new_unix_socket(::sockaddr_un const &addr,             int queue = 1, int type = default_sock_type, int proto = 0);
extern socket_t open_new_unix_socket(::sockaddr_un &addr, char const *path, int queue = 1, int type = default_sock_type, int proto = 0);
extern socket_t open_new_unix_socket(                     char const *path, int queue = 1, int type = default_sock_type, int proto = 0);

extern socket_t connect_to_unix_socket(::sockaddr_un const *addr, int type = default_sock_type, int proto = 0);
extern socket_t connect_to_unix_socket(char const *path,          int type = default_sock_type, int proto = 0);

extern socket_t open_new_inet_socket  (::sockaddr const *addr, socklen_t size, int family, int queue = 255, int type = default_sock_type, int proto = 0);
extern socket_t connect_to_inet_socket(::sockaddr const *addr, socklen_t size, int family,                  int type = default_sock_type, int proto = 0);

extern ::addrinfo *resolve_addr(char const *server, char const *port, int type);
extern int         lazy_getsockname(socket_t listener, sockaddr *addr, socklen_t size);
extern void        init_sockaddr_un(::sockaddr_un &addr, char const *path, size_t len);
extern void        unix_socket_connect_pair(socket_t listener, socket_t &acceptor, socket_t &connector, char const *path);


__forceinline void init_sockaddr_un(::sockaddr_un &addr, char const *const path)
{
      init_sockaddr_un(addr, path, strlen(path));
}

template <typename Container>
    requires util::concepts::Integral<typename Container::value_type>
__forceinline void
init_sockaddr_un(::sockaddr_un &addr, Container const &path)
{
      init_sockaddr_un(addr, path.data(), path.size());
}


/****************************************************************************************/
} // namespace util::socket
} // namespace MAIN_PACKAGE_NAMESPACE
#endif
// vim: ft=cpp
