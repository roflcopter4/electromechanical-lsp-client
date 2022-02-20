#pragma once
#ifndef HGUARD__IPC__BASE_WAIT_LOOP_HH_
#define HGUARD__IPC__BASE_WAIT_LOOP_HH_ //NOLINT
/****************************************************************************************/
#include "Common.hh"
#include "ipc/forward.hh"

#include <event2/event.h>
#include <uv.h>

inline namespace emlsp {
namespace ipc {
/****************************************************************************************/


template <class LoopVariant, class ConnectionType,
          class MsgType, class UserDataType = void *>
REQUIRES(IsLoopVariant<LoopVariant>)
class base_loop_template
{
      using this_type = base_loop_template<LoopVariant, ConnectionType,
                                           MsgType, UserDataType>;

    public:
      using client_type     = base_client<LoopVariant, ConnectionType,
                                          MsgType, UserDataType>;
      using connection_type = ConnectionType;
      using loop_variant    = LoopVariant;
      using message_type    = MsgType;
      using user_data_type  = UserDataType;

      friend client_type;

    protected:
      client_type *client_;
      bool         started_ = false;
      std::thread  thrd_;

    public:
      explicit base_loop_template(client_type *client) : client_(client)
      {}
};

template <class LoopVariant, class ConnectionType,
          class MsgType, class UserDataType = void *>
class base_loop
    : public base_loop_template<LoopVariant, ConnectionType, MsgType, UserDataType>
{};


/*--------------------------------------------------------------------------------------*/


template <class ConnectionType, class MsgType, class UserDataType>
class base_loop<loops::libevent, ConnectionType, MsgType, UserDataType>
    : public base_loop_template<loops::libevent, ConnectionType, MsgType, UserDataType>
{
    public:
      using loop_variant = loops::libevent;

    private:
      using this_type   = base_loop<loop_variant, ConnectionType, MsgType, UserDataType>;
      using base_type   = base_loop_template<loop_variant, ConnectionType, MsgType, UserDataType>;
      using client_type = typename base_type::client_type;

      using base_type::client_;
      using base_type::started_;
      using base_type::thrd_;

    protected:
      event_base *base;
    private:
      std::atomic_flag start_flag_ = {};

    public:
      using connection_type = typename base_type::connection_type;
      using message_type    = typename base_type::message_type;
      using user_data_type  = typename base_type::user_data_type;

      explicit base_loop(client_type *client) : base_type(client)
      {
            event_config *cfg = ::event_config_new();
            ::event_config_require_features(cfg, EV_FEATURE_EARLY_CLOSE);
            base = ::event_base_new_with_config(cfg);
            ::event_config_free(cfg);
      }

      ~base_loop()
      {
            ::event_base_free(base);
            this->base     = nullptr;
            this->started_ = false;
      }

      void start(event_callback_fn const cb)
      {
            this->thrd_    = std::thread{startup_wrapper, this, cb};
            this->started_ = true;
      }

      void loopbreak()
      {
            ::event_base_loopbreak(base);
            this->thrd_.join();
      }

    private:
      static size_t available(socket_t const s)
      {
#ifdef _WIN32
            unsigned long value = 0;
            int result = ::ioctlsocket(s, FIONREAD, &value);
#else
            int value  = 0;
            int result = ::ioctl(s, FIONREAD, &value);
#endif

            return (result < 0) ? SIZE_C(0) : static_cast<size_t>(value);
      }

      __attribute__((__artificial__)) inline static void
      startup_wrapper(this_type *client, event_callback_fn const cb) noexcept
      {
            try {
                  client->main_loop(cb);
            } catch (std::exception &e) {
                  DUMP_EXCEPTION(e);
                  ::exit(1);
            }
      }

      void main_loop(event_callback_fn const cb)
      {
            static constexpr timeval    tv   = {.tv_sec = 0, .tv_usec = 100'000} /* 1/10 seconds */;
            static std::atomic_uint64_t iter = UINT64_C(0);

            if (start_flag_.test_and_set())
                  return;

            event *rd_handle = ::event_new(base, this->client_->raw_descriptor(),
                                           EV_READ | EV_PERSIST, cb, this);
            ::event_add(rd_handle, &tv);

            for (;;) {
                  uint64_t const n = iter.fetch_add(1);
                  fflush(stderr);
                  fmt::print(stderr, FC("\033[1;35mHERE IN {}, iter number {}\033[0m\n"),
                             FUNCTION_NAME, n);
                  fflush(stderr);

                  if (::event_base_loop(base, 0) == 1) [[unlikely]] {
                        fmt::print(stderr, "\033[1;31mERROR: No registered events!\033[0m\n");
                        break;
                  }
                  if (::event_base_got_break(base)) [[unlikely]]
                        break;
            }

            ::event_free(rd_handle);
      }

      DELETE_COPY_CTORS(base_loop);
      DEFAULT_MOVE_CTORS(base_loop);
};


/*--------------------------------------------------------------------------------------*/


// TODO
template <class ConnectionType, class MsgType, class UserDataType>
      REQUIRES((false))
class base_loop<loops::asio, ConnectionType, MsgType, UserDataType>
    : public base_loop_template<loops::asio, ConnectionType, MsgType, UserDataType>
{
};


/*--------------------------------------------------------------------------------------*/


// FIXME: BROKEN!
template <class ConnectionType, class MsgType, class UserDataType>
      REQUIRES((false))
class base_loop<loops::libuv, ConnectionType, MsgType, UserDataType>
    : public base_loop_template<loops::libuv, ConnectionType, MsgType, UserDataType>
{
    public:
      using loop_variant = loops::libuv;

    private:
      using this_type   = base_loop<loop_variant, ConnectionType, MsgType, UserDataType>;
      using base_type   = base_loop_template<loop_variant, ConnectionType, MsgType, UserDataType>;
      using client_type = typename base_type::client_type;
      using base_type::client_;
      using base_type::thrd_;

      //pthread_t pid_ = {};

    protected:
      using base_type::started_;

    public:
      using connection_type = typename base_type ::connection_type;
      using message_type    = typename base_type ::message_type;
      using user_data_type  = typename base_type ::user_data_type;

      uv_loop_t   loop_handle{};
      uv_poll_t   poll_handle{};
      uv_signal_t signal_watchers[5]{};

      explicit base_loop(client_type *client) : base_type(client)
      {
            ::uv_loop_init(&loop_handle);
#ifndef _WIN32
            ::uv_signal_init(&loop_handle, &signal_watchers[0]);
            ::uv_signal_init(&loop_handle, &signal_watchers[1]);
#endif
            ::uv_signal_init(&loop_handle, &signal_watchers[2]);
            ::uv_signal_init(&loop_handle, &signal_watchers[3]);
            ::uv_signal_init(&loop_handle, &signal_watchers[4]);

            loop_handle.data        = static_cast<void *>(client_);
            poll_handle.data        = static_cast<void *>(client_);
            signal_watchers[0].data = static_cast<void *>(client_);
            signal_watchers[1].data = static_cast<void *>(client_);
            signal_watchers[2].data = static_cast<void *>(client_);
            signal_watchers[3].data = static_cast<void *>(client_);
            signal_watchers[4].data = static_cast<void *>(client_);
      }

      ~base_loop() { stop(); }

    private:
      static void *pthread_wrapper(void *vdata)
      {
            auto *loop = static_cast<uv_loop_t *>(vdata);
            try {
                  ::uv_run(loop, UV_RUN_DEFAULT);
            } catch (except::connection_closed &e) {
                  fmt::print(stderr, FMT_COMPILE("Caught exception \"{}\" in \"{}\""), e.what(), FUNCTION_NAME);
            }
            //pthread_exit(nullptr);
            return nullptr;
      }

    protected:
      void kill_thread() {
#if 0
            if (pid_ > 0) {
#ifdef _WIN32
                  ::pthread_kill(pid_, SIGHUP);
#else
                  ::pthread_kill(pid_, SIGUSR1);
#endif
                  pid_ = (-1);
            }
#endif
      }

    public:
      void start(uv_poll_cb cb)
      {
#if 0
            if (std::is_same<ConnectionType, lsp::unix_socket_connection>::value)
                  uv_poll_init_socket(&loop_handle, &poll_handle, client_->con()().accepted());
            else
                  uv_poll_init(&loop_handle, &poll_handle, client_->con()().read_fd());
#endif
#if 0
            if (std::is_same<ConnectionType, ipc::unix_socket_connection>::value)
                  uv_poll_init_socket(&loop_handle, &poll_handle, client_->con()().accepted());
            else
                  uv_poll_init(&loop_handle, &poll_handle, client_->con()().read_fd());
            client_->init_uv_poll_handle(&loop_handle, &poll_handle);
#endif

#ifndef _WIN32
            ::uv_signal_start(&signal_watchers[0], &detail::loop_graceful_signal_cb<connection_type, message_type, user_data_type>, SIGUSR1);
            ::uv_signal_start(&signal_watchers[1], &detail::loop_signal_cb, SIGPIPE);
#endif
            ::uv_signal_start(&signal_watchers[2], &detail::loop_signal_cb, SIGINT);
            ::uv_signal_start(&signal_watchers[3], &detail::loop_graceful_signal_cb<connection_type, message_type, user_data_type>, SIGHUP);
            ::uv_signal_start(&signal_watchers[4], &detail::loop_signal_cb, SIGTERM);

            ::uv_poll_start(&poll_handle, UV_READABLE, cb);

#if 0
            pthread_attr_t pattr;
            ::pthread_attr_init(&pattr);
            //pthread_attr_setdetachstate(&pattr, 1);
            ::pthread_create(&pid_, &pattr, &pthread_wrapper, &loop_handle);
#endif

            // uv_run(&loop_handle, UV_RUN_DEFAULT);
            // thrd_ = std::thread{uv_run, &loop_handle, UV_RUN_DEFAULT};
            this->started_ = true;
      }

      void stop()
      {
            stop_uv_handles(this->client_);
            // pthread_kill(thrd_.native_handle(), SIGUSR1);
            // thrd_.join();
#if 0
            if (pid_ != (-1))
                  ::pthread_join(pid_, nullptr);
#endif
      }

      static void stop_uv_handles(client_type *data)
      {
            if (!data->loop().started_)
                  return;
            data->loop().kill_thread();
#ifndef _WIN32
            ::uv_signal_stop(&data->loop().signal_watchers[0]);
            ::uv_signal_stop(&data->loop().signal_watchers[1]);
#endif
            ::uv_signal_stop(&data->loop().signal_watchers[2]);
            ::uv_signal_stop(&data->loop().signal_watchers[3]);
            ::uv_signal_stop(&data->loop().signal_watchers[4]);
            ::uv_poll_stop(&data->loop().poll_handle);
            ::uv_stop(&data->loop().loop_handle);

            // ::uv_close(reinterpret_cast<uv_handle_t *>(&data->loop().poll_handle), [](uv_handle_t *) { throw except::connection_closed(); });
            ::uv_loop_close(&data->loop().loop_handle);

            data->loop().started_ = false;
      }

      DELETE_COPY_CTORS(base_loop);
      DELETE_MOVE_CTORS(base_loop);
};

#undef TEMPLATE_BS_FULL
#undef TEMPLATE_BS
#undef TEMPLATE_TYPE_BS
#undef TEMPLATE_ALIAS_BS


/****************************************************************************************/


#if 0
template <class ConnectionType, class MsgType>
class basic_event_loop
{
      using this_type = basic_event_loop<ConnectionType, MsgType>;

    public:
      using loop_variant    = loops::libevent;
      using connection_type = ConnectionType;
      using message_type    = MsgType;

    private:
      std::atomic_flag start_flag_ = {};
      std::thread thrd_;
      bool        started_ = false;

    protected:
      event_base *base_;

      friend client_type;

    public:
      TEMPLATE_BASE_ALIAS_BS;

      basic_event_loop()
      {
            event_config *cfg = ::event_config_new();
            ::event_config_require_features(cfg, EV_FEATURE_EARLY_CLOSE);
#ifdef _WIN32
            ::event_config_set_flag(cfg, EVENT_BASE_FLAG_STARTUP_IOCP);
#endif
            base_ = ::event_base_new_with_config(cfg);
            ::event_config_free(cfg);
      }

      ~basic_event_loop()
      {
            ::event_base_free(base_);
            base_    = nullptr;
            started_ = false;
      }

      void start(event_callback_fn const cb)
      {
            thrd_    = std::thread{startup_wrapper, this, cb};
            started_ = true;
      }

      void loopbreak()
      {
            ::event_base_loopbreak(base_);
            thrd_.join();
      }

    private:
      __attribute__((__artificial__)) inline static void
      startup_wrapper(this_type *client, event_callback_fn const cb) noexcept
      {
            try {
                  client->main_loop(cb);
            } catch (std::exception &e) {
                  DUMP_EXCEPTION(e);
                  ::exit(1);
            }
      }

      void main_loop(event_callback_fn const cb)
      {
            static constexpr timeval    tv   = {.tv_sec = 0, .tv_usec = 100'000} /* 1/10 seconds */;
            static std::atomic_uint64_t iter = UINT64_C(0);

            if (start_flag_.test_and_set())
                  return;

            event *rd_handle = ::event_new(base_, client_->raw_descriptor(), EV_READ|EV_PERSIST, cb, this);
            ::event_add(rd_handle, &tv);

            for (;;) {
                  uint64_t const n = iter.fetch_add(1);
                  fmt::print(FMT_COMPILE("\033[1;35mHERE IN {}, iter number {}\033[0m\n"),
                             FUNCTION_NAME, n);

                  if (::event_base_loop(base_, 0) == 1) [[unlikely]] {
                        fmt::print(stderr, "\033[1;31mERROR: No registered events!\033[0m\n");
                        break;
                  }
                  if (::event_base_got_break(base_)) [[unlikely]]
                        break;
            }

            ::event_free(rd_handle);
      }

      DELETE_COPY_CTORS(basic_event_loop);
      DEFAULT_MOVE_CTORS(basic_event_loop);
};
#endif

/****************************************************************************************/
} // namespace ipc
} // namespace emlsp
#endif // base_wait_loop.hh
