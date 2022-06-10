#pragma once
#ifndef HGUARD__IPC__BASIC_DIALER_HH_
#define HGUARD__IPC__BASIC_DIALER_HH_ //NOLINT

#include "Common.hh"
#include "ipc/connection_impl.hh"
#include "ipc/connection_interface.hh"

inline namespace emlsp {
namespace ipc {
/****************************************************************************************/


#if 0
class basic_dialer_interface
{
      using this_type  = basic_dialer_interface;

    protected:
      using procinfo_t = detail::procinfo_t;
#ifdef _WIN32
      using descriptor_t = intptr_t;
#else
      using descriptor_t = int;
#endif

    public:
      basic_dialer_interface()          = default;
      virtual ~basic_dialer_interface() = default;

      DEFAULT_ALL_CTORS(basic_dialer_interface);

      //--------------------------------------------------------------------------------

      virtual void close()                       = 0;
      virtual int  waitpid() noexcept            = 0;
      ND virtual procinfo_t const &pid() const & = 0;

      virtual void redirect_stderr_to_default()                          = 0;
      virtual void redirect_stderr_to_devnull()                          = 0;
      virtual void redirect_stderr_to_fd(descriptor_t fd)                = 0;
      virtual void redirect_stderr_to_filename(std::string const &fname) = 0;
      virtual procinfo_t spawn_connection(size_t argc, char **argv)      = 0;

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
};


template <typename ConnectionImpl>
      REQUIRES (detail::ConnectionImplVariant<ConnectionImpl>)
class basic_dialer
{
      ConnectionImpl impl_;

    public:
      using connection_impl_type = ConnectionImpl;

      basic_dialer()          = default;
      virtual ~basic_dialer() = default;

      DEFAULT_ALL_CTORS(basic_dialer);

      //--------------------------------------------------------------------------------

      // FIXME This should probably be protected
      ND ConnectionImpl       &impl()       & { return impl_; }
      ND ConnectionImpl const &impl() const & { return impl_; }
};
#endif


/*--------------------------------------------------------------------------------------*/


#if 0
template <typename ConnectionImpl>
      REQUIRES (detail::ConnectionImplVariant<ConnectionImpl> && false)
// class spawn_dialer : public basic_dialer<ConnectionImpl>
// class spawn_dialer : public basic_dialer_interface, public connection_impl_wrapper<ConnectionImpl>
class spawn_dialer : public connection_interface, public connection_impl_wrapper<ConnectionImpl>
{

      using this_type      = spawn_dialer<ConnectionImpl>;
      using base_type      = connection_impl_wrapper<ConnectionImpl>;
      using base_interface = connection_interface;

    public:
      using connection_impl_type = ConnectionImpl;
      // using base_type::impl;

    private:
      union {
            procinfo_t    pid_{};
            uv_process_t *libuv_process_handle_;
      };
      bool owns_process = true;

#ifdef _WIN32
      std::unique_ptr<std::thread> process_watcher_ = nullptr;
#endif

      static constexpr bool is_uv_pipe_ = std::is_same_v<connection_impl_type,
                                                         detail::libuv_pipe_handle_impl>;

    public:
      // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init, hicpp-member-init)
      spawn_dialer() : base_interface(), base_type()
      {
            if constexpr (is_uv_pipe_)
                  owns_process = false;
      }

      ~spawn_dialer() override
      {
            try {
                  this->kill();
            } catch (...) {}
      }

      DELETE_ALL_CTORS(spawn_dialer);

      //--------------------------------------------------------------------------------

      // ND ConnectionImpl       &this->impl() &       { return impl(); }
      // ND ConnectionImpl const &this->impl() const & { return impl(); }

      void close() final
      {
            this->kill();
      }

      void redirect_stderr_to_default()                          final { this->impl().set_stderr_default(); }
      void redirect_stderr_to_devnull()                          final { this->impl().set_stderr_devnull(); }
      void redirect_stderr_to_fd(descriptor_t fd)                final { this->impl().set_stderr_fd(fd); }
      void redirect_stderr_to_filename(std::string const &fname) final { this->impl().set_stderr_filename(fname); }

      procinfo_t spawn_connection(size_t argc, char **argv) final
      {
            if constexpr (is_uv_pipe_) {
                  std::ignore = this->impl().do_spawn_connection(argc, argv);
                  libuv_process_handle_ = this->impl().get_uv_process_handle();
            } else {
                  pid_ = this->impl().do_spawn_connection(argc, argv);
            }

            //start_process_watcher();

#ifdef _WIN32
            if (this->impl().spawn_with_shim())
                  this->impl().accept();
#endif

            return pid_;
      }

#if 0
      procinfo_t spawn_connection(size_t const argc, char const **argv) final
      {
            return spawn_connection(argc, const_cast<char **>(argv));
      }

      procinfo_t spawn_connection(char **const argv) final
      {
            char **p = argv;
            while (*p++)
                  ;
            return spawn_connection(util::ptr_diff(p, argv), argv);
      }

      procinfo_t spawn_connection(char const **const argv) final
      {
            return spawn_connection(const_cast<char **>(argv));
      }

      procinfo_t spawn_connection(char const *const *const argv) final
      {
            return spawn_connection(const_cast<char **>(argv));
      }

      procinfo_t spawn_connection(std::vector<char *> &vec) final       { return spawn_connection(reinterpret_cast<std::vector<char const *>  &>(vec)); }
      procinfo_t spawn_connection(std::vector<char *> &&vec) final      { return spawn_connection(reinterpret_cast<std::vector<char const *> &&>(vec)); }
      procinfo_t spawn_connection(std::vector<char const *> &vec) final { return spawn_connection(std::forward<std::vector<char const *> &&>(vec));     }

      procinfo_t spawn_connection(std::vector<char const *> &&vec) final
      {
            if (vec[vec.size() - 1] != nullptr)
                  vec.emplace_back(nullptr);
            return spawn_connection(vec.size() - 1, const_cast<char **>(vec.data()));
      }

#ifdef __TAG_HIGHLIGHT__
      template <typename ...Types>
#else
      /**
       * \brief Spawn a process. This method is To be used much like execl. Unlike execl,
       * no null pointer is required as a sentinel.
       * \tparam Types Must be char const (&)[].
       * \param args  All arguments must be string literals.
       * \return Process id (either pid_t or PROCESS_INFORMATION).
       */
      template <util::concepts::StringLiteral ...Types>
#endif
      /*
       * To be used much like execl. All arguments must be `char const *`.
       * Unlike execl, no null pointer is required as a sentinel.
       */
      procinfo_t spawn_connection_l(Types &&...args)
      {
            char const *const pack[] = {args..., nullptr};
            constexpr size_t  argc   = (sizeof(pack) / sizeof(pack[0])) - SIZE_C(1);
            return spawn_connection(argc, const_cast<char **>(pack));
      }
#endif

      //--------------------------------------------------------------------------------

      ND procinfo_t const &pid() const & final
      {
            return pid_;
      }

      int waitpid() noexcept final
      {
            if constexpr (is_uv_pipe_) {
                  if (!libuv_process_handle_)
                        return -1;
            } else {
                  if (!proc_probably_exists(pid_))
                        return -1;
            }
            int status = 0;

#ifdef _WIN32
            HANDLE proc_hand;
            if constexpr (is_uv_pipe_)
                  proc_hand = libuv_process_handle_->process_handle;
            else
                  proc_hand = pid_.hProcess;

            WaitForSingleObject(proc_hand, INFINITE);
            GetExitCodeProcess (proc_hand, reinterpret_cast<LPDWORD>(&status));

            if (owns_process) {
                  CloseHandle(pid_.hThread);
                  CloseHandle(pid_.hProcess);
            }
#else
            pid_t proc_id;
            if constexpr (is_uv_pipe_)
                  proc_id = libuv_process_handle_->pid;
            else
                  proc_id = pid_;
            ::waitpid(proc_id, reinterpret_cast<int *>(&status), 0);
#endif

            pid_ = {};
            return status;
      }

      //--------------------------------------------------------------------------------

    private:
      static bool proc_probably_exists(procinfo_t const &pid)
      {
#ifdef _WIN32
            return pid.dwProcessId != 0;
#else
            return pid != 0;
#endif
      }

      void kill()
      {
#ifdef _WIN32
            if (process_watcher_) {
# ifdef __GLIBCXX__
                  pthread_cancel(process_watcher_->native_handle());
# else
                  TerminateThread(process_watcher_->native_handle(), 0);
# endif
                  process_watcher_->join();
            }
#endif
            if constexpr (is_uv_pipe_) {
                  if (libuv_process_handle_) {
                        uv_process_kill(libuv_process_handle_, SIGTERM);
                        libuv_process_handle_ = nullptr;
                  }
            } else {
                  if (proc_probably_exists(pid_))
                        util::kill_process(pid_);
            }

            this->impl().close();
      }

      void start_process_watcher()
      {
#ifdef _WIN32
            process_watcher_ = std::make_unique<std::thread>([this]() {
                  WaitForSingleObject(pid_.hProcess, INFINITE);
                  DWORD ret;
                  GetExitCodeProcess(pid_.hProcess, &ret);
                  util::eprint(FC("Process exited with status {}\n"), ret);
            });
#endif
      }

    protected:
         ssize_t   raw_read(void *, size_t)              override {return 0;};
         ssize_t   raw_read(void *, size_t, int)         override {return 0;};
         ssize_t   raw_write(void const *, size_t)       override {return 0;};
         ssize_t   raw_write(void const *, size_t, int)  override {return 0;};
         ssize_t   raw_writev(void const *, size_t, int) override {return 0;};
      ND size_t available() const                        override {return 0;};
      ND intptr_t raw_descriptor() const             override {return 0;};
};


template <> inline detail::procinfo_t const &
spawn_dialer<detail::libuv_pipe_handle_impl>::pid() const &
{
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

      return pinfo;
#else
      return libuv_process_handle_->pid;
#endif
}
#endif


/*--------------------------------------------------------------------------------------*/


WHAT_THE_FUCK()
template <typename ConnectionImpl>
      REQUIRES ( std::same_as<ConnectionImpl, detail::fd_connection_impl> && false )
class std_streams_dialer : public connection_interface,
                           public connection_impl_wrapper<ConnectionImpl>
{
      procinfo_t parent_pid_;

    public:
      using connection_impl_type = ConnectionImpl;

    private:
      using this_type      = std_streams_dialer;
      using base_type      = connection_impl_wrapper<connection_impl_type>;
      using base_interface = connection_interface;

    public:
      // using base_type::impl;

      using base_interface::raw_read;
      using base_interface::raw_write;
      using base_interface::raw_writev;
      using base_interface::available;
      using base_interface::raw_descriptor;
      using base_interface::close;
      using base_interface::waitpid;
      using base_interface::redirect_stderr_to_default;
      using base_interface::redirect_stderr_to_devnull;
      using base_interface::redirect_stderr_to_fd;
      using base_interface::redirect_stderr_to_filename;
      using base_interface::spawn_connection;
      using base_interface::pid;

      std_streams_dialer()
      {
            this->impl().set_descriptors(0, 1);
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

      ~std_streams_dialer() noexcept override = default;

      DELETE_ALL_CTORS(std_streams_dialer);

      //--------------------------------------------------------------------------------

      void close() final 
      {
            util::kill_process(parent_pid_);
      };

      int waitpid() noexcept final
      {
            return util::waitpid(parent_pid_);
      };

      ND procinfo_t const &pid() const & final
      {
            return parent_pid_;
      };

#if 0
    private:
      void redirect_stderr_to_default()                     final {};
      void redirect_stderr_to_devnull()                     final {};
      void redirect_stderr_to_fd(descriptor_t)              final {};
      void redirect_stderr_to_filename(std::string const &) final {};
      procinfo_t spawn_connection(size_t, char **)          final {return {};};
#endif
};


/*--------------------------------------------------------------------------------------*/


#if 0
namespace dialers {
using pipe              = spawn_dialer<detail::pipe_connection_impl>;
using std_streams       = std_streams_dialer;
using inet_ipv4_socket  = spawn_dialer<detail::inet_ipv4_socket_connection_impl>;
using inet_ipv6_socket  = spawn_dialer<detail::inet_ipv6_socket_connection_impl>;
using inet_socket       = spawn_dialer<detail::inet_any_socket_connection_impl>;
using unix_socket       = spawn_dialer<detail::unix_socket_connection_impl>;
using libuv_pipe_handle = spawn_dialer<detail::libuv_pipe_handle_impl>;
#ifdef HAVE_SOCKETPAIR
using socketpair        = spawn_dialer<detail::socketpair_connection_impl>;
#endif
#if defined _WIN32 && defined WIN32_USE_PIPE_IMPL
using win32_named_pipe  = spawn_dialer<detail::win32_named_pipe_impl>;
using win32_handle_pipe = spawn_dialer<detail::pipe_handle_connection_impl>;
#endif

} // namespace dialers
#endif


template <typename T>
concept BasicDialerVariant =
    std::derived_from<T, connection_impl_wrapper<typename T::connection_impl_type>> &&
    std::derived_from<T, connection_interface>;


/****************************************************************************************/
} // namespace ipc
} // namespace emlsp
#endif
