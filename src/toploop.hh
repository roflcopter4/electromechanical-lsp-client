#pragma once
#ifndef HGUARD__TOPLOOP_HH_
#define HGUARD__TOPLOOP_HH_  //NOLINT

#include "Common.hh"
#include "ipc/basic_protocol_connection.hh"

inline namespace emlsp {
namespace testing01::loops {
/****************************************************************************************/


class alignas(2048) main_loop
{
      using this_type = main_loop;

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

      // uv_signal_t watchers_[std::size(signals_to_handle)]{};

      uv_loop_t               base_{};
      uv_timer_t              timer_{};
      std::thread             thrd_{};
      std::mutex              base_mtx_;
      std::mutex              loop_waiter_mtx_{};
      std::condition_variable loop_waiter_cnd_{};

      struct uv_variant {
            union {
                  uv_handle_t *handle;
                  uv_stream_t *stream;
                  uv_poll_t   *poll;
                  uv_pipe_t   *pipe;
            };
            bool own = true;
      };

      std::map<std::string, uv_variant> handles_ {};

      std::atomic_uint workers_ = 0;
      std::atomic_bool started_ = false;

    public:
      static std::unique_ptr<main_loop> create()
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

      main_loop() noexcept
      {
            ::uv_loop_init(&base_);
            ::uv_timer_init(&base_, &timer_);
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

      static std::unique_ptr<main_loop> create_impl()
      {
            return std::unique_ptr<main_loop>(new main_loop());
      }

    public:
      ~main_loop() noexcept
      {
            if (started_) {
                  try {
                        loop_stop();
                        if (thrd_.joinable())
                              thrd_.join();
                  } catch (...) {}
            }

            ::uv_stop(&base_);

            for (auto &[_, hand] : handles_) {
                  assert(hand.handle != nullptr);
                  switch (hand.handle->type) {
                  case UV_NAMED_PIPE:
                        if (hand.own)
                              delete hand.pipe;
                        hand.pipe = nullptr;
                        break;
                  case UV_POLL:
                        if (hand.own)
                              delete hand.poll;
                        hand.poll = nullptr;
                        break;
                  case UV_STREAM:
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
                        abort();
                  }
            }

            handles_.clear();
            //::uv_loop_close(&base_);
            ::uv_library_shutdown();
      }

      main_loop(main_loop const &)                = delete;
      main_loop(main_loop &&) noexcept            = delete;
      main_loop &operator=(main_loop const &)     = delete;
      main_loop &operator=(main_loop &&) noexcept = delete;

      //-------------------------------------------------------------------------------

      ND uv_loop_t *base() { return &base_; }

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
      //SOCKET the_socket_ = INVALID_SOCKET;
      //uv_poll_cb the_cb_ = nullptr;
      //void *the_data_ = nullptr;

      template <typename T> REQUIRES (ipc::BasicProtocolConnectionVariant<T>)
      uv_poll_t *open_poll_handle(std::string name, T *connection)
      {
            connection->set_key(name);
            return open_poll_handle(std::move(name), connection->raw_descriptor(), connection);
      }

      template <typename T> REQUIRES (ipc::BasicProtocolConnectionVariant<T>)
      uv_poll_t *open_poll_handle(std::string name, std::unique_ptr<T> const &connection)
      {
            //the_socket_ = connection->raw_descriptor();
            //the_cb_ = connection->poll_callback();
            //the_data_ = connection.get();
            //return 0;
            return open_poll_handle(std::move(name), connection->raw_descriptor(), connection.get());
      }

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
            //uv_variant var{.pipe = connection->impl().get_uv_handle(), .own = false};
            //var.pipe->data = connection;
            //auto const &[it, ok] = handles_.emplace(std::pair{std::move(name), var});
            //assert(ok != 0);
            //return it->second.pipe;
      }

      //-------------------------------------------------------------------------------

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

      void start_pipe_handle(std::string const &key,
                             uv_alloc_cb const  alloc_callback,
                             uv_read_cb  const  read_callback)
      {
            std::lock_guard<std::mutex> lock(base_mtx_);
            ::uv_read_start(reinterpret_cast<uv_stream_t *>(handles_[key].pipe),
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
            ::uv_read_start(reinterpret_cast<uv_stream_t *>(handles_[key].pipe),
                            connection->pipe_alloc_callback(),
                            connection->pipe_read_callback());
      }

      //-------------------------------------------------------------------------------

      void loop_start_async()
      {
            std::lock_guard<std::mutex> lock(base_mtx_);
            if (started_.load())
                  throw std::logic_error("Attempt to loop_start loop twice!");

            thrd_ = std::thread(
                [](uv_loop_t *const base) {
                      ::uv_run(base, UV_RUN_DEFAULT);
                }, &base_
            );

            started_.store(true);
      }

      void wait()
      {
            assert(thrd_.joinable());
            thrd_.join();
      }

    private:
      static void timer_callback(UNUSED uv_timer_t *hand)
      {
            util::eprint(FC("Timeout!\n"));
      }

    public:
      void loop_start()
      {
            uv_timer_start(&timer_, &timer_callback, 1000, 1000);
            {
                  std::lock_guard<std::mutex> lock(base_mtx_);
                  if (started_.load())
                        throw std::logic_error("Attempt to loop_start loop twice!");
                  started_.store(true);
            }
            ::uv_run(&base_, UV_RUN_DEFAULT);
            started_.store(false);

            //started_.store(true);

            //auto *fds = new WSAPOLLFD[1];
            //fds[0].fd = the_socket_;
            //fds[0].events = POLLRDNORM;

            //try {
            //      for (;;) {
            //            int const ret = WSAPoll(fds, 1, WSA_INFINITE);

            //            if (ret == SOCKET_ERROR)
            //                  util::win32::error_exit_wsa(L"WSAPoll()");
            //            if (ret == 0)
            //                  continue;

            //            uv_poll_t tmp{};
            //            tmp.data = the_data_;
            //            auto const re = fds[0].revents;

            //            if (re & POLLIN)
            //                  the_cb_(&tmp, 0, UV_READ);
            //            if (re & POLLERR)
            //                  util::win32::error_exit_wsa(L"WSAPoll returned POLLERR");
            //            if (re & POLLNVAL)
            //                  util::win32::error_exit_wsa(L"WSAPoll returned POLLNVAL");
            //            if (re & POLLHUP) {
            //                  //the_cb_(&tmp, 0, UV_DISCONNECT);
            //                  //break;
            //            }
            //      }
            //} catch (ipc::except::connection_closed const &e) {
            //      throw;
            //}

            //started_.store(false);
      }

      void loop_stop()
      {
            std::lock_guard<std::mutex> lock(base_mtx_);
            if (!started_.load())
                  throw std::logic_error("Can't loop_stop unstarted loop!");
            //if (--workers_ != 0)
                  //return;

            util::eprint(FC("Stop called, stopping.\n"));

            // for (auto &hand : watchers_)
            //       uv_signal_stop(&hand);

            for (auto &[key, hand] : handles_) {
                  util::eprint(FC("Stoping (key: {}) {} of type {}, data {}, loop {}\n"),
                               key, static_cast<void const *>(hand.handle), hand.handle->type,
                               hand.handle->data, static_cast<void const *>(hand.handle->loop));
                  if (!hand.handle->data)
                        continue;
                  switch (hand.handle->type) {
                  case UV_NAMED_PIPE: {
                        auto *con = static_cast<
                            ipc::connections::libuv_pipe_handle *>(
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
                                     hand.handle->type, static_cast<void const *>(hand.handle));
                        abort();
                  }
            }

            handles_.clear();
            ::uv_timer_stop(&timer_);
            ::uv_loop_close(&base_);
            ::uv_stop(&base_);

            started_.store(false);

            // TODO Kill the thread if libuv doesn't stop by itself.
      }

      //-------------------------------------------------------------------------------

    private:
      static void sighandler_wrapper(_Notnull_ uv_signal_t *handle, int signum)
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
                  loop_stop();
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
}; // class main_loop


/****************************************************************************************/
} // namespace testing01::loops
} // namespace emlsp
#endif
