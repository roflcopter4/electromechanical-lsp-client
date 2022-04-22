#pragma once
#ifndef HGUARD__TOPLOOP_HH_
#define HGUARD__TOPLOOP_HH_  //NOLINT

#include "Common.hh"

inline namespace emlsp {
namespace testing01::loops {
/****************************************************************************************/


class main_loop
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

      uv_loop_t   base_{};
      uv_signal_t watchers_[std::size(signals_to_handle)]{};

      std::mutex       base_mtx_{};
      std::thread      thrd_;
      std::atomic_bool started_ = false;
      std::atomic_uint workers_ = 0;

      union uv_variant {
            uv_handle_t *handle;
            uv_stream_t *stream;
            uv_poll_t   *poll;
            uv_pipe_t   *pipe;

            //operator uv_handle_t *() { return &handle; }
            //operator uv_stream_t *() { return &stream; }
            //operator uv_poll_t *()   { return &poll; }
            //operator uv_pipe_t *()   { return &pipe; }
      };

      std::map<std::string, uv_variant> handles_ {};

    public:
      static std::unique_ptr<main_loop> create()
      {
            static std::once_flag      flg{};
            std::unique_ptr<main_loop> loop = nullptr;
            std::call_once(flg, create_impl, loop);
            return loop;
      }

    private:
      std::function<void()> retardation_ = [this]() { this->loop_stop(); };

      main_loop()
      {
            ::uv_loop_init(&base_);
            base_.data = &retardation_;

            // for (unsigned i = 0; i < std::size(signals_to_handle); ++i) {
            //       uv_signal_init(&base_, &watchers_[i]);
            //       uv_signal_start(&watchers_[i], sighandler_wrapper,
            //                       signals_to_handle[i]);
            //       watchers_[i].data = this;
            // }
      }

      static void create_impl(std::unique_ptr<main_loop> &loop)
      {
            loop = std::unique_ptr<main_loop>(new main_loop());
      }

    public:
      ~main_loop()
      {
            if (started_) {
                  try {
                        loop_stop();
                  } catch (...) {}
                  thrd_.join();
            }

            ::uv_stop(&base_);
            ::uv_loop_close(&base_);

            for (auto &[_, hand] : handles_) {
                  switch (hand.handle->type) {
                  case uv_handle_type::UV_NAMED_PIPE:
                        delete hand.pipe;
                        hand.pipe = nullptr;
                        break;
                  case uv_handle_type::UV_POLL:
                        delete hand.poll;
                        hand.poll = nullptr;
                        break;
                  case uv_handle_type::UV_STREAM:
                        delete hand.stream;
                        hand.stream = nullptr;
                        break;
                  case uv_handle_type::UV_HANDLE:
                        delete hand.handle;
                        hand.handle = nullptr;
                        break;
                  default:
                        abort();
                  }
            }

            handles_.clear();
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
            auto lock = std::lock_guard{base_mtx_};
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
            auto lock = std::lock_guard{base_mtx_};
            auto *hand = new uv_poll_t;
            uv_variant var{.poll = hand};
            ::uv_poll_init_socket(&base_, hand, fd);

            hand->data = data;
            auto const &[it, ok] =handles_.emplace(std::move(name), std::move(var));
            assert(ok != false);

            return it->second.poll;
      }
#endif

      uv_pipe_t *open_pipe_handle(std::string name, void *data)
      {
            auto lock = std::lock_guard{base_mtx_};
            auto *hand = new uv_pipe_t;
            uv_variant var{.pipe = hand};
            uv_pipe_init(&base_, hand, 0);

            hand->data = data;
            auto const &[it, ok] = handles_.emplace(std::pair{std::move(name), var});
            assert(ok != false);

            assert(0);
            return it->second.pipe;
      }

      //-------------------------------------------------------------------------------

      void start_poll_handle(std::string const &key, uv_poll_cb const callback)
      {
            auto lock = std::lock_guard{base_mtx_};
            ::uv_poll_start(handles_[key].poll, UV_READABLE | UV_DISCONNECT, callback);
      }

      void start_pipe_handle(std::string const &key, uv_read_cb const callback)
      {
            assert(0);
            auto lock = std::lock_guard{base_mtx_};
            ::uv_read_start((uv_stream_t *)handles_[key].pipe, nullptr, callback);
      }

      //-------------------------------------------------------------------------------

      void loop_start_async()
      {
            auto lock = std::lock_guard{base_mtx_};
            if (started_.load())
                  throw std::logic_error("Attempt to loop_start loop twice!");

            thrd_ = std::thread(
                [](uv_loop_t *base) {
                      ::uv_run(base, UV_RUN_DEFAULT);
                }, &base_
            );

            started_.store(true);
      }

      void loop_start()
      {
            {
                  auto lock = std::lock_guard{base_mtx_};
                  if (started_.load())
                        throw std::logic_error("Attempt to loop_start loop twice!");
                  started_.store(true);
            }
            ::uv_run(&base_, UV_RUN_DEFAULT);
            started_.store(false);
      }

      void loop_stop()
      {
            auto lock = std::lock_guard{base_mtx_};
            if (!started_.load())
                  throw std::logic_error("Can't loop_stop unstarted loop!");
            //if (--workers_ != 0)
                  //return;

            util::eprint(FC("Stop called, stopping.\n"));

            // for (auto &hand : watchers_)
            //       uv_signal_stop(&hand);

            for (auto &[_, val] : handles_) {
                  switch (val.handle->type) {
                  case UV_POLL:
                        ::uv_poll_stop(val.poll);
                        break;
                  case UV_STREAM:
                  case UV_NAMED_PIPE:
                        ::uv_read_stop(val.stream);
                        break;
                  default:
                        abort();
                  }
            }

            ::uv_stop(&base_);
            started_.store(false);
      }

      //-------------------------------------------------------------------------------

    private:
      static void sighandler_wrapper(uv_signal_t *handle, int signum)
      {
            auto *self = static_cast<this_type *>(handle->data);
            self->sighandler(handle, signum);
      }

      void sighandler(UNUSED uv_signal_t *handle, int signum)
      {
            util::eprint(FC("Have signal {} aka SIG{}  ->  {}\n"),
                         signum,
                         util::get_signal_name(signum),
                         util::get_signal_explanation(signum)
                         //::sigabbrev_np(signum),
                         //::sigdescr_np(signum)
                         );

            switch (signum)
            {
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
}; // class main_loop


/****************************************************************************************/
} // namespace testing01::loops
} // namespace emlsp
#endif
