#include "Common.hh"
#include "ipc/lsp/lsp-client.hh"
#include "ipc/lsp/lsp-connection.hh"
#include "ipc/lsp/static-data.hh"
#include "util/exceptions.hh"

#include <msgpack.hpp>
#include <type_traits>
#include <uv.h>

// #include <uvw.hpp>

inline namespace emlsp {
namespace event {

struct userdata;

UNUSED static void do_event_loop(uv_loop_t *loop, uv_poll_t *upoll, uv_signal_t signal_watcher[5]);

UNUSED NORETURN static void *pthread_shim(void *vdata);

UNUSED static void event_loop_init_watchers(uv_loop_t *loop, uv_signal_t signal_watcher[5]);
UNUSED static void event_loop_start_watchers(uv_signal_t signal_watcher[5]);
UNUSED static void event_loop_io_cb(uv_poll_t* handle, int status, int events);
UNUSED static void event_loop_graceful_signal_cb(uv_signal_t *handle, int signum);
UNUSED static void event_loop_signal_cb(uv_signal_t *handle, int signum);
UNUSED static void event_loop_stop_uv_handles(userdata *data);

struct callback_data
{

};

// using client_t = ipc::lsp::unix_socket_client<userdata *>;
using client_t = ipc::lsp::pipe_client<ipc::loops::libevent, userdata *>;
// using client_t = ipc::base_client<ipc::lsp::unix_socket_connection, rapidjson::Document, userdata *>;

struct userdata {
      client_t    *client;
      uv_loop_t   *loop_handle;
      uv_poll_t   *poll_handle;
      uv_signal_t *signal_watchers;
      bool         grace;
};

struct message_data {
      rapidjson::Document doc{};
      // char *msg = nullptr;

      message_data() = default;
};


#if 0
void uvw_poll_callback (uvw::PollEvent const &ev, uvw::PollHandle &handle)
{
      if (ev.flags & uvw::details::UVPollEvent::READABLE) {
            auto const data   = std::make_unique<message_data>();
            auto const client = handle.data<client_t>();
            // client->read_message(&data->msg);
            auto msg = client->read_message();

            data->doc.ParseInsitu<rapidjson::kParseInsituFlag>(msg.data());

            rapidjson::StringBuffer ss;
            rapidjson::PrettyWriter writer(ss);
            data->doc.Accept(writer);
            fmt::print(FMT_COMPILE("Got message of length {}:\n{}\n"), ss.GetLength(), ss.GetString());

            client->condition().notify_one();
      }
}
#endif

static void
event_loop_io_cb(uv_poll_t *handle, UNUSED int const status, int const events)
{
      if (status) {
            fmt::print(stderr, FMT_COMPILE("\nUh oh -> {}\n\n"), status);
      }
      if (events & UV_READABLE) {
            auto *client = static_cast<client_t *>(handle->data);


            auto  data   = std::make_unique<message_data>();

            // auto *custom = static_cast<userdata *>(handle->data);
            // auto *client = custom->client;
            // if (uv_fileno(reinterpret_cast<uv_handle_t const *>(handle), &data.fd))
            //       err(1, "uv_fileno()");

            // data.obj = mpack_decode_stream(data.fd);
            // handle_nvim_message(&data);

            std::vector<char> msg;

            try {
                  msg = client->read_message();
            } catch (ipc::except::connection_closed &e) {
                  client->cond_notify_one();
                  return;
            }
#if 0
            data->doc.ParseInsitu<rapidjson::kParseInsituFlag>(msg.data());

            rapidjson::StringBuffer ss;
            rapidjson::PrettyWriter writer(ss);
            data->doc.Accept(writer);
            fmt::print(FMT_COMPILE("Got message of length {}:\n{}\n"), ss.GetLength(), ss.GetString());
#endif

            fmt::print(FMT_COMPILE("Got message of length {}:\n{}\n"), msg.size(), std::string{msg.data(), msg.size()});

            client->cond_notify_one();
      }
}

#if 0
static void *
pthread_shim(void *vdata)
{
      uv_run(static_cast<uv_loop_t *>(vdata), UV_RUN_DEFAULT);
      pthread_exit(nullptr);
}
#endif

NOINLINE void lets_do_this(std::filesystem::path const &fname)
{
      fmt::print(stderr, "\nWorking with {}\n\n", fname.string());
      std::vector<char> contents  = util::slurp_file(fname);
      std::string       fname_uri = "file://" + fname.string();

      auto client = std::make_unique<client_t>();
      auto init   = ipc::lsp::data::init_msg("file://" + fname.parent_path().string());
      client->set_stderr_devnull();
      client->spawn_connection_l("clangd", "--pch-storage=memory", "--log=verbose", "--clang-tidy", "--background-index");
      client->write_message(init);

#if 0
      auto init = ipc::lsp::data::init_msg("file:///home/bml/.vim/dein/repos/github.com/autozimu/LanguageClient-neovim");
      client.con().spawn_connection_l("pylsp", "-v");
      auto loop   = uvw::Loop::getDefault();
      auto handle = loop->resource<uvw::PipeHandle>();
      handle->open(con()());
      auto client = std::make_shared<client_t>(std::move(con));

      loop->data(client);
      handle->data(client);
      handle->on<uvw::DataEvent>()
      loop->run();
#endif

#if 0
      auto loop   = uvw::Loop::create();
      auto handle = loop->resource<uvw::PollHandle>(uvw::OSSocketHandle(con().accepted()));
      auto client = std::make_shared<client_t>(std::move(con));

      loop->data(client);
      handle->data(client);
      handle->on<uvw::PollEvent>(uvw_poll_callback);
      handle->start(uvw::details::UVPollEvent::READABLE);
      loop->run();
#endif


#if 0
      // uv_loop_t *loop = uv_default_loop();
      uv_loop_t loop[1];
      uv_loop_init(loop);
      // uv_loop_t *loop = uv_loop_new();

      uv_signal_t signal_watchers[5];
      uv_poll_t   phand{};
      userdata data = {&client, loop, &phand, signal_watchers, false};
      client.data() = &data;

      uv_poll_init_socket(loop, &phand, client.con()().accepted());
      // uv_poll_init(loop, &phand, client.con()().read_fd());
      loop->data = phand.data = signal_watchers[0].data = signal_watchers[1].data =
                                 signal_watchers[2].data = signal_watchers[3].data =
                                 signal_watchers[4].data =
                   static_cast<void *>(&data);

      event_loop_init_watchers(loop, signal_watchers);
      event_loop_start_watchers(signal_watchers);

      uv_poll_start(&phand, UV_READABLE, event_loop_io_cb);
      //auto thrd = std::thread{ uv_run, loop, UV_RUN_DEFAULT };
      pthread_t pid;
      pthread_create(&pid, nullptr, pthread_shim, loop);
#endif

      //client->loop().start(event_loop_io_cb);
      client->cond_wait();
      using rapidjson::Document, rapidjson::Value, rapidjson::Type;

      {
#if 0
            Document doc(Type::kObjectType);
            auto &&al = doc.GetAllocator();

            Value obj(Type::kObjectType);
            obj.AddMember("text", Value(contents.data(), contents.size() - 1), al);
            obj.AddMember("version", 1, al);
            obj.AddMember("languageId", "cpp", al);
            obj.AddMember("uri", fname_uri, al);

            Value params(Type::kObjectType);
            params.AddMember("textDocument", std::move(obj), al);
            doc.AddMember("params", std::move(params), al);
            doc.AddMember("method", "textDocument/didOpen", al);
            // doc.AddMember("id", 2, al);
#endif

            ipc::json::rapid_doc wrap;
            // wrap.add_member("id", 2);
            wrap.add_member("jsonrpc", "2.0");
            wrap.add_member("method", "textDocument/didOpen");
            wrap.set_member("params");
            wrap.set_member("textDocument");
            wrap.add_member("uri", Value(fname_uri.data(), fname_uri.size()));
            wrap.add_member("text", Value(contents.data(), contents.size() - 1)); // Vector is oversized by 1.
            wrap.add_member("version", 1);
            wrap.add_member("languageId", "cpp");

            client->write_message(wrap.doc());
            client->cond_wait();
      }

      {
            ipc::json::rapid_doc wrap;
            wrap.add_member("jsonrpc", "2.0");
            wrap.add_member("id", 2);
            wrap.add_member("method", "textDocument/semanticTokens/full");
            wrap.set_member("params");
            wrap.set_member("textDocument");
            wrap.add_member("uri", Value(fname_uri.data(), fname_uri.size()));

            client->write_message(wrap.doc());
            client->cond_wait();
      }
      {
            ipc::json::rapid_doc wrap;
            wrap.add_member("jsonrpc", "2.0");
            wrap.add_member("id", 3);
            wrap.add_member("method", "textDocument/semanticTokens/full/delta");
            wrap.set_member("params");
            wrap.add_member("previousResultId", "1");
            wrap.set_member("textDocument");
            wrap.add_member("uri", Value(fname_uri.data(), fname_uri.size()));

            client->write_message(wrap.doc());
            client->cond_wait();
      }

#ifdef _WIN32
      UNUSED constexpr int const kill_sig = SIGHUP;
#else
      UNUSED constexpr int const kill_sig = SIGUSR1;
#endif

#if 0
      client.loop().stop();
      client.kill();
      pthread_kill(pid, kill_sig);
      client.kill();
      pthread_join(pid, nullptr);
#endif

      //pthread_cancel(pid);
      //uv_poll_stop(&phand);
      //uv_stop(loop);
      //pthread_cancel(thrd.native_handle());
      //thrd.join();
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
      uv_signal_start_oneshot(&signal_watchers[3], &event_loop_graceful_signal_cb, SIGHUP);
      uv_signal_start_oneshot(&signal_watchers[4], &event_loop_signal_cb, SIGTERM);
}

#if 0
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
#endif

static void
event_loop_stop_uv_handles(userdata *data)
{
#ifndef _WIN32
      uv_signal_stop(&data->signal_watchers[0]);
      uv_signal_stop(&data->signal_watchers[1]);
#endif
      uv_signal_stop(&data->signal_watchers[2]);
      uv_signal_stop(&data->signal_watchers[3]);
      uv_signal_stop(&data->signal_watchers[4]);
      uv_poll_stop(data->poll_handle);
      uv_stop(data->loop_handle);
      uv_loop_close(data->loop_handle);
}

static void
event_loop_graceful_signal_cb(uv_signal_t *handle, UNUSED int const signum)
{
      auto *data  = static_cast<userdata *>(handle->data);
      data->grace = true;
      event_loop_stop_uv_handles(data);
}

static void
event_loop_signal_cb(uv_signal_t *handle, int const signum)
{
      auto *data  = static_cast<userdata *>(handle->data);
      data->grace = false;

      switch (signum) {
      case SIGTERM:
            fflush(stderr);
            quick_exit(0);
      case SIGHUP:
      case SIGINT:
      default:
            exit(0);
      }
}

} // namespace event
} // namespace emlsp
