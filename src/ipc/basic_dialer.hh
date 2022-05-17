#pragma once
#ifndef HGUARD__IPC__BASIC_DIALER_HH_
#define HGUARD__IPC__BASIC_DIALER_HH_ //NOLINT

#include "Common.hh"
#include "ipc/connection_impl.hh"

inline namespace emlsp {
namespace ipc {
/****************************************************************************************/


template <typename ConnectionImpl>
      REQUIRES (detail::ConnectionImplVariant<ConnectionImpl>)
class basic_dialer
{
      ConnectionImpl impl_;

    public:
      using connection_impl_type = ConnectionImpl;

      basic_dialer()          = default;
      virtual ~basic_dialer() = default;

      DELETE_ALL_CTORS(basic_dialer);

      //--------------------------------------------------------------------------------

      virtual void close() = 0;

      // FIXME This should probably be protected
      ND ConnectionImpl       &impl()       & { return impl_; }
      ND ConnectionImpl const &impl() const & { return impl_; }
};


/*--------------------------------------------------------------------------------------*/


template <typename ConnectionImpl>
      REQUIRES (detail::ConnectionImplVariant<ConnectionImpl>)
class spawn_dialer : public basic_dialer<ConnectionImpl>
{
      using procinfo_t = detail::procinfo_t;

    public:
      using this_type            = spawn_dialer<ConnectionImpl>;
      using base_type            = basic_dialer<ConnectionImpl>;
      using connection_impl_type = typename base_type::connection_impl_type;

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
      spawn_dialer() : base_type()
      {
            if constexpr (is_uv_pipe_)
                  owns_process = false;
      }

      virtual ~spawn_dialer() override
      {
            try {
                  this->kill(true);
            } catch (...) {}
      }

      DELETE_ALL_CTORS(spawn_dialer);

      //--------------------------------------------------------------------------------

      void close() override
      {
            this->kill(false);
      }

      template <typename T>
      void set_stderr_fd(T const fd)
      {
            this->impl().set_stderr_fd(fd);
      }

      void set_stderr_default()
      {
            this->impl().set_stderr_default();
      }

      void set_stderr_devnull()
      {
            this->impl().set_stderr_devnull();
      }

      void set_stderr_filename(std::string const &fname)
      {
            this->impl().set_stderr_filename(fname);
      }

      virtual procinfo_t spawn_connection(size_t argc, char **argv)
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

      procinfo_t spawn_connection(size_t const argc, char const **argv)
      {
            return spawn_connection(argc, const_cast<char **>(argv));
      }

      procinfo_t spawn_connection(char **const argv)
      {
            char **p = argv;
            while (*p++)
                  ;
            return spawn_connection(util::ptr_diff(p, argv), argv);
      }

      procinfo_t spawn_connection(char const **const argv)
      {
            return spawn_connection(const_cast<char **>(argv));
      }

      procinfo_t spawn_connection(char const *const *const argv)
      {
            return spawn_connection(const_cast<char **>(argv));
      }

      procinfo_t spawn_connection(std::vector<char *> &vec)       { return spawn_connection(reinterpret_cast<std::vector<char const *>  &>(vec)); }
      procinfo_t spawn_connection(std::vector<char *> &&vec)      { return spawn_connection(reinterpret_cast<std::vector<char const *> &&>(vec)); }
      procinfo_t spawn_connection(std::vector<char const *> &vec) { return spawn_connection(std::forward<std::vector<char const *> &&>(vec));     }

      procinfo_t spawn_connection(std::vector<char const *> &&vec)
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

      //--------------------------------------------------------------------------------

      ND procinfo_t const &pid() const &
      {
            return pid_;
      }

      int waitpid() noexcept
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

      void kill(bool const in_destructor)
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
                  if (proc_probably_exists(pid_)) {
                        util::kill_process(pid_);
                        if (!in_destructor)
                              pid_ = {};
                  }
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


/*--------------------------------------------------------------------------------------*/


WHAT_THE_FUCK()
class std_streams_dialer : public basic_dialer<detail::fd_connection_impl>
{
    public:
      using this_type = std_streams_dialer;
      using base_type = basic_dialer<connection_impl_type>;

      std_streams_dialer()
      {
            impl().set_descriptors(0, 1);
      }

      ~std_streams_dialer() noexcept override = default;

      DELETE_ALL_CTORS(std_streams_dialer);

      //--------------------------------------------------------------------------------

      void close() override {}
};


/*--------------------------------------------------------------------------------------*/


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


template <typename T>
concept BasicDialerVariant =
    std::derived_from<T, basic_dialer<typename T::connection_impl_type>>;


/****************************************************************************************/
} // namespace ipc
} // namespace emlsp
#endif
