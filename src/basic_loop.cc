#include "Common.hh"

#include <msgpack.hpp>
#include <uv.h>

#include <uvw.hpp>

inline namespace emlsp {
namespace ipc {

static thrd_t event_loop_thread;

static void do_event_loop(uv_loop_t *loop, uv_poll_t *upoll, uv_signal_t signal_watcher[5]);
static void event_loop_init_watchers(uv_loop_t *loop, uv_signal_t signal_watcher[5]);
static void event_loop_start_watchers(uv_signal_t signal_watcher[5]);
static void event_loop_io_cb(uv_poll_t* handle, int status, int events);
static void event_loop_graceful_signal_cb(uv_signal_t *handle, int signum);
static void event_loop_signal_cb(uv_signal_t *handle, int signum);

struct event_data {
        msgpack::object *obj;
        uv_os_fd_t fd;
};

struct userdata {
      uv_loop_t   *loop_handle;
      uv_poll_t   *poll_handle;
      uv_signal_t *signal_watchers;
      bool         grace;
};

void wtf_am_i_doing(uvw::DataEvent const &event, UNUSED uvw::PipeHandle &hand)
{
      fmt::print(FMT_COMPILE("I have read \"{}\"\n"), std::string{event.data.get(), event.length});
}

void wtf_am_i_doing2(uvw::DataEvent const &event, UNUSED uvw::ProcessHandle &hand)
{
      fmt::print(FMT_COMPILE("I have read \"{}\"\n"), std::string{event.data.get(), event.length});
}

void run_event_loop_cxx([[maybe_unused]] int const fd)
{
      static std::atomic_flag event_loop_called{};
      if (event_loop_called.test_and_set())
            return;

      auto loop = uvw::Loop::create();

      // std::function<void (uvw::DataEvent const &, uvw::PipeHandle &)> foo = wtf_am_i_doing;

      constexpr char const *const fuckme[] = {"/bin/cat", "-", nullptr};
      auto hand = loop->resource<uvw::ProcessHandle>();
      hand->spawn("/bin/cat", const_cast<char **>(fuckme));
      hand->on<uvw::DataEvent>(&wtf_am_i_doing2);
      // auto thrd = std::thread(killme, std::move(loop));

      loop->run();

#ifdef _WIN32
      Sleep(100);
      if (::WriteFile(hand->fd(), "FUCKING RETARD\n", 15, nullptr, nullptr))
            err(1, "write()");
      if (::WriteFile(hand->fd(), "FUCKING RETARD\n", 15, nullptr, nullptr))
            err(1, "write()");
#else
      usleep(100);
      if (::write(hand->fd(), SLS("FUCKING RETARD\n") - 1) == (-1))
            err(1, "write()");
      if (::write(hand->fd(), SLS("FUCKING RETARD\n") - 1) == (-1))
            err(1, "write()");
#endif

      hand->kill(SIGTERM);
      loop->stop();

#if 0
      // thrd.join();

      // hand->kill(SIGTERM);
      // hand->loop().stop();

      auto hand = loop->resource<uvw::PipeHandle>(false);
      hand->open(fd);
      hand->read();
      hand->on<uvw::DataEvent>(&wtf_am_i_doing);

      loop->run();
      hand->stop();
      loop->stop();
      hand->close();
      loop->close();
      fmt::print("Fin.\n");
#endif

      // delete hand.get();
      // delete loop.get();
}

void
run_event_loop(int const fd)
{
      static std::atomic_flag event_loop_called{};
      if (event_loop_called.test_and_set())
            return;

      uv_loop_t  *loop = uv_default_loop();
      uv_poll_t   upoll;
      uv_signal_t signal_watchers[5];
      memset(&upoll, 0, sizeof upoll);
      memset(signal_watchers, 0, sizeof signal_watchers);

      uv_loop_init(loop);
      event_loop_init_watchers(loop, signal_watchers);

      uv_pipe_t s;
      memset(&s, 0, sizeof s);
      uv_pipe_init(loop, &s, false);
      uv_pipe_open(&s, fd);

#ifdef _WIN32
      uv_poll_init(loop, &upoll, fd);
#else
      uv_poll_init_socket(loop, &upoll, fd);
#endif

      event_loop_thread = thrd_current();

      do_event_loop(loop, &upoll, signal_watchers);
      uv_loop_close(loop);
      uv_library_shutdown();
}


static void
do_event_loop(uv_loop_t *loop, uv_poll_t *upoll, uv_signal_t signal_watchers[5])
{
      userdata data = {loop, upoll, signal_watchers, false};
      loop->data = upoll->data = signal_watchers[0].data = signal_watchers[1].data =
                                 signal_watchers[2].data = signal_watchers[3].data =
                                 signal_watchers[4].data =
                   &data;

      event_loop_start_watchers(signal_watchers);
      uv_poll_start(upoll, UV_READABLE, &event_loop_io_cb);
      uv_run(loop, UV_RUN_DEFAULT);

      if (!data.grace)
            errx(1, "This shouldn't be reachable?...");
}

static void
event_loop_init_watchers(uv_loop_t *loop, uv_signal_t signal_watchers[5])
{
#ifndef _WIN32
      uv_signal_init(loop, &signal_watchers[0]);
      uv_signal_init(loop, &signal_watchers[1]);
#endif
      uv_signal_init(loop, &signal_watchers[2]);
      uv_signal_init(loop, &signal_watchers[3]);
      uv_signal_init(loop, &signal_watchers[4]);
}

static void
event_loop_start_watchers(uv_signal_t signal_watchers[5])
{
#ifndef _WIN32
      uv_signal_start_oneshot(&signal_watchers[0], &event_loop_graceful_signal_cb, SIGUSR1);
      uv_signal_start_oneshot(&signal_watchers[1], &event_loop_signal_cb, SIGPIPE);
#endif
      uv_signal_start_oneshot(&signal_watchers[2], &event_loop_signal_cb, SIGINT);
      uv_signal_start_oneshot(&signal_watchers[3], &event_loop_signal_cb, SIGHUP);
      uv_signal_start_oneshot(&signal_watchers[4], &event_loop_signal_cb, SIGTERM);
}

static void
event_loop_io_cb(uv_poll_t *handle, UNUSED int const status, int const events)
{
      if (events & UV_READABLE) {
            struct event_data data{};
            if (uv_fileno(reinterpret_cast<uv_handle_t const *>(handle), &data.fd))
                  err(1, "uv_fileno()");

            // data.obj = mpack_decode_stream(data.fd);
            // handle_nvim_message(&data);
      }
}

static void
event_loop_graceful_signal_cb(uv_signal_t *handle, UNUSED int const signum)
{
      auto *data  = static_cast<userdata *>(handle->data);
      data->grace = true;

#ifndef _WIN32
      uv_signal_stop(&data->signal_watchers[0]);
      uv_signal_stop(&data->signal_watchers[1]);
#endif
      uv_signal_stop(&data->signal_watchers[2]);
      uv_signal_stop(&data->signal_watchers[3]);
      uv_signal_stop(&data->signal_watchers[4]);
      uv_poll_stop(data->poll_handle);
      uv_stop(data->loop_handle);
}

static void
event_loop_signal_cb(uv_signal_t *handle, int const signum)
{
      auto *data  = static_cast<userdata *>(handle->data);
      data->grace = false;

      switch (signum) {
      case SIGTERM:
      case SIGHUP:
      case SIGINT:
#ifndef _WIN32
      case SIGPIPE:
#endif
            quick_exit(0);
      default:
            exit(0);
      }
}

} // namespace ipc
} // namespace emlsp
