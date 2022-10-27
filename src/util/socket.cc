#include "Common.hh"
#include "util/socket.hh"

#define AUTOC auto const

#ifdef _WIN32
#  define FATAL_ERROR(msg) win32::error_exit_wsa(L ## msg)
#else
#  define FATAL_ERROR(msg)                                                             \
      do {                                                                             \
            char buf[128];                                                             \
            std::string s_ = std::string(msg) + ": " + ::my_strerror(errno, buf, 128); \
            throw std::runtime_error(s_);                                              \
      } while (0)
#endif

#ifndef O_BINARY
# define O_BINARY 0
#endif
#ifndef SOCK_CLOEXEC
# define SOCK_CLOEXEC 0
#endif

#ifdef _WIN32
# define ERRNO          WSAGetLastError()
# define ERR(t)         util::win32::error_exit_wsa(L"" t)
#else
# define ERRNO errno
# define ERR(t) err(t)
#endif

#ifndef SOCKET_ERROR
# define SOCKET_ERROR (-1)
#endif


inline namespace emlsp {
namespace util::socket {
/****************************************************************************************/


constexpr auto invalid_socket = static_cast<socket_t>(-1);


#ifdef _WIN32
__forceinline socket_t
call_socket_listener(intc family, intc type, intc protocol) noexcept
{
      return ::WSASocketW(family, type, protocol, nullptr, 0,
                          WSA_FLAG_OVERLAPPED | WSA_FLAG_NO_HANDLE_INHERIT);
}
__forceinline socket_t
call_socket_connect(intc family, intc type, intc protocol) noexcept
{
      return ::WSASocketW(family, type, protocol, nullptr, 0,
                          WSA_FLAG_OVERLAPPED | WSA_FLAG_NO_HANDLE_INHERIT);
}
#else
__forceinline socket_t
call_socket_listener(intc family, intc type, intc protocol)
{
      return ::socket(family, type, protocol);
}
__forceinline socket_t
call_socket_connect(intc family, intc type, intc protocol)
{
      return ::socket(family, type, protocol);
}
#endif

/*--------------------------------------------------------------------------------------*/

void
init_sockaddr_un(::sockaddr_un &addr, char const *const path, size_t const len)
{
      if (len >= sizeof(addr.sun_path))
            errx("The file path \"%hs\" (len: %zu) is too large to fit into a UNIX "
                 "socket address (max size: %zu)",
                 path, len + SIZE_C(1), sizeof(addr.sun_path));

      memset(&addr, 0, sizeof(::sockaddr_un));
      addr.sun_family = AF_UNIX;
      memcpy(addr.sun_path, path, len);
}

static __forceinline socket_t
open_socket_wrapper(intc dom, intc type, intc proto)
{
      auto const sock = call_socket_listener(dom, type, proto);
      if (sock == invalid_socket) [[unlikely]]
            ERR("socket");
      return sock;
}

/*--------------------------------------------------------------------------------------*/

socket_t
open_new_unix_socket(::sockaddr_un const &addr,
                     int const            queue,
                     int const            type,
                     int const            proto)
{
      socket_t const sock = open_socket_wrapper(AF_UNIX, type, proto);

      if ( ::bind(sock, (::sockaddr *)&addr, sizeof(::sockaddr_un)) == (-1)) [[unlikely]]
            ERR("bind");
      if (::listen(sock, queue) == (-1)) [[unlikely]]
            ERR("listen");

      return sock;
}

socket_t
open_new_unix_socket(::sockaddr_un &addr, char const *path, intc queue, intc type, intc proto)
{
      init_sockaddr_un(addr, path);
      return open_new_unix_socket(addr, queue, type, proto);
}

socket_t
open_new_unix_socket(char const *path, intc queue, intc type, intc proto)
{
      ::sockaddr_un addr;
      init_sockaddr_un(addr, path);
      return open_new_unix_socket(addr, queue, type, proto);
}

/*--------------------------------------------------------------------------------------*/

socket_t
connect_to_unix_socket(::sockaddr_un const *addr, intc type, intc proto)
{
      socket_t const sock = open_socket_wrapper(AF_UNIX, type, proto);

      if (::connect(sock, (::sockaddr const *)addr, sizeof(::sockaddr_un)) == (-1)) [[unlikely]]
            ERR("connect");

      return sock;
}

socket_t
connect_to_unix_socket(char const *const path, intc type, intc proto)
{
      ::sockaddr_un addr{};
      init_sockaddr_un(addr, path);
      return connect_to_unix_socket(&addr, type, proto);
}

void
unix_socket_connect_pair(socket_t const  listener,
                         socket_t       &acceptor,
                         socket_t       &connector,
                         char const     *path)
{
      ::sockaddr_un con_addr{};
      socklen_t len = sizeof(::sockaddr_un);

      connector = connect_to_unix_socket(path, default_sock_type, 0);
      acceptor  = ::accept(listener, (::sockaddr *)&con_addr, &len);

      if (!acceptor || acceptor == invalid_socket)
            ERR("accept()");
}

/*--------------------------------------------------------------------------------------*/

int
lazy_getsockname(socket_t const listener, sockaddr *addr, socklen_t const size)
{
      socklen_t size_local = size;
      int const ret        = ::getsockname(listener, addr, &size_local);
      if (ret != 0 || size != size_local)
            ERR("getsockname()");
      return ret;
}

socket_t
open_new_inet_socket(sockaddr const *addr,
                     socklen_t const size,
                     int const       family,
                     int const       queue,
                     int const       type,
                     int const       proto)
{
      socket_t const listener = call_socket_listener(family, type, proto);

      if (listener == invalid_socket) [[unlikely]]
            ERR("socket()");
      if (::bind(listener, addr, size) == -1) [[unlikely]]
            ERR("bind()");
      if (::listen(listener, queue) == -1) [[unlikely]]
            ERR("listen()");

      return listener;
}

socket_t
connect_to_inet_socket(sockaddr const *addr,
                       socklen_t const size,
                       int const       family,
                       int const       type,
                       int const       proto)
{
      socket_t const connector = call_socket_connect(family, type, proto);

      if (connector == invalid_socket) [[unlikely]]
            ERR("socket()");
      if (::connect(connector, addr, size) == -1) [[unlikely]]
            ERR("connect()");

      return connector;
}

/*--------------------------------------------------------------------------------------*/

::addrinfo *
resolve_addr(char const *server, char const *port, int const type)
{
      ::addrinfo  hints{};
      ::addrinfo *res = nullptr;

      if (auto const portnum = xatoi(port); portnum > 65535)
            errx("Error: port %ju is not within the range (1, 65535).", portnum);

      hints.ai_flags    = AI_NUMERICSERV | AI_NUMERICHOST;
      hints.ai_socktype = type;

      if (int rc = ::getaddrinfo(server, port, &hints, &res); rc != 0)
#ifdef _WIN32
            errx("Host not found for ('%s') and ('%s') --> %ls",
                 server, port, ::gai_strerrorW(rc));
#else
            errx("Host not found for ('%s') and ('%s') --> %s",
                 server, port, gai_strerror(rc));
#endif

      return res;
}


/****************************************************************************************/
} // namespace util::socket
} // namespace emlsp
