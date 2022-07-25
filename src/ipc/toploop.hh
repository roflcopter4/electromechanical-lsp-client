// ReSharper disable CppRedundantElaboratedTypeSpecifier
#pragma once
#ifndef HGUARD__IPC__TOPLOOP_HH_
#define HGUARD__IPC__TOPLOOP_HH_  //NOLINT

#include "Common.hh"
#include "ipc/basic_protocol_connection.hh"

inline namespace emlsp {
namespace ipc::loop {
/****************************************************************************************/


namespace detail {

class bad_wait final : public std::runtime_error
{
      using base_type = std::runtime_error;
    public:
      template <typename T>
      explicit bad_wait(T const &arg) : base_type(arg) {}
};


template <typename Ptr>
concept ProtocolConnectionVariantPointer =
      util::concepts::SmartPointer<Ptr> &&
      ipc::ProtocolConnectionVariant<typename Ptr::element_type>;

} // namespace detail


//========================================================================================


class main_loop final
{
      using this_type    = main_loop;
      using stopper_fn_t = std::function<void (std::string const &, bool)>;

      static constexpr uint64_t timeout = 1000;
      static constexpr int signals_to_handle[] = {
#ifdef SIGINT
            SIGINT,
#endif
#ifdef SIGHUP
            SIGHUP,
#endif
#ifdef SIGTERM
            SIGTERM,
#endif
#ifdef SIGPIPE
            SIGPIPE,
#endif
#ifdef SIGCHLD
            SIGCHLD,
#endif
      };

#ifdef _WIN32
      static constexpr int timespec_get_base = TIME_UTC;
#else
      static constexpr int timespec_get_base = CLOCK_MONOTONIC;
#endif

      struct uv_variant {
            union {
                  uv_handle_t *handle;
                  uv_stream_t *stream;
                  uv_poll_t   *poll;
                  uv_pipe_t   *pipe;
            };
            bool own = true;
      };

#ifdef _WIN32
      HANDLE thrd_ = INVALID_HANDLE_VALUE;

      SOCKET the_socket_{};
      uv_poll_cb the_cb_{};
      void *the_data_{};
#else
      pthread_t thrd_{};
#endif

      uv_loop_t        base_{};
      uv_timer_t       timer_{};
      std::atomic_bool started_ = false;
      std::mutex       base_mtx_;

      std::recursive_mutex              loop_waiter_mtx_{};
      std::condition_variable           loop_waiter_cnd_{};
      std::map<std::string, uv_variant> handles_{};

      uv_signal_t watchers_[std::size(signals_to_handle)]{};

      //-------------------------------------------------------------------------------

    protected:
      main_loop() noexcept
      {
            ::uv_loop_init(&base_);
            ::uv_timer_init(&base_, &timer_);
            ::uv_timer_set_repeat(&timer_, timeout);
            base_.data = &stopper_fn_;

#if 0
            for (unsigned i = 0; i < std::size(signals_to_handle); ++i) {
                  uv_signal_init(&base_, &watchers_[i]);
                  uv_signal_start(&watchers_[i], sighandler_wrapper,
                                  signals_to_handle[i]);
                  watchers_[i].data = this;
            }
#endif
      }

    public:
      static std::unique_ptr<main_loop> create()
      {
            static std::atomic_flag flg{};
            if (flg.test_and_set(std::memory_order::seq_cst))
                  return nullptr;
            return create_impl();
      }

      ~main_loop() noexcept
      {
            try {
                  if (started_)
                        loop_stop();
            } catch (std::exception &e) {
                  eprintf("Caught exception in destructor! \"%s\"\n", e.what());
            } catch (...) {
                  eprintf("Caught unknown exception in destructor!\n");
            }

            if (started_)
                  ::uv_timer_stop(&timer_);
            ::fflush(stderr);
            ::uv_library_shutdown();
      }

      main_loop(main_loop const &)                = delete;
      main_loop(main_loop &&) noexcept            = delete;
      main_loop &operator=(main_loop const &)     = delete;
      main_loop &operator=(main_loop &&) noexcept = delete;

      //-------------------------------------------------------------------------------

      ND uv_loop_t *base()
      {
            return &base_;
      }

    private:
      static std::unique_ptr<main_loop> create_impl()
      {
            return std::unique_ptr<main_loop>{new main_loop};
      }

      stopper_fn_t stopper_fn_ = [this](std::string const &key, bool stop)
      {
            this->callback_callback(key, stop);
      };

      /********************************************************************************/

      template <ProtocolConnectionVariant T>
      uv_poll_t *
      open_poll_handle_fd(std::string const &name, int const fd, T *connection)
      {
            std::lock_guard<std::mutex> lock(base_mtx_);
            auto *hand = new uv_poll_t;
            memset(hand, 0, sizeof *hand);
            //uv_variant var{.poll = hand};
            uv_variant var;
            var.poll = hand;
#ifdef _WIN32
            ::uv_poll_init(&base_, hand, fd);
#else
            ::uv_poll_init_socket(&base_, hand, fd);
#endif
            connection->set_key(name);
            hand->data = connection;
            auto const &[it, ok] = handles_.emplace(std::pair{name, var});
            assert(ok != false);

            return it->second.poll;
      }

#ifdef _WIN32
      template <ProtocolConnectionVariant T>
      uv_poll_t *
      open_poll_handle_socket(std::string const &name, SOCKET const fd, T *connection)
      {
# if 0
            the_socket_ = fd;
            the_data_ = connection;
            return nullptr;
# else
            std::lock_guard lock(base_mtx_);
            auto *hand = new uv_poll_t;
            memset(hand, 0, sizeof *hand);
            //uv_variant var{.poll = hand};
            uv_variant var;
            var.poll = hand;
            ::uv_poll_init_socket(&base_, hand, fd);

            connection->set_key(name);
            hand->data = connection;
            auto const &[it, ok] = handles_.emplace(name, var);
            assert(ok != false);

            return it->second.poll;
# endif
      }
#endif

      //-------------------------------------------------------------------------------

    public:
      template <ProtocolConnectionVariant T>
      uv_poll_t *open_poll_handle(std::string const &name, T *connection)
      {
            uv_poll_t *ret;
#ifdef _WIN32
            ret = connection->impl().is_socket()
                      ? open_poll_handle_socket(
                            name, static_cast<SOCKET>(connection->raw_descriptor()), connection)
                      : open_poll_handle_fd(
                            name, static_cast<int>(connection->raw_descriptor()), connection);
#else
            ret = open_poll_handle(std::move(name), connection->raw_descriptor(), connection);
#endif
            return ret;
      }

      template <ProtocolConnectionVariant T>
      uv_poll_t *open_poll_handle(std::string const &name, T &connection)
      {
            return open_poll_handle(name, &connection);
      }

      template <detail::ProtocolConnectionVariantPointer Ptr>
      uv_poll_t *open_poll_handle(std::string const &name, Ptr &connection)
      {
            return open_poll_handle(name, connection.get());
      }

      //-------------------------------------------------------------------------------

      void start_poll_handle(std::string const &key, uv_poll_cb const callback)
      {
#if 0
            the_cb_ = callback;
#else
            std::lock_guard<std::mutex> lock(base_mtx_);
            ::uv_poll_start(handles_[key].poll, UV_READABLE | UV_PRIORITIZED | UV_DISCONNECT, callback);
#endif
      }

      template <ProtocolConnectionVariant T>
      void start_poll_handle(std::string const &key, T const *connection)
      {
            start_poll_handle(key, connection->poll_callback());
      }

      template <ProtocolConnectionVariant T>
      void start_poll_handle(std::string const &key, T const &connection)
      {
            start_poll_handle(key, connection.poll_callback());
      }

      template <detail::ProtocolConnectionVariantPointer Ptr>
      void start_poll_handle(std::string const &key, Ptr &connection)
      {
            start_poll_handle(key, connection->poll_callback());
      }

      /********************************************************************************/

      uv_pipe_t *open_pipe_handle(std::string const        &name,
                                  uv_file const             fd,
                                  basic_protocol_interface *connection)
      {
            std::lock_guard<std::mutex> lock(base_mtx_);
            auto *hand = new uv_pipe_t;
            memset(hand, 0, sizeof *hand);
            uv_variant var{.pipe = hand};
            ::uv_pipe_init(&base_, hand, 0);
            ::uv_pipe_open(hand, fd);

            connection->set_key(name);
            hand->data = connection;
            auto const &[it, ok] = handles_.emplace(std::pair{name, var});
            assert(ok != false);

            return it->second.pipe;
      }

      uv_pipe_t *use_pipe_handle(std::string const        &name,
                                 uv_pipe_t                *hand,
                                 basic_protocol_interface *connection)
      {
            connection->set_key(name);
            uv_variant var{.pipe = hand, .own = false};
            hand->data = connection;
            auto const &[it, ok] = handles_.emplace(std::pair{name, var});
            assert(ok != 0);
            return it->second.pipe;
      }

      template <ProtocolConnectionVariant T>
      uv_pipe_t *use_pipe_handle(std::string const        &name,
                                 std::unique_ptr<T> const &connection)
      {
            return use_pipe_handle(name, connection->impl_libuv().get_uv_handle(),
                                   connection.get());
      }

      //-------------------------------------------------------------------------------

      void start_pipe_handle(std::string const &key,
                             uv_alloc_cb const  alloc_callback,
                             uv_read_cb  const  read_callback)
      {
            std::lock_guard<std::mutex> lock(base_mtx_);
            ::uv_read_start(reinterpret_cast<uv_stream_t *>(handles_.at(key).pipe),
                            alloc_callback, read_callback);
      }

      void start_pipe_handle(std::string const              &key,
                             basic_protocol_interface const *connection)
      {
            std::lock_guard<std::mutex> lock(base_mtx_);
            ::uv_read_start(reinterpret_cast<uv_stream_t *>(handles_.at(key).pipe),
                            connection->pipe_alloc_callback(),
                            connection->pipe_read_callback());
      }

      template <ProtocolConnectionVariant T>
      void start_pipe_handle(std::string const &key, std::unique_ptr<T> const &connection)
      {
            start_pipe_handle(key, connection.get());
      }

      /********************************************************************************/

      void loop_start_async()
      {
            std::lock_guard<std::mutex> lock(base_mtx_);
            if (started_.load())
                  throw std::logic_error("Attempt to loop_start loop twice!");

            ::uv_timer_start(&timer_, &timer_callback, timeout, timeout);

            started_.store(true);
            start_loop_thread();
      }

      void loop_start()
      {
#if 1
            loop_start_async();
            wait();
#else
            started_.store(true);

            auto *fds = new WSAPOLLFD[1];
            fds[0].fd = the_socket_;
            fds[0].events = POLLRDNORM;

            try {
                  for (;;) {
                        int const ret = WSAPoll(fds, 1, WSA_INFINITE);

                        if (ret == SOCKET_ERROR)
                              util::win32::error_exit_wsa(L"WSAPoll()");
                        if (ret == 0)
                              continue;

                        uv_poll_t tmp{};
                        tmp.data = the_data_;
                        auto const re = fds[0].revents;

                        if (re & POLLHUP) {
                              the_cb_(&tmp, 0, UV_DISCONNECT);
                              break;
                        }
                        if (re & POLLERR)
                              util::win32::error_exit_wsa(L"WSAPoll returned POLLERR");
                        if (re & POLLNVAL)
                              util::win32::error_exit_wsa(L"WSAPoll returned POLLNVAL");
                        if (re & POLLIN)
                              the_cb_(&tmp, 0, UV_READABLE);
                  }
            } catch (ipc::except::connection_closed const &e) {
                  throw;
            }

            started_.store(false);
#endif
      }

      //-------------------------------------------------------------------------------

      void loop_stop() noexcept
      {
            if (!base_mtx_.try_lock())
                  return;
            std::unique_lock lock(base_mtx_, std::adopt_lock_t{});

            if (!started_.load())
                  errx_nothrow("Can't stop unstarted loop!\n");
            util::eprint(FC("Stop called, stopping.\n"));

            kill_them_all();
            ::uv_timer_stop(&timer_);
            ::uv_stop(&base_);
            if (int const e = ::uv_loop_close(&base_))
                  util::eprint(FC("uv_loop_close failed with status {} -> {}\n"), e, uv_strerror(e));

            util::eprint(FC("Trying to join...\n"));

            if (errno_t const wait_e = timed_wait(5s); wait_e != 0)
                  force_stop_loop_thread(wait_e);

            util::eprint(FC("Joined.\n"));

#ifndef _WIN32
            /* Why exactly libuv doesn't bother to free these pointers is perhaps a
             * question to which man wasn't meant to know the answer. Alternatively it
             * could be a bug. Who's to say? */
            util::free_all(base_.watchers, base_.internal_fields);
#endif

            started_.store(false);
      }

      void kill_them_all()
      {
            std::lock_guard lock(loop_waiter_mtx_);

            for (auto &[key, hand] : handles_)
            {
                  if (!hand.handle || !hand.handle->data)
                        continue;

                  util::eprint(FC("Stoping (key: {}) {} of type {}, data {}, loop {}\n"),
                               key, static_cast<void const *>(hand.handle),
                               hand.handle->type, hand.handle->data,
                               static_cast<void const *>(hand.handle->loop));

                  delete_handle(hand);
            }

            handles_.clear();
      }

      static void delete_handle(uv_variant &hand)
      {
            if (!hand.handle)
                  return;
            // NOLINTNEXTLINE(clang-diagnostic-switch-enum)
            switch (hand.handle->type) {
                  case UV_NAMED_PIPE: {
                        auto *con = static_cast<connections::libuv_pipe_handle *>(hand.handle->data);
                        con->close();
                        ::uv_read_stop(hand.stream);
                        if (hand.own)
                              delete hand.pipe;
                        hand.pipe = nullptr;
                        break;
                  }
                  case UV_POLL:
                        ::uv_poll_stop(hand.poll);
                        if (hand.own)
                              delete hand.poll;
                        hand.poll = nullptr;
                        break;
                  case UV_STREAM:
                        ::uv_read_stop(hand.stream);
                        if (hand.own)
                              delete hand.stream;
                        hand.stream = nullptr;
                        break;
                  case UV_HANDLE:
                        if (hand.own)
                              delete hand.handle;
                        hand.handle = nullptr;
                        break;
                  default:
                        util::eprint(FC("Got type {} for {} somehow???\n"),
                                     hand.handle->type,
                                     static_cast<void const *>(hand.handle));
                        abort();
                  }
      }

      //-------------------------------------------------------------------------------

#ifdef _WIN32
      void wait()
      {
            check_joinable();
            do_timed_wait(INFINITE);
      }

      void check_joinable() const
      {
            if (!started_.load(std::memory_order::relaxed))
                  errx("Attempt to wait for non-started thread.");
            if (!thrd_ || thrd_ == INVALID_HANDLE_VALUE)
                  errx("Attempt to wait for invalid thread");
      }

      //-------------------------------------------------------------------------------

      template <typename Rep, typename Period>
      errno_t timed_wait(std::chrono::duration<Rep, Period> const &dur)
      {
            check_joinable();
            auto const millis = std::chrono::duration_cast<std::chrono::milliseconds>(dur).count();
            return do_timed_wait(static_cast<DWORD>(millis));
      }

      errno_t timed_wait(struct timespec const *dur)
      {
            check_joinable();
            auto const millis = std::chrono::duration_cast<std::chrono::milliseconds>(
                                    util::timespec_to_duration(dur))
                                    .count();
            return do_timed_wait(static_cast<DWORD>(millis));
      }

    private:
      errno_t do_timed_wait(DWORD const dwtimeout)
      {
            std::lock_guard<std::recursive_mutex> lock(loop_waiter_mtx_);
            auto const ret = ::WaitForSingleObjectEx(thrd_, dwtimeout, true);
            if (ret == 0 && thrd_ != INVALID_HANDLE_VALUE) {
                  ::CloseHandle(thrd_);
                  thrd_ = INVALID_HANDLE_VALUE;
            }
            return ret;
      }

      //-------------------------------------------------------------------------------

      void start_loop_thread()
      {
            thrd_ = reinterpret_cast<HANDLE>(::_beginthreadex(
                nullptr, 0, loop_start_thread_routine, &base_, 0, nullptr));
            if (!thrd_)
                  util::win32::error_exit(L"_beginthreadex()");
#ifdef _MSC_VER
            ::SetThreadDescription(thrd_, L"Main Libuv Thread");
#else
            /* SetThreadDescription isn't exposed by default on MinGW. */
            auto *SetThreadDescription_p =
                util::win32::get_proc_address_module<HRESULT(WINAPI *)(HANDLE, PCWSTR)>(
                    L"Kernel32", "SetThreadDescription");
            SetThreadDescription_p(thrd_, L"Main Libuv Thread");
#endif
      }

      void force_stop_loop_thread(int const wait_e)
      {
            std::lock_guard<std::recursive_mutex> lock(loop_waiter_mtx_);

            if (wait_e == WAIT_TIMEOUT) {
                  util::eprint(FC("Resorting to crying a lot after timeout.\n"));
#ifdef _MSC_VER
                  [this]() -> void {
                        __try {
                              ::TerminateThread(thrd_, 1);
                              do_timed_wait(INFINITE);
                        } __except (EXCEPTION_EXECUTE_HANDLER) {
                              eprintf("Exception code %lu caught\n", GetExceptionCode());
                              ::fflush(stderr);
                        }
                  }();
#else
                  TerminateThread(thrd_, 1);
                  do_timed_wait(INFINITE);
#endif
            } else {
                  util::win32::error_exit_explicit(L"WaitForSingleObject()", wait_e);
            }
      }

      static unsigned loop_start_thread_routine(void *vdata)
      {
            auto *base = static_cast<uv_loop_t *>(vdata);
            ::uv_run(base, UV_RUN_DEFAULT);
            ::_endthreadex(0);
            return 0;
      }

#else //********************************************************************************

      void wait()
      {
            check_joinable();
            pthread_join(thrd_, nullptr);
            thrd_ = {};
      }

      void check_joinable() const
      {
            if (!started_.load(std::memory_order::relaxed))
                  errx("Attempt to wait for non-started thread.");
            if (pthread_equal(thrd_, pthread_self()))
                  throw detail::bad_wait("");
            if (pthread_equal(thrd_, pthread_t{}))
                  errx("Attempt to wait for invalid thread");
      }

      //-------------------------------------------------------------------------------

      template <typename Rep, typename Period>
      errno_t timed_wait(std::chrono::duration<Rep, Period> const &dur) const
      {
            check_joinable();
            struct timespec ts;  //NOLINT(cppcoreguidelines-pro-type-member-init)
            ts = detail::duration_to_timespec(std::chrono::system_clock::now().time_since_epoch() + dur);
            return do_timed_wait(&ts);
      }

      errno_t timed_wait(struct timespec const *dur) const
      {
            check_joinable();
            struct timespec ts;  //NOLINT(cppcoreguidelines-pro-type-member-init)
            ts = detail::duration_to_timespec(std::chrono::system_clock::now().time_since_epoch());
            ts += dur;
            return do_timed_wait(&ts);
      }

    private:
      errno_t do_timed_wait(struct timespec const *timepoint) const
      {
            errno = 0;
            return pthread_timedjoin_np(thrd_, nullptr, timepoint);
      }

      //-------------------------------------------------------------------------------

      void start_loop_thread()
      {
            struct sched_param const sched = {.sched_priority = 1};
            pthread_attr_t           attr;

            pthread_attr_init(&attr);
            pthread_attr_setschedparam(&attr, &sched);
            pthread_attr_setschedpolicy(&attr, SCHED_RR);
            pthread_create(&thrd_, &attr, &loop_start_thread_routine, &base_);
            pthread_attr_destroy(&attr);

      }

      void force_stop_loop_thread(int wait_e)
      {
            if (wait_e == ETIMEDOUT) {
                  util::eprint(FC("Resorting to pthread_cancel after timeout.\n"));
                  pthread_cancel(thrd_);
                  pthread_join(thrd_, nullptr);
                  thrd_ = {};
            } else {
                  errno = wait_e;
                  err("Unexpected error");
            }
      }

      static void *loop_start_thread_routine(void *vdata)
      {
            pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, nullptr);
            ::uv_run(base, UV_RUN_DEFAULT);
            pthread_exit(nullptr);
      }
#endif

      /********************************************************************************/

      static void sighandler_wrapper(_Notnull_ uv_signal_t *handle, int const signum)
      {
            auto *self = static_cast<this_type *>(handle->data);
            self->sighandler(handle, signum);
      }

      void sighandler(UU uv_signal_t *handle, int signum)
      {
            util::eprint(FC("Have signal {} aka SIG{}  ->  {}\n"),
                         signum,
                         util::get_signal_name(signum),
                         util::get_signal_explanation(signum)
            );

            switch (signum) {
            case SIGHUP:
#ifdef SIGPIPE
            case SIGPIPE:
#endif
#ifdef SIGCHLD
            case SIGCHLD:
#endif
                  try {
                        wait();
                  } catch (detail::bad_wait const &) {
                        util::eprint(FC("Caught attempt to wait for myself to die on "
                                        "signal {} aka {}.\n"),
                                     signum, util::get_signal_name(signum));
                  }
                  break;
            case SIGINT:
            case SIGTERM:
                  ::exit(80);
            default:
                  throw std::logic_error(
                      fmt::format("Somehow received unhandled signal number {} ({})",
                                  signum, util::get_signal_name(signum)));
            }
      }

      void callback_callback(std::string const &key, UU bool const stop)
      {
            std::lock_guard<std::recursive_mutex> lock(loop_waiter_mtx_);
            if (!key.empty()) {
                  delete_handle(handles_[key]);
                  handles_.erase(key);
            }
      }

      static void timer_callback(uv_timer_t * /*handle*/)
      {
            /* NOP */
      }
};


/****************************************************************************************/
} // namespace ipc::loop
} // namespace emlsp
#endif
