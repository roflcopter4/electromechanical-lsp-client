#include "client.hh"
#include "event/loop.hh"
#include "lsp-protocol/lsp-connection.hh"
#include "lsp-protocol/static-data.hh"

#include "rapid.hh"
//#include <fmt/printf.h>
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

//#include <uvw.hpp>

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
      std::cerr.flush();
      fprintf(stdout, "\n\033[1;32m------ MSG LENGTH %zu ------\033[0m\n%.*s\n"
                      "\033[1;32m------ END MSG ------\033[0m\n\n",
              len, static_cast<int>(len), static_cast<char const *>(msg));
      fflush(stdout);
}

static void dump_message(std::string const &msg) { dump_message(msg.data(), msg.size()); }

using emlsp::rpc::lsp::data::initialization_message;
using emlsp::rpc::lsp::unix_socket_connection;
using emlsp::rpc::lsp::pipe_connection;

static constexpr char const test_rooturi[] = R"("file:///home/bml/files/Projects/elemwin")";
static constexpr char const test_uri[]     = "file:///home/bml/files/Projects/elemwin/src/main.cc";
static constexpr char const test_file[]    = "/home/bml/files/Projects/elemwin/src/main.cc";


NOINLINE static void
test03_1(unix_socket_connection &con, std::vector<char> const &buf)
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

NOINLINE static void
test03_2(unix_socket_connection &con, std::vector<char> const &buf)
{
      using rapidjson::Type;
      using rapidjson::Value;

      emlsp::rpc::json::rapid_doc wrap;
      wrap.add_member ("method", "textDocument/didOpen");
      wrap.push_member("params");
      wrap.push_member("textDocument");
      wrap.add_member ("uri", test_uri);
      wrap.add_member ("languageId", "cpp");
      wrap.add_member ("version", 1);
      wrap.add_member ("text", Value(buf.data(), buf.size() - 1));

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

NOINLINE static void
test03_3(unix_socket_connection &con, std::vector<char> &buf)
{
      using nlohmann::json;

      auto obj = json::object({
          {"method", "textDocument/didOpen"},
          {"params", json::object({{"textDocument",
                                    json::object({{"uri", test_uri},
                                                  {"languageId", "cpp"},
                                                  {"version", 1},
                                                  {"text", json(buf.data())}})}})}
      });

      con.write_message(obj);
      auto msg = con.read_message();
      dump_message(msg.data(), msg.size() - 1);

      auto rdobj = json::parse(msg.data());
      std::stringstream ss;
      ss << std::setw(4) << rdobj << '\n';

      dump_message(ss.str());
}

void test03()
{
      std::vector<char> buf;

      {
            struct stat st; //NOLINT
            FILE *fp = ::fopen(test_file, "rb");
            ::fstat(::fileno(fp), &st);

            emlsp::util::resize_vector_hack(buf, st.st_size + 1);
            (void)::fread(buf.data(), 1, st.st_size, fp);
            buf[st.st_size] = '\0';
            ::fclose(fp);
      }

      char *init;
      int   initlen = ::asprintf(&init, initialization_message, ::getpid(), test_rooturi);
      std::cout << "I HAVE: " << init << '\n';
      std::cout.flush();

#if 1
      fmt::print(FMT_COMPILE("\n\033[1;31mrapidjson\033[0m\n\n"));
      {
            unix_socket_connection con;
            con.connection().set_stderr_devnull();
            con.spawn_connection_l("clangd", "--pch-storage=memory");
            con.write_message(init, initlen);
            con.discard_message();
            test03_1(con, buf);
      }
#endif

#if 0
      fmt::print(FMT_COMPILE("\n\033[1;31mMy wrapper\033[0m\n\n"));
      {
            unix_socket_connection con;
            con.connection().set_stderr_devnull();
            con.spawn_connection_l("clangd", "--pch-storage=memory");
            con.write_message(init, initlen);
            con.discard_message();
            test03_2(con, buf);
      }
#endif

#if 0
      fmt::print(FMT_COMPILE("\n\033[1;31mnlohmann\033[0m\n\n"));
      {
            pipe_connection con;
            con.spawn_connection_l("clangd", "--pch-storage=memory");
            con.write_message(init, initlen);
            con.discard_message();
            test03_3(con, buf);
      }
#endif

      free(init); //NOLINT
}

} // namespace emlsp::event


namespace emlsp::rpc::event::test {



} // namespace emlsp::rpc::event::test 
