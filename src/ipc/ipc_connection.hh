// ReSharper disable CppFinalFunctionInFinalClass
#pragma once
#ifndef HGUARD__IPC__IPC_CONNECTION_HH_
#define HGUARD__IPC__IPC_CONNECTION_HH_ //NOLINT

#include "Common.hh"
#include "ipc/connection_impl.hh"
#include "ipc/connection_interface.hh"

#ifdef __TAG_HIGHLIGHT__
# define TYPENAME typename
#else
# define TYPENAME util::concepts::StringLiteral
#endif

inline namespace emlsp {
namespace ipc {
/****************************************************************************************/


class base_connection
{
      using this_type = base_connection;
      using iovec = detail::iovec;

    protected:
      using procinfo_t   = util::detail::procinfo_t;
      using descriptor_t = detail::descriptor_type;
      using err_descriptor_type =
          detail::base::base_connection_impl_interface::err_descriptor_type;

    public:
      //--------------------------------------------------------------------------------

      base_connection()          = default;
      virtual ~base_connection() = default;

      DELETE_ALL_CTORS(base_connection);

      //--------------------------------------------------------------------------------

      //virtual void redirect_stderr_to_default()                          = 0;
      //virtual void redirect_stderr_to_devnull()                          = 0;
      //virtual void redirect_stderr_to_fd(descriptor_t fd)                = 0;
      //virtual void redirect_stderr_to_filename(std::string const &fname) = 0;

      ND virtual procinfo_t const &pid() const & = 0;
      virtual void close()                       = 0;
      virtual void kill()                        = 0;
      virtual int  waitpid() noexcept            = 0;

      //--------------------------------------------------------------------------------

      ND virtual detail::base::base_connection_impl_interface       &impl()       & = 0;
      ND virtual detail::base::base_connection_impl_interface const &impl() const & = 0;

      //--------------------------------------------------------------------------------

#if 0
      virtual procinfo_t spawn_connection(size_t argc, char **argv) = 0;

      virtual procinfo_t spawn_connection(char **const argv)
      {
            char **p = argv;
            while (*p++)
                  ;
            return spawn_connection(util::ptr_diff(p, argv), argv);
      }

      virtual procinfo_t spawn_connection(std::vector<char const *> &&vec)
      {
            if (vec[vec.size() - 1] != nullptr)
                  vec.emplace_back(nullptr);
            return spawn_connection(vec.size() - 1, const_cast<char **>(vec.data()));
      }

      virtual procinfo_t spawn_connection(size_t argc, char const **argv) { return spawn_connection(argc, const_cast<char **>(argv)); }
      virtual procinfo_t spawn_connection(char const **argv)              { return spawn_connection(const_cast<char **>(argv)); }
      virtual procinfo_t spawn_connection(char const *const *argv)        { return spawn_connection(const_cast<char **>(argv)); }
      virtual procinfo_t spawn_connection(std::vector<char *> &vec)       { return spawn_connection(reinterpret_cast<std::vector<char const *> &>(vec)); }
      virtual procinfo_t spawn_connection(std::vector<char *> &&vec)      { return spawn_connection(reinterpret_cast<std::vector<char const *> &&>(vec)); }
      virtual procinfo_t spawn_connection(std::vector<char const *> &vec) { return spawn_connection(std::forward<std::vector<char const *> &&>(vec)); }

      /**
       * \brief Spawn a process. This method is To be used much like execl. Unlike execl,
       * no null pointer is required as a sentinel.
       * \tparam Types Must be char const (&)[].
       * \param args  All arguments must be string literals.
       * \return Process id (either pid_t or PROCESS_INFORMATION).
       */
      template <util::concepts::StringLiteral ...Types>
      procinfo_t spawn_connection_l(Types &&...args)
      {
            char const *const pack[] = {args..., nullptr};
            constexpr size_t  argc   = (sizeof(pack) / sizeof(pack[0])) - SIZE_C(1);
            return spawn_connection(argc, const_cast<char **>(pack));
      }
#endif

      //--------------------------------------------------------------------------------

      __forceinline ssize_t raw_write(std::string const &buf)                 { return raw_write(buf.data(), buf.size());        }
      __forceinline ssize_t raw_write(std::string const &buf, int flags)      { return raw_write(buf.data(), buf.size(), flags); }
      __forceinline ssize_t raw_write(std::string_view const &buf)            { return raw_write(buf.data(), buf.size());        }
      __forceinline ssize_t raw_write(std::string_view const &buf, int flags) { return raw_write(buf.data(), buf.size(), flags); }
      __forceinline ssize_t raw_writev(void  *buf, size_t nbytes)             { return raw_writev(buf, nbytes, 0);               }
      __forceinline ssize_t raw_writev(iovec *buf, size_t nbytes)             { return raw_writev(buf, nbytes, 0);               }

      ssize_t raw_read(void *buf, size_t nbytes)
      {
            return impl().read(buf, static_cast<ssize_t>(nbytes));
      }

      ssize_t raw_read(void *buf, size_t nbytes, int flags)
      {
            return impl().read(buf, static_cast<ssize_t>(nbytes), flags);
      }

      ssize_t raw_write(void const *buf, size_t nbytes)
      {
            util::eprint(FC("\033[1;33mwriting {} bytes "
                            "<<'_EOF_'\n\033[0;32m{}\033[1;33m\n_EOF_\033[0m\n"),
                         nbytes,
                         std::string_view(static_cast<char const *>(buf), nbytes));
            return impl().write(buf, static_cast<ssize_t>(nbytes));
      }

      ssize_t raw_write(void const *buf, size_t nbytes, int flags)
      {
            return impl().write(buf, static_cast<ssize_t>(nbytes), flags);
      }

      ssize_t raw_writev(void *buf, size_t nbytes, int flags)
      {
            return impl().writev(static_cast<iovec *>(buf),
                                 static_cast<ssize_t>(nbytes), flags);
      }

      ssize_t raw_writev(iovec *buf, size_t nbytes, int flags)
      {
            return impl().writev(buf, static_cast<ssize_t>(nbytes), flags);
      }

      //--------------------------------------------------------------------------------

      ND size_t available() const
      {
            return impl().available();
      }

      ND intptr_t raw_descriptor() const
      {
            return impl().fd();
      }
};


/****************************************************************************************/
/****************************************************************************************/


namespace detail {


class spawner_connection : public base_connection
{
      using this_type = spawner_connection;
      using base_type = base_connection;

    public:
      spawner_connection()           = default;
      ~spawner_connection() override = default;

      DELETE_ALL_CTORS(spawner_connection);

      //--------------------------------------------------------------------------------

      void redirect_stderr_to_default()
      {
            impl().set_stderr_default();
      }

      void redirect_stderr_to_devnull()
      {
            impl().set_stderr_devnull();
      }

      void redirect_stderr_to_fd(err_descriptor_type const fd)
      {
            impl().set_stderr_fd(fd);
      }

      void redirect_stderr_to_filename(std::string const &fname)
      {
            impl().set_stderr_filename(fname);
      }

      //--------------------------------------------------------------------------------

      ND virtual constexpr base::socket_connection_base_impl       *impl_socket()       = 0;
      ND virtual constexpr base::socket_connection_base_impl const *impl_socket() const = 0;
      ND virtual constexpr base::libuv_pipe_handle_impl            *impl_libuv()        = 0;
      ND virtual constexpr base::libuv_pipe_handle_impl      const *impl_libuv() const  = 0;

#if 0
      /**
       * \brief Get the base implementation as a generic socket interface rather than the
       * fully generic interface. Caution: uses dynamic_cast and therefore has significant
       * overhead. Store the result if you intend to use it more than once to avoid
       * unnecessary duplication of effort. Throws on bad cast.
       * \return impl() dynamically casted to socket_connection_base_impl
       */
      ND constexpr base::socket_connection_base_impl &impl_socket() &
      {
            return dynamic_cast<base::socket_connection_base_impl &>(impl());
      }

      /**
       * \brief Get the base implementation as a generic socket interface rather than the
       * fully generic interface. Caution: uses dynamic_cast and therefore has significant
       * overhead. Store the result if you intend to use it more than once to avoid
       * unnecessary duplication of effort. Throws on bad cast.
       * \return impl() dynamically casted to socket_connection_base_impl
       */
      ND constexpr base::socket_connection_base_impl const &impl_socket() const &
      {
            return dynamic_cast<base::socket_connection_base_impl const &>(impl());
      }

      /**
       * \brief Get the base implementation as a the libuv implenentation type. Caution:
       * uses dynamic_cast and therefore has significant overhead. Store the result if you
       * intend to use it more than once to avoid unnecessary duplication of effort.
       * Throws on bad cast.
       * \return impl() dynamically casted to libuv_pipe_handle_impl
       */
      ND constexpr base::libuv_pipe_handle_impl &impl_libuv() &
      {
            return dynamic_cast<base::libuv_pipe_handle_impl &>(impl());
      }

      /**
       * \brief Get the base implementation as a the libuv implenentation type. Caution:
       * uses dynamic_cast and therefore has significant overhead. Store the result if you
       * intend to use it more than once to avoid unnecessary duplication of effort.
       * Throws on bad cast.
       * \return impl() dynamically casted to libuv_pipe_handle_impl
       */
      ND constexpr base::libuv_pipe_handle_impl const &impl_libuv() const &
      {
            return dynamic_cast<base::libuv_pipe_handle_impl const &>(impl());
      }
#endif

      //--------------------------------------------------------------------------------

      procinfo_t spawn_connection(char **const argv)
      {
            assert(*argv);
            char **p = argv;
            while (*p++)
                  ;
            return spawn_connection(util::ptr_diff(p, argv), argv);
      }

      procinfo_t spawn_connection(std::vector<char const *> &&vec)
      {
            assert(!vec.empty());
            if (vec.back() != nullptr)
                  vec.emplace_back(nullptr);
            return spawn_connection(vec.size() - 1, const_cast<char **>(vec.data()));
      }

      virtual procinfo_t spawn_connection(size_t argc, char const **argv) { return spawn_connection(argc, const_cast<char **>(argv)); }
      virtual procinfo_t spawn_connection(char const **argv)              { return spawn_connection(const_cast<char **>(argv)); }
      virtual procinfo_t spawn_connection(char const *const *argv)        { return spawn_connection(const_cast<char **>(argv)); }
      virtual procinfo_t spawn_connection(std::vector<char *> &vec)       { return spawn_connection(reinterpret_cast<std::vector<char const *> &>(vec)); }
      virtual procinfo_t spawn_connection(std::vector<char *> &&vec)      { return spawn_connection(reinterpret_cast<std::vector<char const *> &&>(vec)); }
      virtual procinfo_t spawn_connection(std::vector<char const *> &vec) { return spawn_connection(std::forward<std::vector<char const *> &&>(vec)); }

      /**
       * \brief Spawn a process. This method is To be used much like execl. Unlike execl,
       * no null pointer is required as a sentinel.
       * \tparam Types Must be char const (&)[].
       * \param args  All arguments must be string literals.
       * \return Process id (either pid_t or PROCESS_INFORMATION).
       */
      template <TYPENAME ...Types>
      procinfo_t spawn_connection_l(Types &&...args)
      {
            char const *const pack[] = {args..., nullptr};
            constexpr size_t  argc   = (sizeof(pack) / sizeof(pack[0])) - SIZE_C(1);
            return spawn_connection(argc, const_cast<char **>(pack));
      }

      virtual procinfo_t spawn_connection(size_t argc, char **argv) = 0;

    protected:
      static bool proc_probably_exists(procinfo_t const &pid)
      {
#ifdef _WIN32
            return pid.dwProcessId != 0;
#else
            return pid != 0;
#endif
      }
};


/****************************************************************************************/


namespace impl {

template <typename ConnectionImpl>
      REQUIRES(detail::base::ConnectionImplVariant<ConnectionImpl>)
class spawner_connection : public detail::spawner_connection
{
      ConnectionImpl impl_;

      union process_info {
            procinfo_t    pid_;
            uv_process_t *libuv_process_handle_;
      } process_{};

      base::socket_connection_base_impl *socket_impl_;
      base::libuv_pipe_handle_impl      *libuv_impl_;

      bool owns_process = true;

      static constexpr bool is_uv_pipe_ =
          std::is_same_v<ConnectionImpl, base::libuv_pipe_handle_impl>;

    public:
      spawner_connection()
      {
            socket_impl_ = dynamic_cast<base::socket_connection_base_impl *>(&impl_);
            libuv_impl_  = dynamic_cast<base::libuv_pipe_handle_impl *>(&impl_);
      }

      ~spawner_connection() override
      {
            close();
      }

      DELETE_ALL_CTORS(spawner_connection);

      using connection_impl_type = ConnectionImpl;

      //--------------------------------------------------------------------------------

      ND base::base_connection_impl_interface       &impl()       & final { return impl_; }
      ND base::base_connection_impl_interface const &impl() const & final { return impl_; }

      //--------------------------------------------------------------------------------

      procinfo_t spawn_connection(size_t argc, char **argv) override
      {
            if constexpr (is_uv_pipe_) {
                  //auto &hand = dynamic_cast<base::libuv_pipe_handle_impl &>(impl());
                  std::ignore = impl_.do_spawn_connection(argc, argv);
                  process_.libuv_process_handle_ = impl_.get_uv_process_handle();
            } else {
                  process_.pid_ = impl_.do_spawn_connection(argc, argv);
            }

            return process_.pid_;
      }

      void close() final
      {
            kill();
      }

      void kill() final
      {
            if constexpr (is_uv_pipe_) {
                  if (process_.libuv_process_handle_) {
                        ::uv_process_kill(process_.libuv_process_handle_, SIGTERM);
                        process_.libuv_process_handle_ = nullptr;
                  }
            } else {
                  if (proc_probably_exists(process_.pid_))
                        util::kill_process(process_.pid_);
            }

            impl().close();
      }

      ND procinfo_t const &pid() const & final
      {
            if constexpr (is_uv_pipe_) {
#ifdef _WIN32
                  static std::mutex          mtx{};
                  static std::atomic_flag    flg{};
                  static PROCESS_INFORMATION pinfo{};

                  std::lock_guard lock(mtx);

                  if (!flg.test_and_set(std::memory_order::relaxed)) {
                        pinfo.hProcess    = process_.libuv_process_handle_->process_handle;
                        pinfo.hThread     = nullptr;
                        pinfo.dwProcessId = process_.libuv_process_handle_->pid;
                        pinfo.dwThreadId  = 0;
                  }

                  return pinfo;
#else
                  return process_.libuv_process_handle_->pid;
#endif
            } else {
                  return process_.pid_;
            }
      }

      int waitpid() noexcept final
      {
            if constexpr (is_uv_pipe_) {
                  if (!process_.libuv_process_handle_)
                        return -1;
            } else {
                  if (!proc_probably_exists(process_.pid_))
                        return -1;
            }
            int status = 0;

#ifdef _WIN32
            HANDLE proc_hand;
            if constexpr (is_uv_pipe_)
                  proc_hand = process_.libuv_process_handle_->process_handle;
            else
                  proc_hand = process_.pid_.hProcess;

            ::WaitForSingleObject(proc_hand, INFINITE);
            ::GetExitCodeProcess (proc_hand, reinterpret_cast<LPDWORD>(&status));

            if (owns_process) {
                  ::CloseHandle(process_.pid_.hThread);
                  ::CloseHandle(process_.pid_.hProcess);
            }
#else
            pid_t proc_id;
            if constexpr (is_uv_pipe_)
                  proc_id = process_.libuv_process_handle_->pid;
            else
                  proc_id = process_.pid_;

            ::waitpid(proc_id, reinterpret_cast<int *>(&status), 0);
#endif

            process_.pid_ = {};
            return status;
      }

      ND constexpr base::socket_connection_base_impl *impl_socket() final
      {
            return socket_impl_;
      }

      ND constexpr base::socket_connection_base_impl const *impl_socket() const final
      {
            return socket_impl_;
      }

      ND constexpr base::libuv_pipe_handle_impl *impl_libuv() final
      {
            return libuv_impl_;
      }

      ND constexpr base::libuv_pipe_handle_impl const *impl_libuv() const final
      {
            return libuv_impl_;
      }
};



/****************************************************************************************/
/****************************************************************************************/


class stdio_connection : public base_connection
{
      using this_type      = stdio_connection;
      using ConnectionImpl = base::fd_connection_impl;

      ConnectionImpl impl_;
      procinfo_t     parent_pid_{};

    public:
      using connection_impl_type = ConnectionImpl;

      explicit stdio_connection()
      {
            impl_.set_descriptors(0, 1);

#ifdef _WIN32
            parent_pid_.dwProcessId = ::getppid();
            parent_pid_.hThread     = INVALID_HANDLE_VALUE;
            parent_pid_.hProcess    = ::OpenProcess(
               SYNCHRONIZE | PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
               false, parent_pid_.dwProcessId
            );
            assert(parent_pid_.hProcess != INVALID_HANDLE_VALUE);
#else
            //NOLINTNEXTLINE(cppcoreguidelines-prefer-member-initializer)
            parent_pid_ = ::getppid();
#endif
      }

      ~stdio_connection() noexcept override = default;

      DELETE_ALL_CTORS(stdio_connection);

      //--------------------------------------------------------------------------------

      ND base::base_connection_impl_interface        &impl()        & final { return impl_; }
      ND base::base_connection_impl_interface const  &impl() const  & final { return impl_; }

      void close() final
      {
            kill();
            waitpid();
      }

      ND procinfo_t const &pid() const & final
      {
            return parent_pid_;
      }

    protected:
      void kill() final
      {
            util::kill_process(parent_pid_);
      }

      int waitpid() noexcept final
      {
            return util::waitpid(parent_pid_);
      }
};


} // namespace impl
} // namespace detail


/****************************************************************************************/
/****************************************************************************************/


template <typename T>
concept BasicConnectionVariant = std::derived_from<T, base_connection>;


namespace connections {

using std_streams       = detail::impl::stdio_connection;
using pipe              = detail::impl::spawner_connection<detail::base::pipe_connection_impl>;
using unix_socket       = detail::impl::spawner_connection<detail::base::unix_socket_connection_impl>;
using inet_ipv4_socket  = detail::impl::spawner_connection<detail::base::inet_ipv4_socket_connection_impl>;
using inet_ipv6_socket  = detail::impl::spawner_connection<detail::base::inet_ipv6_socket_connection_impl>;
using inet_socket       = detail::impl::spawner_connection<detail::base::inet_any_socket_connection_impl>;
using file_descriptors  = detail::impl::spawner_connection<detail::base::fd_connection_impl>;
using libuv_pipe_handle = detail::impl::spawner_connection<detail::base::libuv_pipe_handle_impl>;
#if defined _WIN32 && defined WIN32_USE_PIPE_IMPL
using win32_handle_pipe = detail::impl::spawner_connection<detail::base::pipe_handle_connection_impl>;
using win32_named_pipe  = detail::impl::spawner_connection<detail::base::win32_named_pipe_impl>;
#endif
#ifdef HAVE_SOCKETPAIR
using socketpair        = detail::impl::spawner_connection<detail::socketpair_connection_impl>;
#endif
using Default           = libuv_pipe_handle;

} // namespace connections


#undef TYPENAME

/****************************************************************************************/
} // namespace ipc
} // namespace emlsp
#endif
