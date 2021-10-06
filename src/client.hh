#pragma once
#ifndef HGUARD_d_CLIENT_HH_
#define HGUARD_d_CLIENT_HH_
/****************************************************************************************/

#include "Common.hh"
#include <map>
#include <vector>

namespace emlsp
{

namespace rpc { class base_connection; }

/**
 * TODO: Documentation of some kind...
 *
 * Main class for the client. Ideally this should be more like a struct than a class.
 */
class client
{
    public:
      /* Only gcc insists that this have an initializer. Neither clang nor MSVC care. */
      static constexpr struct info_s {
            static constexpr uint8_t major    = MAIN_PROJECT_VERSION_MAJOR;
            static constexpr uint8_t minor    = MAIN_PROJECT_VERSION_MINOR;
            static constexpr uint8_t patch    = MAIN_PROJECT_VERSION_PATCH;
            static constexpr char    string[] = MAIN_PROJECT_VERSION_STRING;
            static constexpr char    name[]   = MAIN_PROJECT_NAME;
      } info = {};

      enum class client_type {
            NONE, LSP,
      };

    private:
      client_type type_;

    public:
      explicit client(client_type ty) : type_(ty) {}
      virtual ~client() = default;

      ND client_type get_client_type() const { return type_; }

      DEFAULT_COPY_CTORS(client);
      DEFAULT_MOVE_CTORS(client);
};

namespace rpc {

class pipe_connection;
class unix_socket_connection;

namespace lsp {
class pipe_connection;
class unix_socket_connection;
} // namespace lsp


/**
 * Base class for connections. Obviously.
 */
class base_connection : public emlsp::client
{
      friend class lsp::pipe_connection;
      friend class lsp::unix_socket_connection;

    public:
      enum class connection_type {
            UNINITIALIZED = 0,
            PIPES,
            FIFO,
            UNIX_SOCKET,
      };

#ifdef _WIN32
      static int close_socket(socket_t sock) { return closesocket(sock); }
      using procinfo_t = PROCESS_INFORMATION;
#else
      static int close_socket(socket_t sock) { return close(sock); }
      using procinfo_t = pid_t;
#endif

    private:
      connection_type con_type_ = connection_type::UNINITIALIZED;

    protected:
      char const **argv_ = nullptr; //NOLINT
      procinfo_t   pid_{};          //NOLINT

    public:
      explicit base_connection(client_type ty) : client(ty) {}
      explicit base_connection(emlsp::client const &other) : client(other) {}
      ~base_connection() override = default;

      ND connection_type   get_connection_type() const   { return con_type_; }
      ND procinfo_t const &get_procinfo()        const   { return pid_; }
      void set_connection_type(connection_type const ty) { con_type_ = ty; }

      virtual int spawn_connection(size_t argc, char **argv)           = 0;
      virtual int spawn_connection(char **argv)                        = 0;
      virtual int spawn_connection(char const **argv)                  = 0;
      virtual int spawn_connection(size_t argc, char const **argv)     = 0;
      virtual int spawn_connection(std::vector<char *> &vec)           = 0;
      virtual int spawn_connection(std::vector<char const *> &vec)     = 0;
      virtual ssize_t read(void *buf, size_t nbytes)                   = 0;
      virtual ssize_t read(void *buf, size_t nbytes, int flags)        = 0;
      virtual ssize_t write(void const *buf, size_t nbytes)            = 0;
      virtual ssize_t write(void const *buf, size_t nbytes, int flags) = 0;

      DELETE_COPY_CTORS(base_connection);
      DEFAULT_MOVE_CTORS(base_connection);

    protected:
      struct base_socket_connection_s {
            class socket_container_c
            {
                public:
                  socket_t root      = (-1);
                  socket_t accepted  = (-1);
                  socket_t connected = (-1);

                  socket_container_c() = default;
                  ~socket_container_c()
                  {
                        if (connected != (-1))
                              close_socket(connected);
                        if (accepted != (-1))
                              close_socket(accepted);
                        if (root != (-1))
                              close_socket(root);
                  }
                  ND socket_t &operator()() { return accepted; }

                  DELETE_COPY_CTORS(socket_container_c);
                  DEFAULT_MOVE_CTORS(socket_container_c);
            } sock;
      };

      struct unix_socket_connection_s : public base_socket_connection_s {
            std::string path;
            sockaddr_un addr;
            ND socket_t &operator()() { return sock.accepted; }
      };

      struct some_other_socket_connection_s : public base_socket_connection_s {
            /* ... */
      };

      struct pipe_connection_s {
            struct fd_container_s {
                  int read;
                  int write;
            } fd;
      };
};

/**
 * Details for actual connection classes have to be deferred to sub-classes for each RPC
 * protocol. Not much can fit here.
 */
class pipe_connection : public base_connection
{
    protected:
      pipe_connection_s connection{};  //NOLINT

    public:
      explicit pipe_connection(client_type ty) : base_connection(ty) {}
      explicit pipe_connection(emlsp::client const &other) : base_connection(other) {}
      ~pipe_connection() override = default;

      int spawn_connection(char **argv) override;
      int spawn_connection(char const **argv) override;
      int spawn_connection(size_t argc, char **argv) override;
      int spawn_connection(size_t argc, char const **argv) override;
      int spawn_connection(std::vector<char *> &vec) override;
      int spawn_connection(std::vector<char const *> &vec) override;

      ssize_t read(void *buf, size_t nbytes) override;
      ssize_t read(void *buf, size_t nbytes, UNUSED int flags) override;

      ssize_t write(void const *buf, size_t nbytes) override;
      ssize_t write(void const *buf, size_t nbytes, UNUSED int flags) override;

      DELETE_COPY_CTORS(pipe_connection);
      DEFAULT_MOVE_CTORS(pipe_connection);

      /* All the arguments should be of type 'const char *'. */
      template <typename... Types>
      int spawn_connection_l(UNUSED Types &&...args)
      {
            char const *unpack[] = {args..., nullptr};
            constexpr size_t argc = (sizeof(unpack) / sizeof(unpack[0])) - SIZE_C(1);
            assert(argc >= 1);
            return spawn_connection(argc, const_cast<char **>(unpack));
      }
};

class unix_socket_connection : public base_connection
{
      static socket_t open_new_socket(sockaddr_un &addr, char const *path);
      static socket_t connect_to_socket(sockaddr_un const &addr);

    protected:
      unix_socket_connection_s connection{};  //NOLINT

    public:
      explicit unix_socket_connection(client_type ty) : base_connection(ty) {}
      explicit unix_socket_connection(emlsp::client const &other) : base_connection(other) {}
      ~unix_socket_connection() override = default;

      int spawn_connection(char **argv) override;
      int spawn_connection(char const **argv) override;
      int spawn_connection(size_t argc, char **argv) override;
      int spawn_connection(size_t argc, char const **argv) override;
      int spawn_connection(std::vector<char *> &vec) override;
      int spawn_connection(std::vector<char const *> &vec) override;

      ssize_t read(void *buf, size_t nbytes) override;
      ssize_t read(void *buf, size_t nbytes, int flags) override;

      ssize_t write(void const *buf, size_t nbytes) override;
      ssize_t write(void const *buf, size_t nbytes, int flags) override;

      DELETE_COPY_CTORS(unix_socket_connection);
      DEFAULT_MOVE_CTORS(unix_socket_connection);

      /* All the arguments should be of type 'const char *'. */
      template <typename... Types>
      int spawn_connection_l(UNUSED Types &&...args)
      {
            char const *unpack[] = {args..., nullptr};
            constexpr size_t argc = (sizeof(unpack) / sizeof(unpack[0])) - SIZE_C(1);
            assert(argc >= 1);
            return spawn_connection(argc, const_cast<char **>(unpack));
      }
};

} // namespace rpc
} // namespace emlsp

/****************************************************************************************/
#endif // client.hh
