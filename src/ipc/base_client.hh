#pragma once
#ifndef HGUARD__IPC__BASE_CLIENT_HH_
#define HGUARD__IPC__BASE_CLIENT_HH_

#include <Common.hh>
#include "ipc/base_connection.hh"
// #include "lsp-protocol/lsp-connection.hh"

#include <event2/event.h>
#include <uv.h>

inline namespace emlsp {
namespace ipc {
/****************************************************************************************/


template <typename T>
concept BaseConnectionVariant =
    std::derived_from<T, ipc::base_connection<typename T::connection_type>>;


namespace detail {

template <typename ConnectionType, typename MsgType, typename UserDataType = void *>
void loop_graceful_signal_cb(uv_signal_t *handle, int signum);

void loop_signal_cb(uv_signal_t *handle, int signum);

} // namespace detail


#if 0
template <
    typename ConnectionType,
    typename MsgType,
    typename UserDataType = void *,
    std::enable_if_t<
        std::is_base_of<base_connection<typename ConnectionType::connection_type>,
                        ConnectionType>::value &&
            std::is_convertible<const volatile ConnectionType *,
                                const volatile base_connection<
                                    typename ConnectionType::connection_type> *>::value,
        bool> = true>
#endif
#if 0
template <detail::BaseConnectionSubclass ConnectionType, typename MsgType, typename UserDataType = void *>
#endif
#if 0
template <typename ConnectionType, typename MsgType, typename UserDataType = void *>
requires detail::BaseConnectionSubclass<ConnectionType>
#endif
#if 0
template <typename ConnectionType, typename MsgType, typename UserDataType = void *,
          typename std::enable_if<
              std::is_base_of<ipc::base_connection<typename ConnectionType::connection_type>,
                                                   ConnectionType>::value,
              bool>::type = true>
#endif


template <typename ConnectionType, typename MsgType, typename UserDataType = void *>
#ifndef __TAG_HIGHLIGHT__
      requires ipc::BaseConnectionVariant<ConnectionType>
#endif
class base_client : public ConnectionType
{
      using atomic_queue = std::queue<std::atomic<MsgType *>>;

    public:
      class event_loop;
      using connection_type = ConnectionType;
      using message_type    = MsgType;
      using user_data_type  = UserDataType;
      using base_type       = ConnectionType;

    protected:
      atomic_queue queue_{};
      event_loop   loop_{this};

      std::mutex                   mtx_ {};
      std::unique_lock<std::mutex> lock_ {mtx_};
      std::condition_variable      condition_ {};

    private:
      UserDataType user_data_{};
      using base_type::kill;

    public:
      base_client() = default;

      ~base_client()
      {
            loop_.loopbreak();
            kill(true);
      }

      DELETE_COPY_CTORS(base_client);
      DEFAULT_MOVE_CTORS(base_client);

      ND UserDataType       &data()       { return user_data_; }
      ND UserDataType const &data() const { return user_data_; }

      ND event_loop       &loop()       { return loop_; }
      ND event_loop const &loop() const { return loop_; }

      ND auto &mtx()         { return mtx_; }
      ND auto &unique_lock() { return lock_; }
      ND auto &condition()   { return condition_; }

      void cond_notify_one() { condition_.notify_one(); }
      void cond_wait() { condition_.wait(lock_); }

      class event_loop
      {
            using client_type = base_client<ConnectionType, MsgType, UserDataType>;

            client_type *client_;
            std::thread  thrd_;
            bool         started_ = false;

          public:
            event_base *base;

            explicit event_loop(client_type *client) : client_(client)
            {
                  event_config *cfg = event_config_new();
                  event_config_require_features(cfg, EV_FEATURE_EARLY_CLOSE);
#ifdef _WIN32
                  event_config_set_flag(cfg, EVENT_BASE_FLAG_STARTUP_IOCP);
#endif
                  base = event_base_new_with_config(cfg);
                  event_config_free(cfg);
            }

            ~event_loop()
            {
                  event_base_free(base);
                  base = nullptr;
            }

            void start(event_callback_fn cb)
            {
                  thrd_ = std::thread{startup_wrapper, client_, base, cb};
            }

            void loopbreak()
            {
                  event_base_loopbreak(base);
                  thrd_.join();
            }

          private:
            static void startup_wrapper(client_type *client, event_base *base, event_callback_fn cb)
            {
                  static std::atomic_uint iter = 0;
                  timeval tv = {.tv_sec = 0, .tv_usec = 100'000};
                  event *rd_handle = event_new(base, client->raw_descriptor(), EV_READ|EV_PERSIST, cb, client);
                  event_add(rd_handle, &tv);

                  for (;;) {
                        fmt::print(FMT_COMPILE("\033[1;35mHERE IN {}, iter number {}\033[0m\n"), __func__, iter.fetch_add(1));
                        if (event_base_loop(base, 0) == 1) {
                              fmt::print(stderr, FMT_COMPILE("\033[1;31mERROR: No registered events!\033[0m\n"));
                              break;
                        }
                        if (event_base_got_break(base))
                              break;
                        // event_add(rd_handle, nullptr);
                        // event_base_dispatch(loop);
                  }

                  event_free(rd_handle);
            }

            DELETE_COPY_CTORS(event_loop);
            DEFAULT_MOVE_CTORS(event_loop);
      };

#if 0
      class event_loop
      {
            using client_type = base_client<ConnectionType, MsgType, UserDataType>;

            client_type *client_;
            std::thread  thrd_;
            bool         started_ = false;
            pthread_t pid_ = {};


          public:
            uv_loop_t   loop_handle{};
            uv_poll_t   poll_handle{};
            uv_signal_t signal_watchers[5]{};

            void fuckme();

            explicit event_loop(client_type *client) : client_(client)
            {
                  uv_loop_init(&loop_handle);
#ifndef _WIN32
                  uv_signal_init(&loop_handle, &signal_watchers[0]);
                  uv_signal_init(&loop_handle, &signal_watchers[1]);
#endif
                  uv_signal_init(&loop_handle, &signal_watchers[2]);
                  uv_signal_init(&loop_handle, &signal_watchers[3]);
                  uv_signal_init(&loop_handle, &signal_watchers[4]);

                  loop_handle.data        = static_cast<void *>(client_);
                  poll_handle.data        = static_cast<void *>(client_);
                  signal_watchers[0].data = static_cast<void *>(client_);
                  signal_watchers[1].data = static_cast<void *>(client_);
                  signal_watchers[2].data = static_cast<void *>(client_);
                  signal_watchers[3].data = static_cast<void *>(client_);
                  signal_watchers[4].data = static_cast<void *>(client_);
            }

            ~event_loop() { stop(); }

          private:
            static void *pthread_wrapper(void *vdata)
            {
                  auto *loop = static_cast<uv_loop_t *>(vdata);
                  try {
                        uv_run(loop, UV_RUN_DEFAULT);
                  } catch (except::connection_closed &e) {
                        fmt::print(stderr, FMT_COMPILE("Caught exception \"{}\" in \"{}\""), e.what(), FUNCTION_NAME);
                  }
                  pthread_exit(nullptr);
            }

          protected:
            void kill_thread() {
                  if (pid_ > 0) {
#ifdef _WIN32
                        pthread_kill(pid_, SIGHUP);
#else
                        pthread_kill(pid_, SIGUSR1);
#endif
                        pid_ = (-1);
                  }
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
                  client_->connection()->init_uv_poll_handle(&loop_handle, &poll_handle);

#ifndef _WIN32
                  uv_signal_start(&signal_watchers[0], &detail::loop_graceful_signal_cb<ConnectionType, MsgType, UserDataType>, SIGUSR1);
                  uv_signal_start(&signal_watchers[1], &detail::loop_signal_cb, SIGPIPE);
#endif
                  uv_signal_start(&signal_watchers[2], &detail::loop_signal_cb, SIGINT);
                  uv_signal_start(&signal_watchers[3], &detail::loop_graceful_signal_cb<ConnectionType, MsgType, UserDataType>, SIGHUP);
                  uv_signal_start(&signal_watchers[4], &detail::loop_signal_cb, SIGTERM);

                  uv_poll_start(&poll_handle, UV_READABLE, cb);

                  pthread_attr_t pattr;
                  pthread_attr_init(&pattr);
                  //pthread_attr_setdetachstate(&pattr, 1);
                  pthread_create(&pid_, &pattr, &pthread_wrapper, &loop_handle);

                  // uv_run(&loop_handle, UV_RUN_DEFAULT);
                  // thrd_ = std::thread{uv_run, &loop_handle, UV_RUN_DEFAULT};
                  started_ = true;
            }

            void stop()
            {
                  stop_uv_handles(client_);
                  // pthread_kill(thrd_.native_handle(), SIGUSR1);
                  // thrd_.join();
                  if (pid_ != (-1))
                        pthread_join(pid_, nullptr);
            }

            static void stop_uv_handles(client_type *data)
            {
                  if (!data->loop().started_)
                        return;
                  data->loop().kill_thread();
#ifndef _WIN32
                  uv_signal_stop(&data->loop().signal_watchers[0]);
                  uv_signal_stop(&data->loop().signal_watchers[1]);
#endif
                  uv_signal_stop(&data->loop().signal_watchers[2]);
                  uv_signal_stop(&data->loop().signal_watchers[3]);
                  uv_signal_stop(&data->loop().signal_watchers[4]);
                  uv_poll_stop(&data->loop().poll_handle);
                  uv_stop(&data->loop().loop_handle);

                  // uv_close(reinterpret_cast<uv_handle_t *>(&data->loop().poll_handle), [](uv_handle_t *) { throw except::connection_closed(); });
                  uv_loop_close(&data->loop().loop_handle);

                  data->loop().started_ = false;
            }

            DELETE_COPY_CTORS(event_loop);
            DELETE_MOVE_CTORS(event_loop);
      };
#endif
};


/****************************************************************************************/
} // namespace ipc
} // namespace emlsp
#endif // base_client.hh
