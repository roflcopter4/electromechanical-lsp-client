#include "client.hh"
#include "event/loop.hh"
#include "lsp-protocol/lsp-connection.hh"

#if 0
#include <asio.hpp>
#include <asio/basic_socket.hpp>
#include <asio/basic_socket_iostream.hpp>
#include <asio/connect.hpp>
#include <asio/io_context.hpp>
#include <asio/io_service.hpp>
#include <asio/socket_base.hpp>
#include <asio/streambuf.hpp>
#endif

#include <uvw.hpp>

namespace emlsp::event {

void test01()
{
      // asio::posix::stream_descriptor desc((asio::any_io_executor()));

#if 0
      auto sock = asio::basic_stream_socket<asio::generic::stream_protocol>(asio::any_io_executor());
      auto proto = asio::generic::stream_protocol(AF_UNIX, AF_UNIX);
      sock.open(proto);
#endif

#if 0
      auto buf = asio::streambuf();
      std::ostream os(&buf);
#endif

#if 0
      str += "HELLO, IDIOT!\n";
      str += "HELLO, IDIOT!\n";
      str += "HELLO, IDIOT!\n";
      str += "HELLO, IDIOT!\n";
#endif

#if 0
      char buf[6];
      auto ctxt = asio::io_context{};
      auto desc = asio::posix::stream_descriptor(ctxt);
      desc.assign(STDIN_FILENO);

      asio::read(desc, asio::buffer(buf));
      std::cout << "Read:\t\"" << std::string{buf, 6} << "\"" << std::endl;
#endif
}

static constexpr
char const initial_msg[] =
R"({
      "jsonrpc": "2.0",
      "id": 1,
      "method": "initialize",
      "params": {
            "trace":     "on",
            "locale":    "en_US.UTF8",
            "clientInfo": {
                  "name": ")" MAIN_PROJECT_NAME R"(",
                  "version": ")" MAIN_PROJECT_VERSION_STRING R"("
            },
            "capabilities": {
            }
      }
})";

void test02()
{
#if 0
      emlsp::rpc::lsp::unix_socket_connection client;
      client.spawn_connection_l("clangd", "--pretty", "--log=verbose");
      std::string const msg1 = fmt::format(FMT_COMPILE("Content-Length: {}\r\n\r\n{}"), sizeof(initial_msg) - SIZE_C(1), initial_msg);

      auto loop = uvw::Loop::create();
      auto foo  = loop->resource<uvw::PipeHandle>();
      foo->open(client.sock().sock.accepted);
      foo->bind(tmppath.string());
      foo->listen();
      struct sigaction act{};
      act.sa_handler = [](UNUSED int x){};
      sigaction(SIGINT, &act, nullptr);

      loop->run();
      foo->write(const_cast<char *>(msg1.data()), msg1.size());
      //pause();
      loop->stop();

      //foo->shutdown();
      foo->close();
      loop->close();
      foo->clear();
      loop->clear();
      // delete foo.get();
      // delete (uvw::PipeHandle *)(((char *)foo.get()) - 24);
      // delete loop.get();
      //delete (uvw::Loop *)(((char *)loop.get()) - 24);
#endif
}

} // namespace emlsp::event
