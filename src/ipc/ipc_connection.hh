#pragma once
#ifndef HGUARD__IPC__IPC_CONNECTION_HH_
#define HGUARD__IPC__IPC_CONNECTION_HH_ //NOLINT

#include "Common.hh"
#include "ipc/connection_impl.hh"
#include "ipc/connection_interface.hh"

inline namespace emlsp {
namespace ipc {
/****************************************************************************************/


class base_connection : public connection_interface
{
      using this_type      = base_connection;
      using base_interface = connection_interface;

    protected:
      ND virtual detail::base_connection_interface       &impl()       & = 0;
      ND virtual detail::base_connection_interface const &impl() const & = 0;

      virtual void kill() = 0;

    public:
      base_connection()           = default;     
      ~base_connection() override = default;

      DELETE_ALL_CTORS(base_connection);

      //--------------------------------------------------------------------------------

      ssize_t raw_read(void *buf, size_t const nbytes) final
      {
            return impl().read(buf, static_cast<ssize_t>(nbytes));
      }

      ssize_t raw_read(void *buf, size_t const nbytes, int const flags) final
      {
            return impl().read(buf, static_cast<ssize_t>(nbytes), flags);
      }


      ssize_t raw_write(void const *buf, size_t const nbytes) final
      {
            util::eprint(FC("\033[1;33mwriting {} bytes "
                            "<<'_EOF_'\n\033[0;32m{}\033[1;33m\n_EOF_\033[0m\n"),
                         nbytes,
                         std::string_view(static_cast<char const *>(buf), nbytes));
            return impl().write(buf, static_cast<ssize_t>(nbytes));
      }

      ssize_t raw_write(void const *buf, size_t const nbytes, int const flags) final
      {
            return impl().write(buf, static_cast<ssize_t>(nbytes), flags);
      }

      ssize_t raw_writev(void const *buf, size_t const nbytes, int const flags) final
      {
            return impl().write(buf, static_cast<ssize_t>(nbytes), flags);
      }

      ND size_t available() const final
      {
            return impl().available();
      }

      ND intptr_t raw_descriptor() const final
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

      void close() final
      {
            kill();
      }

#if 0
      void redirect_stderr_to_default() override
      {
            impl().set_stderr_default();
      }

      void redirect_stderr_to_devnull() override
      {
            impl().set_stderr_devnull();
      }

      void redirect_stderr_to_fd(descriptor_t fd) override
      {
            impl().set_stderr_fd(fd);
      }

      void redirect_stderr_to_filename(std::string const &fname) override
      {
            impl().set_stderr_filename(fname);
      }
#endif
};


namespace detail {


template <typename ConnectionImpl>
      REQUIRES(detail::ConnectionImplVariant<ConnectionImpl>)
class spawner_connection : public base_connection
{
      using this_type = spawner_connection<ConnectionImpl>;
      using base_type = base_connection;

    public:
      using connection_impl_type = ConnectionImpl;

    private:
      ConnectionImpl impl_{};

      union {
            procinfo_t    pid_;
            uv_process_t *libuv_process_handle_;
      } process{};

      bool owns_process = true;

      static constexpr bool is_uv_pipe_ = std::is_same_v<connection_impl_type, detail::libuv_pipe_handle_impl>;

    public:
      spawner_connection()
      {
            if constexpr (is_uv_pipe_)
                  owns_process = false;
      }

      ~spawner_connection() override
      {
            try {
                  kill();
            } catch (...) {
            }
      }

      DELETE_ALL_CTORS(spawner_connection);

      procinfo_t spawn_connection(size_t argc, char **argv) final
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

      ND detail::base_connection_interface       &impl()       & final { return impl_; }
      ND detail::base_connection_interface const &impl() const & final { return impl_; }

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

      void redirect_stderr_to_default() final
      {
            impl().set_stderr_default();
      }

      void redirect_stderr_to_devnull() final
      {
            impl().set_stderr_devnull();
      }

      void redirect_stderr_to_fd(descriptor_t fd) final
      {
            impl().set_stderr_fd(fd);
      }

      void redirect_stderr_to_filename(std::string const &fname) final
      {
            impl().set_stderr_filename(fname);
      }

    private:
      static bool proc_probably_exists(procinfo_t const &pid)
      {
#ifdef _WIN32
            return pid.dwProcessId != 0;
#else
            return pid != 0;
#endif
      }

    protected:
      void kill() final
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

            impl_.close();
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


class stdio_connection : public base_connection
{
      // using procinfo_t     = detail::procinfo_t;
      using this_type      = stdio_connection;
      using ConnectionImpl = detail::fd_connection_impl;

    public:
      using connection_impl_type = ConnectionImpl;

    private:
      ConnectionImpl impl_;
      procinfo_t     parent_pid_;

    public:
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

      ND detail::base_connection_interface       &impl()       & final { return impl_; }
      ND detail::base_connection_interface const &impl() const & final { return impl_; }

    protected:
      void kill() final
      {
            util::kill_process(parent_pid_);
      }

      int waitpid() noexcept final
      {
            return util::waitpid(parent_pid_);
      }

      ND procinfo_t const &pid() const & final
      {
            return parent_pid_;
      }

    private:
#define NO_IMPLEMENTY()       \
      throw std::logic_error( \
          "This function is meaningless for this class. Calling it is an error.")

      procinfo_t spawn_connection(size_t /*argc*/, char **/*argv*/) final
      {
            NO_IMPLEMENTY();
      }

      void redirect_stderr_to_default() final { NO_IMPLEMENTY(); }
      void redirect_stderr_to_devnull() final { NO_IMPLEMENTY(); }
      void redirect_stderr_to_fd(descriptor_t) final { NO_IMPLEMENTY(); }
      void redirect_stderr_to_filename(std::string const &) final { NO_IMPLEMENTY(); }

#undef NO_IMPLEMENTY
};


} // namespace detail


template <typename T>
concept BasicConnectionVariant = std::derived_from<T, base_connection>;


namespace connections {

using std_streams       = detail::stdio_connection;
using pipe              = detail::spawner_connection<detail::pipe_connection_impl>;
using unix_socket       = detail::spawner_connection<detail::unix_socket_connection_impl>;
using inet_ipv4_socket  = detail::spawner_connection<detail::inet_ipv4_socket_connection_impl>;
using inet_ipv6_socket  = detail::spawner_connection<detail::inet_ipv6_socket_connection_impl>;
using inet_socket       = detail::spawner_connection<detail::inet_any_socket_connection_impl>;
using libuv_pipe_handle = detail::spawner_connection<detail::libuv_pipe_handle_impl>;
#if defined _WIN32 && defined WIN32_USE_PIPE_IMPL
using win32_named_pipe  = detail::spawner_connection<dialers::win32_named_pipe>;
using win32_handle_pipe = detail::spawner_connection<dialers::win32_handle_pipe>;
#endif
#ifdef HAVE_SOCKETPAIR
using socketpair        = detail::spawner_connection<detail::socketpair_connection_impl>;
#endif
using Default           = libuv_pipe_handle;

} // namespace connections



/****************************************************************************************/
} // namespace ipc
} // namespace emlsp
#endif
