#pragma once
#ifndef HGUARD__TOPLOOP_HH_
#define HGUARD__TOPLOOP_HH_  //NOLINT

#include "Common.hh"
#include "ipc/basic_protocol_connection.hh"

#include <boost/throw_exception.hpp>

inline namespace emlsp {
namespace testing::loops {
/****************************************************************************************/


namespace detail {

class bad_wait : public std::runtime_error
{
      using base_type = std::runtime_error;
    public:
      template <typename T>
      explicit bad_wait(T const &arg) : base_type(arg) {}
};

constexpr void
timespec_add(struct timespec       *const __restrict lval,
             struct timespec const *const __restrict rval)
{
      lval->tv_nsec += rval->tv_nsec;
      lval->tv_sec  += rval->tv_sec;

      if (lval->tv_nsec >= INT64_C(1000000000)) {
            ++lval->tv_sec;
            lval->tv_nsec -= INT64_C(1000000000);
      }
}


template <typename Rep, typename Period>
__forceinline constexpr struct timespec
duration_to_timespec(std::chrono::duration<Rep, Period> const &dur)
{
      struct timespec dur_ts; // NOLINT(*-init)
      auto const secs  = std::chrono::duration_cast<std::chrono::seconds>(dur);
      auto const nsecs = std::chrono::duration_cast<std::chrono::nanoseconds>(dur) -
                         std::chrono::duration_cast<std::chrono::nanoseconds>(secs);
      dur_ts.tv_sec  = secs.count();
      dur_ts.tv_nsec = nsecs.count();
      return dur_ts;
}

} // namespace detail


constexpr auto &operator+=(timespec &lval, timespec const &rval)       { detail::timespec_add(&lval, &rval); return lval; }
constexpr auto &operator+=(timespec &lval, timespec const *const rval) { detail::timespec_add(&lval, rval);  return lval; }


//========================================================================================


class alignas(64) main_loop
{
      using this_type = main_loop;

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

      struct uv_variant {
            union {
                  uv_handle_t *handle;
                  uv_stream_t *stream;
                  uv_poll_t   *poll;
                  uv_pipe_t   *pipe;
            };
            bool own = true;
      };

      uv_loop_t        base_{};
      uv_timer_t       timer_{};
      pthread_t        pthrd_{};
      std::atomic_bool started_ = false;
      std::mutex       base_mtx_;
      std::mutex       loop_waiter_mtx_{};

      std::condition_variable           loop_waiter_cnd_{};
      std::map<std::string, uv_variant> handles_{};

      uv_signal_t watchers_[std::size(signals_to_handle)]{};

    public:
      static std::unique_ptr<main_loop>
      create()
      {
            static std::atomic_flag flg{};
            if (flg.test_and_set(std::memory_order::seq_cst))
                  return nullptr;
            return create_impl();
      }

    private:
      std::function<void(std::string const &, bool)> stopper_fn_ =
          [this](std::string const &key, bool stop)
      {
            this->callback_callback(key, stop);
      };

      static std::unique_ptr<main_loop> create_impl()
      {
            return std::unique_ptr<main_loop>(new main_loop());
      }

    protected:
      main_loop() noexcept
      {
            ::uv_loop_init(&base_);
            ::uv_timer_init(&base_, &timer_);
            ::uv_timer_set_repeat(&timer_, timeout);
            base_.data = &stopper_fn_;

#if 1
            for (unsigned i = 0; i < std::size(signals_to_handle); ++i) {
                  uv_signal_init(&base_, &watchers_[i]);
                  uv_signal_start(&watchers_[i], sighandler_wrapper,
                                  signals_to_handle[i]);
                  watchers_[i].data = this;
            }
#endif
      }

    public:
      virtual ~main_loop() noexcept
      {
            try {
                  if (started_)
                        loop_stop();
            } catch (...) {}

            ::uv_library_shutdown();
      }

      main_loop(main_loop const &)                = delete;
      main_loop(main_loop &&) noexcept            = delete;
      main_loop &operator=(main_loop const &)     = delete;
      main_loop &operator=(main_loop &&) noexcept = delete;

      //-------------------------------------------------------------------------------

      uv_poll_t *open_poll_handle(std::string name, int const fd, void *data)
      {
            std::lock_guard<std::mutex> lock(base_mtx_);
            auto *hand = new uv_poll_t;
            uv_variant var{.poll = hand};
#ifdef _WIN32
            ::uv_poll_init(&base_, hand, fd);
#else
            ::uv_poll_init_socket(&base_, hand, fd);
#endif
            hand->data = data;
            auto const &[it, ok] = handles_.emplace(std::pair{std::move(name), var});
            assert(ok != false);

            return it->second.poll;
      }

#ifdef _WIN32
      uv_poll_t *open_poll_handle(std::string name, SOCKET const fd, void *data)
      {
            std::lock_guard lock(base_mtx_);
            auto *hand = new uv_poll_t;
            uv_variant var{.poll = hand};
            ::uv_poll_init_socket(&base_, hand, fd);

            hand->data = data;
            auto const &[it, ok] = handles_.emplace(std::move(name), var);
            assert(ok != false);

            return it->second.poll;
      }
#endif

      template <typename T> REQUIRES (ipc::BasicProtocolConnectionVariant<T>)
      uv_poll_t *open_poll_handle(std::string name, T *connection)
      {
            connection->set_key(name);
            return open_poll_handle(std::move(name), connection->raw_descriptor(), connection);
      }

      template <typename T> REQUIRES (ipc::BasicProtocolConnectionVariant<T>)
      uv_poll_t *open_poll_handle(std::string name, std::unique_ptr<T> const &connection)
      {
            return open_poll_handle(std::move(name), connection->raw_descriptor(), connection.get());
      }

      void start_poll_handle(std::string const &key, uv_poll_cb const callback)
      {
            std::lock_guard<std::mutex> lock(base_mtx_);
            ::uv_poll_start(handles_[key].poll, UV_READABLE | UV_DISCONNECT, callback);
      }

      template <typename T> REQUIRES (ipc::BasicProtocolConnectionVariant<T>)
      void start_poll_handle(std::string const &key, std::unique_ptr<T> const &connection)
      {
            return start_poll_handle(key, connection->poll_callback());
      }

      //-------------------------------------------------------------------------------

      uv_pipe_t *open_pipe_handle(std::string name, uv_file const fd, void *data)
      {
            std::lock_guard<std::mutex> lock(base_mtx_);
            auto *hand = new uv_pipe_t;
            uv_variant var{.pipe = hand};
            uv_pipe_init(&base_, hand, 0);
            uv_pipe_open(hand, fd);

            hand->data = data;
            auto const &[it, ok] = handles_.emplace(std::pair{std::move(name), var});
            assert(ok != false);

            return it->second.pipe;
      }

      uv_pipe_t *use_pipe_handle(std::string name, uv_pipe_t *hand, void *data)
      {
            uv_variant var{.pipe = hand, .own = false};
            hand->data = data;
            auto const &[it, ok] = handles_.emplace(std::pair{std::move(name), var});
            assert(ok != 0);
            return it->second.pipe;
      }

      template <typename T> REQUIRES (ipc::BasicProtocolConnectionVariant<T>)
      uv_pipe_t *use_pipe_handle(std::string name, std::unique_ptr<T> const &connection)
      {
            connection->set_key(name);
            return use_pipe_handle(std::move(name), connection->impl().get_uv_handle(), connection.get());
      }

      void start_pipe_handle(std::string const &key,
                             uv_alloc_cb const  alloc_callback,
                             uv_read_cb  const  read_callback)
      {
            std::lock_guard<std::mutex> lock(base_mtx_);
            ::uv_read_start(reinterpret_cast<uv_stream_t *>(handles_.at(key).pipe),
                            alloc_callback, read_callback);
      }

      template <typename T> REQUIRES (ipc::BasicProtocolConnectionVariant<T>)
      void start_pipe_handle(std::string const &key, std::unique_ptr<T> const &connection)
      {
            return start_pipe_handle(key, connection.get());
      }

      template <typename T> REQUIRES (ipc::BasicProtocolConnectionVariant<T>)
      void start_pipe_handle(std::string const &key, T *connection)
      {
            std::lock_guard<std::mutex> lock(base_mtx_);
            ::uv_read_start(reinterpret_cast<uv_stream_t *>(handles_.at(key).pipe),
                            connection->pipe_alloc_callback(),
                            connection->pipe_read_callback());
      }

      //-------------------------------------------------------------------------------

      void loop_start_async()
      {
            std::lock_guard<std::mutex> lock(base_mtx_);
            if (started_.load())
                  throw std::logic_error("Attempt to loop_start loop twice!");

            uv_timer_start(&timer_, &timer_callback, timeout, timeout);

            // XXX: Available on Win32???
            struct sched_param sched = {.sched_priority = 1};
            pthread_attr_t     attr;

            pthread_attr_init(&attr);
            pthread_attr_setschedparam(&attr, &sched);
            pthread_attr_setschedpolicy(&attr, SCHED_RR);
            pthread_create(&pthrd_, &attr, &loop_start_pthread_routine, &base_);
            pthread_attr_destroy(&attr);

            started_.store(true);
      }

      void loop_start()
      {
            loop_start_async();
            wait();
#if 0
            std::unique_lock<std::mutex> lock(base_mtx_);
            ::uv_timer_start(&timer_, &timer_callback, timeout, timeout);
            if (started_.load())
                  throw std::logic_error("Attempt to loop_start loop twice!");
            started_.store(true);

            lock.unlock();
            ::uv_run(&base_, UV_RUN_DEFAULT);
            started_.store(false);
#endif
#if 0
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

                        if (re & POLLIN)
                              the_cb_(&tmp, 0, UV_READ);
                        if (re & POLLERR)
                              util::win32::error_exit_wsa(L"WSAPoll returned POLLERR");
                        if (re & POLLNVAL)
                              util::win32::error_exit_wsa(L"WSAPoll returned POLLNVAL");
                        if (re & POLLHUP) {
                              //the_cb_(&tmp, 0, UV_DISCONNECT);
                              //break;
                        }
                  }
            } catch (ipc::except::connection_closed const &e) {
                  throw;
            }

            started_.store(false);
#endif
      }

      //-------------------------------------------------------------------------------

      void loop_stop()
      {
            std::lock_guard<std::mutex> lock(base_mtx_);
            if (!started_.load())
                  throw std::logic_error("Can't stop unstarted loop!");
            util::eprint(FC("Stop called, stopping.\n"));

            ::uv_loop_close(&base_);
            ::uv_stop(&base_);
            util::eprint(FC("Trying to join...\n"));
            
            if (errno_t const wait_e = timed_wait(5s); wait_e != 0)
            {
                  if (wait_e == ETIMEDOUT) {
                        util::eprint(FC("Resorting to pthread_cancel after timeout.\n"));
                        pthread_cancel(pthrd_);
                        pthread_join(pthrd_, nullptr);
                        pthrd_ = {};
                  } else {
                        errno = wait_e;
                        err("Unexpected error");
                  }
            }

            kill_everyone();
            ::uv_timer_stop(&timer_);

            /* Why exactly libuv doesn't bother to free these pointers is perhaps a
             * question to which man wasn't meant to know the answer. Alternatively it
             * could be a bug. Who's to say? */
            util::free_all(base_.watchers, base_.internal_fields);

            started_.store(false);
      }

      void kill_everyone()
      {
            for (auto &[key, hand] : handles_)
            {
                  util::eprint(FC("Stoping (key: {}) {} of type {}, data {}, loop {}\n"),
                               key, static_cast<void const *>(hand.handle),
                               hand.handle->type, hand.handle->data,
                               static_cast<void const *>(hand.handle->loop));

                  if (!hand.handle->data)
                        continue;

                  switch (hand.handle->type) {
                  case UV_NAMED_PIPE: {
                        auto *con = static_cast<ipc::connections::libuv_pipe_handle *>(
                            hand.handle->data);
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

            handles_.clear();
      }

    protected:
      void wait()
      {
            check_joinable();
            pthread_join(pthrd_, nullptr);
            pthrd_ = {};
      }

      //-------------------------------------------------------------------------------

    private:
      void check_joinable() const noexcept(false)
      {
            if (!started_.load(std::memory_order::relaxed))
                  errx("Attempt to wait for non-started thread.");
            if (pthread_equal(pthrd_, pthread_self()))
                  throw detail::bad_wait("");
            if (pthread_equal(pthrd_, pthread_t{}))
                  errx("Attempt to wait for invalid thread");
      }

      errno_t do_timed_wait(struct timespec const *timepoint) const
      {
            errno = 0;
            return pthread_timedjoin_np(pthrd_, nullptr, timepoint);
      }

      template <typename Rep, typename Period>
      errno_t timed_wait(std::chrono::duration<Rep, Period> const &dur) const
      {
            check_joinable();
            struct timespec dur_ts = detail::duration_to_timespec(dur);
            struct timespec ts;  //NOLINT(*-init)
            timespec_get(&ts, CLOCK_MONOTONIC);
            ts += dur_ts;
            return do_timed_wait(&ts);
      }

      errno_t timed_wait(struct timespec const *dur) const
      {
            check_joinable();
            struct timespec ts;  //NOLINT(*-init)
            timespec_get(&ts, CLOCK_MONOTONIC);
            ts += dur;
            return do_timed_wait(&ts);
      }

      static void timer_callback(uv_timer_t * /*handle*/)
      {
            /* NOP */
      }

      static void *loop_start_pthread_routine(void *vdata)
      {
            auto *base = static_cast<uv_loop_t *>(vdata);
            pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, nullptr);
            ::uv_run(base, UV_RUN_DEFAULT);
            pthread_exit(nullptr);
      }

      //-------------------------------------------------------------------------------

      static void sighandler_wrapper(_Notnull_ uv_signal_t *handle, int signum)
      {
            auto *self = static_cast<this_type *>(handle->data);
            self->sighandler(handle, signum);
      }

      void sighandler(UU uv_signal_t *handle, int signum)
      {
            (void)(this);

            util::eprint(FC("Have signal {} aka SIG{}  ->  {}\n"),
                         signum,
                         util::get_signal_name(signum),
                         util::get_signal_explanation(signum)
                         //::sigabbrev_np(signum),
                         //::sigdescr_np(signum)
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
                      fmt::format(FC("Somehow received unhandled signal number {} ({})"),
                                  signum, util::get_signal_name(signum)));
            }
      }

      void callback_callback(std::string const &key, bool const stop)
      {
            if (!key.empty())
                  handles_.erase(key);
            if (stop || handles_.empty())
                  loop_stop();
      }

    public:
      /* Don't rely on this being here. */
      ND uv_loop_t *base() { return &base_; }
}; // class main_loop


/****************************************************************************************/
} // namespace testing::loops
} // namespace emlsp
#endif
