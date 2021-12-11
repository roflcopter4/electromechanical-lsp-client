#include "Common.hh"
#include "libevent.hh"
#include "ipc/lsp/lsp-client.hh"
#include "ipc/lsp/lsp-connection.hh"
#include "ipc/lsp/static-data.hh"
#include "util/exceptions.hh"

#define AUTOC auto const

inline namespace emlsp {
namespace libevent {
/****************************************************************************************/

using ConnectionType = ipc::lsp::base_connection<ipc::detail::unix_socket_connection_impl>;

static void
read_cb(UNUSED evutil_socket_t sock, short const flags, void *userdata) // NOLINT(google-runtime-int)
{
      static std::atomic_uint call_no = 0;
      auto *data = static_cast<ConnectionType *>(userdata);
      fmt::print(FMT_COMPILE("\033[1;31mHERE IN {}, call number {}\033[0m\n"), __func__, call_no.fetch_add(1));
      if (flags & EV_READ) {
            auto msg = data->read_message();
            fmt::print(FMT_COMPILE("\033[1;32mGot\033[0m \"{}\"\n"), msg.data());
      }
}

static void
loop_thrd_wrapper(ConnectionType *connection, event_base *loop)
{
      static std::atomic_uint iter = 0;

      timeval tv = {.tv_sec = 0, .tv_usec = 100'000};

      event *rd_handle = event_new(loop, connection->raw_descriptor(), EV_READ|EV_PERSIST, &read_cb, connection);
      event_add(rd_handle, &tv);

      // event_base_loop(loop, EVLOOP_NO_EXIT_ON_EMPTY);

      for (;;) {
            fmt::print(FMT_COMPILE("\033[1;35mHERE IN {}, iter number {}\033[0m\n"), __func__, iter.fetch_add(1));
            event_base_loop(loop, 0);
            if (event_base_got_break(loop))
                  break;
            // event_add(rd_handle, nullptr);
            // event_base_dispatch(loop);
      }

      fmt::print(FMT_COMPILE("\033[1;34mBreaking from event loop!\033[0m\n"));
      event_free(rd_handle);
}


NOINLINE void
testing01(std::filesystem::path const &fname)
{
      event_base *loop;
      {
            event_config *cfg = event_config_new();
            event_config_require_features(cfg, EV_FEATURE_EARLY_CLOSE);
#ifdef _WIN32
            event_config_set_flag(cfg, EVENT_BASE_FLAG_STARTUP_IOCP);
#endif
            loop = event_base_new_with_config(cfg);
            event_config_free(cfg);
      }

      std::vector<char> contents  = util::slurp_file(fname);
      std::string       fname_uri = "file://" + fname.string();
      auto  init       = ipc::lsp::data::init_msg("file://" + fname.parent_path().string());
      auto *connection = new ConnectionType();

      // connection->set_stderr_devnull();
      connection->spawn_connection_l("clangd", "--pch-storage=memory", "--log=verbose", "--clang-tidy", "--background-index");
      connection->write_message(init);

      auto thrd = std::thread{loop_thrd_wrapper, connection, loop};

      //connection->write_message_l(message_1);
      usleep(1'100'000);
      fmt::print(FMT_COMPILE("Sending break sig.\n"));
      event_base_loopbreak(loop);
      thrd.join();
      fmt::print(FMT_COMPILE("Left event loop!\n"));

      event_base_free(loop);
      delete connection;
}

/*--------------------------------------------------------------------------------------*/

static void
read_cb_2(UNUSED evutil_socket_t sock, short const flags, void *userdata) // NOLINT(google-runtime-int)
{
      static std::atomic_uint call_no = 1;
      auto *data = static_cast<ipc::lsp::unix_socket_client<> *>(userdata);
      if (flags == EV_TIMEOUT) {
            call_no.fetch_add(1, std::memory_order::relaxed);
            return;
      }

      std::string foo;
      switch (flags) {
            case 0x01: foo = "timeout"; break;
            case 0x02: foo = "read"; break;
            case 0x04: foo = "write"; break;
            case 0x08: foo = "signal"; break;
            case 0x40: foo = "finalize"; break;
            case 0x80: foo = "close"; break;
            default:   foo = "unknown"; break;
      }

      fmt::print(FMT_COMPILE("\033[1;31mHERE IN {}, call number {} -> {} ({})\033[0m\n"), __func__, call_no.fetch_add(1, std::memory_order::relaxed), foo, flags);

      if (flags & EV_CLOSED)
            return;
      if (flags & EV_READ) {
            std::vector<char> msg;
            try {
                  msg = data->read_message();
            } catch (ipc::except::connection_closed const &e) {
                  data->cond_notify_one();
                  return;
            }
            fmt::print(FMT_COMPILE("\033[1;32mGot\033[0m \"{}\"\n"), msg.data());
            data->cond_notify_one();
      }
}


NOINLINE void
testing02(std::filesystem::path const &fname)
{
      std::string const fname_uri = "file://" + fname.string();
      std::string const init      = ipc::lsp::data::init_msg("file://" + fname.parent_path().string());
      auto        const contents  = util::slurp_file(fname);
      auto              client    = std::make_unique<ipc::lsp::unix_socket_client<>>();

      // client->set_stderr_devnull();
      client->spawn_connection_l("clangd", "--pch-storage=memory", "--log=verbose", "--clang-tidy", "--background-index");
      client->write_message(init);
      client->loop().start(&read_cb_2);
      client->cond_wait();

      {
            ipc::json::rapid_doc wrap;
            wrap.add_member("jsonrpc", "2.0");
            wrap.add_member("method", "textDocument/didOpen");
            wrap.set_member("params");
            wrap.set_member("textDocument");
            wrap.add_member("uri", rapidjson::Value(fname_uri.data(), fname_uri.size()));
            wrap.add_member("text", rapidjson::Value(contents.data(), contents.size() - 1)); // Vector is oversized by 1.
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
            wrap.add_member("uri", rapidjson::Value(fname_uri.data(), fname_uri.size()));

            client->write_message(wrap.doc());
            client->cond_wait();
      }
      {
            ipc::json::rapid_doc wrap;
            wrap.add_member ("jsonrpc", "2.0");
            wrap.add_member ("id", 3);
            wrap.add_member ("method", "textDocument/semanticTokens/full/delta");
            wrap.set_member ("params");
            wrap.add_member ("previousResultId", "1");
            wrap.set_member ("textDocument");
            wrap.add_member ("uri", rapidjson::Value(fname_uri.data(), fname_uri.size()));

            client->write_message(wrap.doc());
            client->cond_wait();
      }

      // client->kill();
      // client->loop().loopbreak();
}

/****************************************************************************************/
} // namespace libevent
} // namespace emlsp
