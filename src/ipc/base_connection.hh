// ReSharper disable CppInconsistentNaming
#pragma once
#ifndef HGUARD__IPC__BASE_CONNECTION_HH_
#define HGUARD__IPC__BASE_CONNECTION_HH_

#include "Common.hh"
#include "ipc/connection_impl.hh"
#include "util/exceptions.hh"

inline namespace emlsp {
namespace ipc {
/****************************************************************************************/


template <typename T>
concept ConnectionImplVariant = std::derived_from<T, ipc::detail::base_connection_interface<typename T::descriptor_type>>;
using util::Stringable;

#if 0
template <typename T>
concept ConnectionImplVariant = std::same_as<T, emlsp::ipc::detail::unix_socket_connection_impl>
                                || std::same_as<T, emlsp::ipc::detail::pipe_connection_impl>
#ifdef _WIN32
                                || std::same_as<T, emlsp::ipc::detail::win32_named_pipe_impl>
#endif
;
#endif


#if 0
template <typename ConnectionImpl>
requires(ConnectionImplVariant<ConnectionImpl>)
requires(std::same_as<ConnectionImpl, emlsp::ipc::detail::unix_socket_connection_impl> ||
         std::same_as<ConnectionImpl, emlsp::ipc::detail::pipe_connection_impl>)
#endif
#if 0
template <
    typename ConnectionImpl,
    std::enable_if_t<(std::is_same<ConnectionImpl, emlsp::ipc::detail::unix_socket_connection_impl>::value ||
                      std::is_same<ConnectionImpl, emlsp::ipc::detail::pipe_connection_impl>::value),
                     bool> = true>
#endif
#if 0
template <
    typename ConnectionImpl,
    typename enable_if<is_base_of<ipc::detail::base_connection_interface,
                                  ConnectionImpl>::value, bool>::type = true
>
      requires (std::derived_from<ConnectionImpl, ipc::detail::base_connection_interface>)
#endif


/**
 * TODO: Documentation of some kind...
 */
template <typename ConnectionImpl>
#ifndef __TAG_HIGHLIGHT__
      requires (ConnectionImplVariant<ConnectionImpl>)
#endif
class base_connection
{
    public:
      using connection_type = ConnectionImpl;
      using procinfo_t      = detail::procinfo_t;
      using cstring_uptr    = std::unique_ptr<char[]>;
      using cstring_sptr    = std::shared_ptr<char[]>;

    private:
      procinfo_t pid_ = {};
      std::unique_ptr<ConnectionImpl> connection_ = {};

    protected:
      ND connection_type       *connection()       { return connection_.get(); }
      ND connection_type const *connection() const { return connection_.get(); }

      void kill(bool const in_destructor)
      {
            // wipe_connection();
            connection_->close();
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
                  pid_ = {};
            }
            // connection_.reset();
            // connection_.~ConnectionImpl();
            // connection_ = {};
      }

      void wipe_connection() { connection_ = {}; }

    public:
      base_connection() : connection_(std::make_unique<ConnectionImpl>())
      {}

      virtual ~base_connection() noexcept
      {
            try {
                  base_connection::kill(true);
            } catch (...) {}
      }

      explicit base_connection(connection_type &con) : connection_(std::move(con)) {}

#if 0
      ND connection_type &connection() { return connection_; }
      ND connection_type &operator()() { return connection_; }
      ND connection_type const &connection() const { return connection_; }
      ND connection_type const &operator()() const { return connection_; }
#endif

      procinfo_t spawn_connection(size_t argc, char **argv)
      {
            pid_ = connection_->do_spawn_connection(argc, argv);
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

      /*
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

      /*
       * Directly call the connection_'s `read()` method.
       * Params: void *, size_t, int
       */
      template <typename ...Args>
      ssize_t raw_read(Args &&...args)
      {
            return connection_->read(std::forward<Args>(args)...);
      }
#if 0
      ND ssize_t raw_read(void *buf, ssize_t const nbytes, int const flags)
      {
            if (!connection_) [[unlikely]]
                  throw ipc::except::connection_closed();
            return connection_->read(buf, nbytes, flags);
      }

      ND ssize_t raw_read(void *buf, ssize_t const nbytes)
      {
            return raw_read(buf, nbytes, MSG_WAITALL);
      }
#endif

      /*
       * Directly call the connection_'s `write()` method.
       * Params: void *, size_t, int
       */
      template <typename ...Args>
      ssize_t raw_write(Args &&...args)
      {
            return connection_->write(std::forward<Args>(args)...);
      }

      template <size_t N>
      void write_message_l(char const (&msg)[N])
      {
            write_message(msg, sizeof(msg) - SIZE_C(1));
      }

      void kill() { base_connection::kill(false); }

      void set_stderr_default() { connection_->set_stderr_default(); }
      void set_stderr_devnull() { connection_->set_stderr_devnull(); }
      void set_stderr_filename(std::string const &fname)
      {
            connection_->set_stderr_filename(fname);
      }

      ND procinfo_t const &pid() const { return pid_; }

      ND typename ConnectionImpl::descriptor_type raw_descriptor() const
      {
            return connection_->fd();
      }

      virtual size_t discard_message()                        = 0;
      virtual void   write_message(char const *)              = 0;
      virtual void   write_message(void const *, size_t)      = 0;
      virtual void   write_message(std::string const &)       = 0;
      virtual void   write_message(std::vector<char> const &) = 0;
      virtual size_t            read_message(void **)         = 0;
      virtual size_t            read_message(char **)         = 0;
      virtual size_t            read_message(char *&)         = 0;
      virtual std::vector<char> read_message()                = 0;
      virtual std::string       read_message_string()         = 0;
      virtual size_t            read_message(cstring_uptr &)  = 0;
      virtual size_t            read_message(cstring_sptr &)  = 0;

      base_connection(base_connection const &) = delete;
      base_connection &operator=(base_connection const &) = delete;
      base_connection(base_connection &&) noexcept        = default;
      base_connection &operator=(base_connection &&) noexcept = default;
};


/****************************************************************************************/
} // namespace ipc
} // namespace emlsp
#endif // base_connection.hh
