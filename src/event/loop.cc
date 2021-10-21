#include "client.hh"
#include "event/loop.hh"
#include "lsp-protocol/lsp-connection.hh"
#include "lsp-protocol/static-data.hh"

#include "rapid.hh"
#include <fmt/printf.h>
#include <nlohmann/json.hpp>

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

#include <uv.h>
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
      char buf[6];
      auto ctxt = asio::io_context{};
      auto desc = asio::posix::stream_descriptor(ctxt);
      desc.assign(STDIN_FILENO);

      asio::read(desc, asio::buf(buf));
      std::cout << "Read:\t\"" << std::string{buf, 6} << "\"" << std::endl;
#endif
}

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


static void
dump_message(void const *msg, size_t const len)
{
      std::cout.flush();
      fflush(stdout);
      std::cerr.flush();
      fflush(stderr);

      fprintf(stdout, "\n\033[1;32m------ MSG LENGTH %zu ------\033[0m\n%.*s\n"
                      "\033[1;32m------ END MSG ------\033[0m\n\n",
              len, static_cast<int>(len), static_cast<char const *>(msg));

      fflush(stdout);
      fflush(stderr);
}

static void dump_message(std::string const &msg) { dump_message(msg.data(), msg.size()); }
static void dump_message(std::vector<char> const &msg) { dump_message(msg.data(), msg.size() - SIZE_C(1)); }

using emlsp::rpc::lsp::data::initialization_message;

#ifdef _WIN32
static constexpr char const test_rooturi[] = R"("file:///D:/ass/VisualStudio/elemwin")";
static constexpr char const test_uri[]     = R"(file:///D:/ass/VisualStudio/elemwin/src/main.cc)";
static constexpr char const test_file[]    = R"(D:\ass\VisualStudio\elemwin\src\main.cc)";
static constexpr char const clangd_exe[]   = R"(D:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Tools\Llvm\x64\bin\clangd.exe)";
#else
static constexpr char const test_rooturi[] = R"("file:///home/bml/files/Projects/elemwin")";
static constexpr char const test_uri[]     = "file:///home/bml/files/Projects/elemwin/src/main.cc";
static constexpr char const test_file[]    = "/home/bml/files/Projects/elemwin/src/main.cc";
static constexpr char const clangd_exe[]   = "clangd";
#endif

using emlsp::rpc::lsp::unix_socket_connection;
using emlsp::rpc::lsp::pipe_connection;

using con_type = emlsp::rpc::lsp::unix_socket_connection;
//using con_type = emlsp::rpc::lsp::pipe_connection;
//using con_type = emlsp::rpc::lsp::named_pipe_connection;


UNUSED NOINLINE static void
test03_1(con_type &con, std::vector<char> const &buf)
{
      using rapidjson::Type;
      using rapidjson::Value;

      rapidjson::Document doc(Type::kObjectType);
      auto &&al = doc.GetAllocator();

      Value obj(Type::kObjectType);
      obj.AddMember("text", Value(buf.data(), buf.size() - 1), al);
      obj.AddMember("version", 1, al);
      obj.AddMember("languageId", "cpp", al);
      obj.AddMember("uri", test_uri, al);

      Value params(Type::kObjectType);
      params.AddMember("textDocument", std::move(obj), al);
      doc.AddMember("params", std::move(params), al);
      doc.AddMember("method", "textDocument/didOpen", al);

      con.write_message(doc);
      auto msg = con.read_message();
      dump_message(msg.data(), msg.size() - SIZE_C(1));

      rapidjson::Document     rddoc(Type::kObjectType);
      rapidjson::StringBuffer ss;
      rapidjson::PrettyWriter writer(ss);
      rddoc.ParseInsitu(msg.data());
      rddoc.Accept(writer);

      dump_message(ss.GetString());
}

UNUSED NOINLINE static void
test03_2(con_type &con, std::vector<char> const &buf)
{
      using rapidjson::Type;
      using rapidjson::Value;

      rpc::json::rapid_doc wrap;
      wrap.add_member ("method", "textDocument/didOpen");
      wrap.push_member("params");
      wrap.push_member("textDocument");
      wrap.add_member ("uri", test_uri);
      wrap.add_member ("languageId", "cpp");
      wrap.add_member ("version", 1);
      wrap.add_member ("text", Value(buf.data(), buf.size() - SIZE_C(1)));

      con.write_message(wrap.doc());
      auto msg = con.read_message();
      dump_message(msg.data(), msg.size() - 1);

      auto rddoc = rapidjson::Document(Type::kObjectType);
      rapidjson::StringBuffer ss;
      rapidjson::PrettyWriter writer(ss);
      rddoc.ParseInsitu(msg.data());
      rddoc.Accept(writer);

      dump_message(ss.GetString());
}

UNUSED NOINLINE static void
test03_3(con_type &con, std::vector<char> &buf)
{
      using nlohmann::json;

      /* If you don't specify json::object explicitly, the library just guesses what
       * kind of json value you might have wanted. It's usually wrong. */
      auto obj = json::object( {
          {"method", "textDocument/didOpen"},
          {"params", json::object( {{"textDocument",
                                     json::object( {{"uri", test_uri},
                                                    {"languageId", "cpp{}"},
                                                    {"version", 1},
                                                    {"text", json(buf.data())}} )
                                    }}
          )}
      } );

      con.write_message(obj);
      auto const msg = con.read_message();
      dump_message(msg.data(), msg.size() - SIZE_C(1));

      auto const rdobj = json::parse(msg.data());
      std::stringstream ss;
      ss << std::setw(4) << rdobj << '\n';

      dump_message(ss.str());
}

namespace {

void
uv_poll_callback(uv_poll_t *hand, UNUSED int status, UNUSED int events)
{
      auto      *con = static_cast<unix_socket_connection *>(hand->data);
      auto const msg  = con->read_message();
      dump_message(msg);
}

void uvw_poll_callback (uvw::PollEvent const &ev, uvw::PollHandle &handle)
{
      if (ev.flags & uvw::details::UVPollEvent::READABLE) {
            auto con = handle.data<unix_socket_connection>();
            auto const msg  = con->read_message();
            dump_message(msg);
      }
}

} // namespace

void test03()
{
      std::vector<char> buf;

      {
            struct stat st; //NOLINT(cppcoreguidelines-pro-type-member-init)
            FILE *fp = ::fopen(test_file, "rb");
            ::fstat(::fileno(fp), &st);
            util::resize_vector_hack(buf, st.st_size + SIZE_C(1));
            (void)::fread(buf.data(), 1, st.st_size, fp);
            ::fclose(fp);
            buf[st.st_size] = '\0';
      }

      auto const init = fmt::sprintf(FMT_STRING(initialization_message), ::getpid(), test_rooturi);
      std::cout << "I HAVE: " << init << '\n';
      std::cout.flush();

#if 0
      fmt::print(FMT_COMPILE("\n\033[1;31mrapidjson\033[0m\n\n"));
      {
            con_type con;
            //con.connection().set_stderr_devnull();
            con.spawn_connection_l(clangd_exe, "--pch-storage=memory", "--log=verbose");
            con.write_message(init);
            con.discard_message();
            test03_1(con, buf);
      }
#endif

#if 0
      fmt::print(FMT_COMPILE("\n\033[1;31mMy wrapper\033[0m\n\n"));
      {
            con_type con;
            //con.connection().set_stderr_devnull();
            con.spawn_connection_l(clangd_exe, "--pch-storage=memory", "--log=verbose");
            con.write_message(init);
            //con.discard_message();
            {
                  //auto foo = con.read_message_string();
                  auto foo = con.read_message();
                  std::cerr.flush();
                  std::cerr << fmt::format(FMT_COMPILE("\nRead {} bytes ->\n((\n{}\n))\n\n"), foo.size(), foo.data());
                  dump_message(foo.data(), foo.size() - 1);
                  std::cerr.flush();
            }
            test03_2(con, buf);
      }
#endif

#if 0
      fmt::print(FMT_COMPILE("\n\033[1;31mnlohmann\033[0m\n\n"));
      {
            con_type con;
            //con.connection().set_stderr_devnull();
            con.spawn_connection_l(clangd_exe, "--pch-storage=memory", "--log=verbose");
            con.write_message(init);
            con.discard_message();
            test03_3(con, buf);
      }
#endif

#if 0
      {
            auto con = std::make_unique<unix_socket_connection>();
            con->spawn_connection_l(clangd_exe, "--pch-storage=memory", "--log=verbose");
            con->write_message(init);

            auto loop = std::make_unique<uv_loop_t>();
            auto hand = std::make_unique<uv_poll_t>();

            uv_loop_init(loop.get());
            uv_poll_init_socket(loop.get(), hand.get(), (*con)()());
            loop->data = hand->data = con.get();

            uv_poll_start(hand.get(), UV_READABLE, uv_poll_callback);
            uv_run(loop.get(), UV_RUN_DEFAULT);
            uv_poll_stop(hand.get());
      }
#endif

#if 1
      {
            auto con = std::make_shared<unix_socket_connection>();
            con->spawn_connection_l(clangd_exe, "--pch-storage=memory", "--log=verbose");
            con->write_message(init);

            auto loop   = uvw::Loop::create();
            auto handle = loop->resource<uvw::PollHandle>(uvw::OSSocketHandle((*con)()()));

            loop->data(con);
            handle->data(con);
            handle->on<uvw::PollEvent>(std::function(uvw_poll_callback));
            handle->start(uvw::details::UVPollEvent::READABLE);

            loop->run();

            handle->stop();
            handle->close();
            loop->close();
            handle.reset();
            loop.reset();
      }
#endif
}

} // namespace emlsp::event


namespace emlsp::rpc::event::test {



} // namespace emlsp::rpc::event::test
