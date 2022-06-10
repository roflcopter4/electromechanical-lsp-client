#pragma once
#ifndef HGUARD__IPC__IPC_CONNECTION_HH_
#define HGUARD__IPC__IPC_CONNECTION_HH_ //NOLINT

#include "Common.hh"
#include "ipc/connection_impl.hh"
#include "ipc/connection_interface.hh"

inline namespace emlsp {
namespace ipc {
/****************************************************************************************/


class base_connection
{
      using this_type = base_connection;

    protected:
      ND virtual detail::base_connection_impl_interface       &impl()       & = 0;
      ND virtual detail::base_connection_impl_interface const &impl() const & = 0;

      using procinfo_t   = util::detail::procinfo_t;
      using descriptor_t = detail::descriptor_type;

    public:
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
      virtual int waitpid() noexcept             = 0;

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

      virtual ssize_t raw_write(std::string const &buf)                 { return raw_write(buf.data(), buf.size()); }
      virtual ssize_t raw_write(std::string const &buf, int flags)      { return raw_write(buf.data(), buf.size(), flags); }
      virtual ssize_t raw_write(std::string_view const &buf)            { return raw_write(buf.data(), buf.size()); }
      virtual ssize_t raw_write(std::string_view const &buf, int flags) { return raw_write(buf.data(), buf.size(), flags); }

      virtual ssize_t raw_read(void *buf, size_t const nbytes)
      {
            return impl().read(buf, static_cast<ssize_t>(nbytes));
      }

      virtual ssize_t raw_read(void *buf, size_t const nbytes, int const flags)
      {
            return impl().read(buf, static_cast<ssize_t>(nbytes), flags);
      }

      virtual ssize_t raw_write(void const *buf, size_t const nbytes)
      {
            util::eprint(FC("\033[1;33mwriting {} bytes "
                            "<<'_EOF_'\n\033[0;32m{}\033[1;33m\n_EOF_\033[0m\n"),
                         nbytes,
                         std::string_view(static_cast<char const *>(buf), nbytes));
            return impl().write(buf, static_cast<ssize_t>(nbytes));
      }

      virtual ssize_t raw_write(void const *buf, size_t const nbytes, int const flags)
      {
            return impl().write(buf, static_cast<ssize_t>(nbytes), flags);
      }

      virtual ssize_t raw_writev(void const *buf, size_t const nbytes, int const flags)
      {
            return impl().write(buf, static_cast<ssize_t>(nbytes), flags);
      }

      //--------------------------------------------------------------------------------

      ND virtual size_t available() const
      {
            return impl().available();
      }

      ND virtual intptr_t raw_descriptor() const
      {
            auto const fd = impl().fd();
#ifdef _WIN32
            if constexpr (std::is_integral_v<decltype(fd)>)
                  return static_cast<descriptor_type>(impl().fd());
            else
                  return reinterpret_cast<descriptor_type>(impl().fd());
#else
            return fd;
#endif
      }
};


/****************************************************************************************/
/****************************************************************************************/


namespace detail {


// template <typename ConnectionImpl>
//       REQUIRES(detail::ConnectionImplVariant<ConnectionImpl>)
class spawner_connection : public base_connection
{
      using this_type = spawner_connection;
      using base_type = base_connection;

    public:
      spawner_connection()           = default;
      ~spawner_connection() override = default;

      DELETE_ALL_CTORS(spawner_connection);

      //--------------------------------------------------------------------------------

      virtual void redirect_stderr_to_default()
      {
            impl().set_stderr_default();
      }

      virtual void redirect_stderr_to_devnull()
      {
            impl().set_stderr_devnull();
      }

      virtual void redirect_stderr_to_fd(descriptor_t fd)
      {
            impl().set_stderr_fd(fd);
      }

      virtual void redirect_stderr_to_filename(std::string const &fname)
      {
            impl().set_stderr_filename(fname);
      }

      //--------------------------------------------------------------------------------

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

      virtual procinfo_t spawn_connection(size_t argc, char **argv) = 0;
      virtual void kill() = 0;

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
      REQUIRES(ConnectionImplVariant<ConnectionImpl>)
class spawner_connection : public ipc::detail::spawner_connection
{
      ConnectionImpl impl_;

      union {
            procinfo_t    pid_;
            uv_process_t *libuv_process_handle_;
      } process{};

      bool owns_process = true;

      static constexpr bool is_uv_pipe_ =
          std::is_same_v<ConnectionImpl, detail::libuv_pipe_handle_impl>;

    public:
      spawner_connection() = default;
      ~spawner_connection() override
      {
            close();
      }

      DELETE_ALL_CTORS(spawner_connection);

      using connection_impl_type = ConnectionImpl;

      //--------------------------------------------------------------------------------

      ND detail::base_connection_impl_interface       &impl()       & final { return impl_; }
      ND detail::base_connection_impl_interface const &impl() const & final { return impl_; }

      //--------------------------------------------------------------------------------


      procinfo_t spawn_connection(size_t argc, char **argv) override
      {
            if constexpr (is_uv_pipe_) {
                  auto &hand = dynamic_cast<ipc::detail::libuv_pipe_handle_impl &>(impl());
                  std::ignore = hand.do_spawn_connection(argc, argv);
                  process.libuv_process_handle_ = hand.get_uv_process_handle();
            } else {
                  process.pid_ = this->impl().do_spawn_connection(argc, argv);
            }

            return process.pid_;
      }

      void close() override
      {
            kill();
      }

      void kill() override
      {
            if constexpr (is_uv_pipe_) {
                  if (process.libuv_process_handle_) {
                        ::uv_process_kill(process.libuv_process_handle_, SIGTERM);
                        process.libuv_process_handle_ = nullptr;
                  }
            } else {
                  if (proc_probably_exists(process.pid_))
                        util::kill_process(process.pid_);
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
                        pinfo.hProcess    = libuv_process_handle_->process_handle;
                        pinfo.hThread     = nullptr;
                        pinfo.dwProcessId = libuv_process_handle_->pid;
                        pinfo.dwThreadId  = 0;
                  }

                  return process.pinfo;
#else
                  return process.libuv_process_handle_->pid;
#endif
            } else {
                  return process.pid_;
            }
      }

      int waitpid() noexcept final
      {
            if constexpr (is_uv_pipe_) {
                  if (!process.libuv_process_handle_)
                        return -1;
            } else {
                  if (!proc_probably_exists(process.pid_))
                        return -1;
            }
            int status = 0;

#ifdef _WIN32
            HANDLE proc_hand;
            if constexpr (is_uv_pipe_)
                  proc_hand = process.libuv_process_handle_->process_handle;
            else
                  proc_hand = process.pid_.hProcess;

            WaitForSingleObject(proc_hand, INFINITE);
            GetExitCodeProcess (proc_hand, reinterpret_cast<LPDWORD>(&status));

            if (owns_process) {
                  CloseHandle(process.pid_.hThread);
                  CloseHandle(process.pid_.hProcess);
            }
#else
            pid_t proc_id;
            if constexpr (is_uv_pipe_)
                  proc_id = process.libuv_process_handle_->pid;
            else
                  proc_id = process.pid_;

            ::waitpid(proc_id, reinterpret_cast<int *>(&status), 0);
#endif

            process.pid_ = {};
            return status;
      }
};

} // namespace impl


/****************************************************************************************/
/****************************************************************************************/


class stdio_connection : public base_connection
{
      using this_type      = stdio_connection;
      using ConnectionImpl = detail::fd_connection_impl;

      ConnectionImpl impl_;
      procinfo_t     parent_pid_;


    public:
      using connection_impl_type = ConnectionImpl;

      explicit stdio_connection()
      {
            impl_.set_descriptors(0, 1);

#ifdef _WIN32
            static_assert(0);
            memset(&parent_pid, 0, sizeof parent_pid_);
            parent_pid_.dwProcessId = ::getppid();
            parent_pid_.hProcess = OpenProcess(
               SYNCHRONIZE | PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
               FALSE, parent_pid_.dwProcessId);
            parent_pid_.hThread = INVALID_HANDLE_VALUE;
#else
            //NOLINTNEXTLINE(cppcoreguidelines-prefer-member-initializer)
            parent_pid_ = ::getppid();
#endif
      }

      ~stdio_connection() noexcept override = default;

      DELETE_ALL_CTORS(stdio_connection);

      //--------------------------------------------------------------------------------

      ND detail::base_connection_impl_interface       &impl()       & final { return impl_; }
      ND detail::base_connection_impl_interface const &impl() const & final { return impl_; }

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
      void kill()
      {
            util::kill_process(parent_pid_);
      }

      int waitpid() noexcept final
      {
            return util::waitpid(parent_pid_);
      }
};


} // namespace detail


/****************************************************************************************/
/****************************************************************************************/


template <typename T>
concept BasicConnectionVariant = std::derived_from<T, base_connection>;


namespace connections {

using std_streams       = detail::stdio_connection;
using pipe              = detail::impl::spawner_connection<detail::pipe_connection_impl>;
using unix_socket       = detail::impl::spawner_connection<detail::unix_socket_connection_impl>;
using inet_ipv4_socket  = detail::impl::spawner_connection<detail::inet_ipv4_socket_connection_impl>;
using inet_ipv6_socket  = detail::impl::spawner_connection<detail::inet_ipv6_socket_connection_impl>;
using inet_socket       = detail::impl::spawner_connection<detail::inet_any_socket_connection_impl>;
using libuv_pipe_handle = detail::impl::spawner_connection<detail::libuv_pipe_handle_impl>;
#if defined _WIN32 && defined WIN32_USE_PIPE_IMPL
using win32_named_pipe  = detail::impl::spawner_connection<dialers::win32_named_pipe>;
using win32_handle_pipe = detail::impl::spawner_connection<dialers::win32_handle_pipe>;
#endif
#ifdef HAVE_SOCKETPAIR
using socketpair        = detail::impl::spawner_connection<detail::socketpair_connection_impl>;
#endif
using Default           = libuv_pipe_handle;

} // namespace connections



/****************************************************************************************/
} // namespace ipc
} // namespace emlsp
#endif
