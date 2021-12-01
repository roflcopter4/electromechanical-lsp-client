// ReSharper disable CppInconsistentNaming
#pragma once
#ifndef HGUARD_d_CLIENT_HH_
#define HGUARD_d_CLIENT_HH_

#include "Common.hh"
#include "connection_impl.hh"
#include "util/exceptions.hh"
/****************************************************************************************/

namespace emlsp::ipc {

template <typename T>
concept ConnectionImplVariant =    std::same_as<T, emlsp::ipc::detail::unix_socket_connection_impl>
                                || std::same_as<T, emlsp::ipc::detail::pipe_connection_impl>
#ifdef _WIN32
                                || std::same_as<T, emlsp::ipc::detail::win32_named_pipe_impl>
#endif
;

using util::Stringable;

/**
 * TODO: Documentation of some kind...
 */
#if 0
// template <ConnectionImplVariant ConnectionImpl>
template <
    typename ConnectionImpl,
    std::enable_if_t<(std::is_same<ConnectionImpl, emlsp::ipc::detail::unix_socket_connection_impl>::value ||
                      std::is_same<ConnectionImpl, emlsp::ipc::detail::pipe_connection_impl>::value),
                     bool> = true>
template <typename ConnectionImpl>
#endif

template <ConnectionImplVariant ConnectionImpl>
class base_connection
{
    public:
      using connection_type = ConnectionImpl;
      using procinfo_t      = detail::procinfo_t;
      using cstring_uptr    = std::unique_ptr<char[]>;
      using cstring_sptr    = std::shared_ptr<char[]>;

    private:
      procinfo_t     pid_        = {};
      ConnectionImpl connection_ = {};

      void kill(bool const in_destructor)
      {
#ifdef _WIN32
            if (pid_.hProcess) {
                  TerminateProcess(pid_.hProcess, 0);
                  CloseHandle(pid_.hThread);
                  CloseHandle(pid_.hProcess);
            }
#else
            if (pid_) {
                  ::kill(pid_, SIGTERM);
                  ::waitpid(pid_, nullptr, 0);
            }
#endif
            if (!in_destructor) {
                  pid_        = {};
                  connection_ = {};
            }
      }

    public:
      base_connection() = default;
      virtual ~base_connection() noexcept
      {
            try {
                  base_connection::kill(true);
            } catch (...) {}
      }

      explicit base_connection(connection_type &con) : connection_(std::move(con)) {}

      ND connection_type &connection() { return connection_; }
      ND connection_type &operator()() { return connection_; }

      ND connection_type const &connection() const { return connection_; }
      ND connection_type const &operator()() const { return connection_; }
      ND procinfo_t      const &pid()        const { return pid_; }

      void kill() { base_connection::kill(false); }

      procinfo_t spawn_connection(size_t argc, char **argv)
      {
            pid_ = connection_.do_spawn_connection(argc, argv);
            return pid_;
      }

      procinfo_t spawn_connection(size_t const argc, char const **argv)
      {
            return spawn_connection(argc, const_cast<char **>(argv));
      }

      procinfo_t spawn_connection(char **const argv)
      {
            char **p = argv;
            while (*p++)
                  ;
            return spawn_connection(p - argv, argv);
      }

      procinfo_t spawn_connection(char const **const argv)
      {
            return spawn_connection(const_cast<char **>(argv));
      }

      procinfo_t spawn_connection(char const *const *const argv)
      {
            return spawn_connection(const_cast<char **>(argv));
      }

      procinfo_t spawn_connection(std::vector<char *> &vec)
      {
            if (vec[vec.size() - 1] != nullptr)
                  vec.emplace_back(nullptr);
            return spawn_connection(vec.size(), vec.data());
      }

      procinfo_t spawn_connection(std::vector<char const *> &vec)
      {
            if (vec[vec.size() - 1] != nullptr)
                  vec.emplace_back(nullptr);
            return spawn_connection(vec.size(), const_cast<char **>(vec.data()));
      }

      /**
       * To be used much like execl. All arguments must be `char const *`. Unlike execl,
       * no null pointer is required as a sentinel.
       */
      template <Stringable ...Types>
      procinfo_t spawn_connection_l(Types &&...args)
      {
            char const *const pack[] = {args..., nullptr};
            constexpr size_t  argc   = (sizeof(pack) / sizeof(pack[0])) - SIZE_C(1);
            return spawn_connection(argc, const_cast<char **>(pack));
      }

      /**
       * Directly call the connection_'s `read()` method.
       * Params: void *, size_t, int
       */
      template <typename ...Args>
      ssize_t raw_read(Args &&...args)
      {
            return connection_.read(args...);
      }

      /**
       * Directly call the connection_'s `write()` method.
       * Params: void *, size_t, int
       */
      template <typename ...Args>
      ssize_t raw_write(Args &&...args)
      {
            return connection_.write(args...);
      }

      template <size_t N>
      void write_message_l(char const (&msg)[N])
      {
            write_message(msg, sizeof(msg) - SIZE_C(1));
      }

      virtual void write_message(char const *)               = 0;
      virtual void write_message(void const *, size_t)       = 0;
      virtual void write_message(std::string const &)        = 0;
      virtual void write_message(std::vector<char> const &)  = 0;
      virtual size_t            read_message(void **)        = 0;
      virtual size_t            read_message(char **)        = 0;
      virtual size_t            read_message(char *&)        = 0;
      virtual std::vector<char> read_message()               = 0;
      virtual std::string       read_message_string()        = 0;
      virtual size_t            read_message(cstring_uptr &) = 0;
      virtual size_t            read_message(cstring_sptr &) = 0;

      virtual size_t discard_message() = 0;

      base_connection(base_connection const &) = delete;
      base_connection &operator=(base_connection const &) = delete;
      base_connection(base_connection &&) noexcept        = default;
      base_connection &operator=(base_connection &&) noexcept = default;
};

} // namespace emlsp::ipc

/****************************************************************************************/
#endif // client.hh
