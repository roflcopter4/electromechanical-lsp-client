#include "client.hh"
#include "event/loop.hh"
#include "lsp-protocol/lsp-connection.hh"
#include "lsp-protocol/static-data.hh"

//#include <nlohmann/json.hpp>

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

//#include <event2/event.h>
//#include <anyrpc/anyrpc.h>

#ifndef __MINGW32__
#  ifdef snprintf
#    undef snprintf
#  endif
#  ifdef vsnprintf
#    undef vsnprintf
#  endif
#endif


namespace emlsp::event {

#if 0
void test01()
{
      // asio::posix::stream_descriptor desc((asio::any_io_executor()));

      auto sock = asio::basic_stream_socket<asio::generic::stream_protocol>(asio::any_io_executor());
      auto proto = asio::generic::stream_protocol(AF_UNIX, AF_UNIX);
      sock.open(proto);

      auto buf = asio::streambuf();
      std::ostream os(&buf);

      char buf[6];
      auto ctxt = asio::io_context{};
      auto desc = asio::posix::stream_descriptor(ctxt);
      desc.assign(STDIN_FILENO);

      asio::read(desc, asio::buf(buf));
      std::cout << "Read:\t\"" << std::string{buf, 6} << "\"" << std::endl;
}

void test02()
{
      emlsp::ipc::lsp::unix_socket_connection client;
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
}
#endif

#ifdef _WIN32
static constexpr char const test_rooturi[] = R"("file:///D:/ass/VisualStudio/elemwin")";
static constexpr char const test_uri[]     = R"(file:///D:/ass/VisualStudio/elemwin/src/main.cc)";
static constexpr char const test_file[]    = R"(D:\ass\VisualStudio\elemwin\src\main.cc)";
//static constexpr char const clangd_exe[]   = R"(D:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Tools\Llvm\x64\bin\clangd.exe)";
static constexpr char const clangd_exe[]   = R"(D:\new_msys64\ucrt64\bin\clangd.exe)";
#else
#define ACTUAL_FILE "client.hh"
static constexpr char const test_rooturi[] = "file:///home/bml/files/Projects/Again";
static constexpr char const test_uri[]     = "file:///home/bml/files/Projects/Again/src/" ACTUAL_FILE;
static constexpr char const test_file[]    = "/home/bml/files/Projects/Again/src/" ACTUAL_FILE;
static constexpr char const clangd_exe[]   = "clangd";
#endif

using emlsp::ipc::lsp::unix_socket_connection;
using emlsp::ipc::lsp::pipe_connection;

using con_type = emlsp::ipc::lsp::unix_socket_connection;
//using con_type = emlsp::ipc::lsp::pipe_connection;
//using con_type = emlsp::ipc::lsp::named_pipe_connection;


namespace {

template <bool Writing = false> void dump_message(void const *, size_t);
template <bool Writing = false> void dump_message(std::string const &);
template <bool Writing = false> void dump_message(std::vector<char> const &);

template <bool Writing> void
dump_message(void const *msg, size_t const len)
{
      std::cout.flush();
      fflush(stdout);
      std::cerr.flush();
      fflush(stderr);

      if (Writing)
            fprintf(stdout, "\n\033[1;34m------ WRITING MSG OF LENGTH %zu ------\033[0m\n%.*s\n"
                            "\033[1;34m------ END MSG ------\033[0m\n\n",
                    len, static_cast<int>(len), static_cast<char const *>(msg));
      else
            fprintf(stdout, "\n\033[1;32m------ MSG LENGTH %zu ------\033[0m\n%.*s\n"
                            "\033[1;32m------ END MSG ------\033[0m\n\n",
                    len, static_cast<int>(len), static_cast<char const *>(msg));

      fflush(stdout);
      fflush(stderr);
}

template <bool Writing> void
dump_message(std::string const &msg)
{
      dump_message<Writing>(msg.data(), msg.size());
}

template <bool Writing> void
dump_message(std::vector<char> const &msg)
{
      dump_message<Writing>(msg.data(), msg.size() - SIZE_C(1));
}

__inline void
write_and_dump(con_type &con, rapidjson::Document const &doc)
{
      rapidjson::MemoryBuffer ss;
      rapidjson::Writer writer(ss);
      doc.Accept(writer);
      dump_message<true>(ss.GetBuffer(), ss.GetSize());
      con.write_message(ss.GetBuffer(), ss.GetSize());
}

using rapidjson::Type;
using rapidjson::Value;

UNUSED NOINLINE void
test03_2_1(con_type &con, std::vector<char> const &buf)
{
      ipc::json::rapid_doc wrap;
      wrap.add_member ("jsonrpc", "2.0");
      // wrap.add_member ("id", 1);
      wrap.add_member ("method", "textDocument/didOpen");
      wrap.push_member("params");
      wrap.push_member("textDocument");
      wrap.add_member ("uri", test_uri);
      wrap.add_member ("languageId", "cpp");
      wrap.add_member ("version", 1);
      wrap.add_member ("text", Value(buf.data(), buf.size() - SIZE_C(1)));

      write_and_dump(con, wrap.doc());
}

UNUSED NOINLINE void
test03_3_1(con_type &con)
{
      {
            ipc::json::rapid_doc wrap;
            wrap.add_member ("jsonrpc", "2.0");
            wrap.add_member ("id", 2);
            wrap.add_member ("method", "client/registerCapability");
            wrap.push_member("params");
            wrap.push_member("registrations", Type::kArrayType);
            wrap.push_value (Type::kObjectType);
            wrap.add_member ("id", "1");
            wrap.add_member ("method", "textDocument/semanticTokens/full/delta");
            wrap.add_member ("registerOptions", Value{Type::kArrayType});

            write_and_dump(con, wrap.doc());
      }
      {
            ipc::json::rapid_doc wrap;
            wrap.add_member ("jsonrpc", "2.0");
            wrap.add_member ("id", 3);
            wrap.add_member ("method", "textDocument/semanticTokens/full");
            wrap.push_member("params");
            wrap.push_member("textDocument");
            wrap.add_member ("uri", test_uri);

            write_and_dump(con, wrap.doc());
      }
}

UNUSED NOINLINE void
test03_2_2(con_type &con)
{
      ipc::json::rapid_doc wrap;
      wrap.add_member ("jsonrpc", "2.0");
      // wrap.add_member ("id", 4);
      wrap.add_member ("method", "textDocument/didClose");
      wrap.push_member("params");
      wrap.push_member("textDocument");
      wrap.add_member ("uri", test_uri);

      write_and_dump(con, wrap.doc());
}

UNUSED NOINLINE void
test03_2_3(con_type &con)
{
      ipc::json::rapid_doc wrap;
      wrap.add_member ("jsonrpc", "2.0");
      // wrap.add_member ("id", 5);
      wrap.add_member ("method", "shutdown");
      wrap.push_member("params");

      write_and_dump(con, wrap.doc());
}

UNUSED NOINLINE void
test03_2_4(con_type &con)
{
      ipc::json::rapid_doc wrap;
      wrap.add_member ("jsonrpc", "2.0");
      wrap.add_member ("method", "exit");
      wrap.push_member("params");

      rapidjson::MemoryBuffer ss;
      rapidjson::Writer writer(ss);
      wrap.doc().Accept(writer);
      dump_message<true>(ss.GetBuffer(), ss.GetSize());

      con.write_message(ss.GetBuffer(), ss.GetSize());
}


} // namespace

#if 0
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
test03_3(con_type &con, std::vector<char> &buf)
{
      //using nlohmann::json;

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
#endif

namespace {


struct callback_data
{
    private:
      // using con_type_ptr  = std::shared_ptr<con_type>;
      // using condition_ptr = std::shared_ptr<std::condition_variable>;
      using con_type_ptr  = con_type *;
      using condition_ptr = std::condition_variable *;

    public:
      con_type_ptr  con       = {};
      condition_ptr condition = {};

      ~callback_data()
      {
            delete con;
            delete condition;
      }

      callback_data() = default;
      callback_data(con_type_ptr const &cl, condition_ptr const &cnd)
          : con(cl), condition(cnd)
      {}
      callback_data(con_type_ptr &&cl, condition_ptr const &cnd)
          : con(cl), condition(cnd)
      {}

      DELETE_COPY_CTORS(callback_data);
      DEFAULT_MOVE_CTORS(callback_data);
};

struct my_uv_loop_data
{
    private:
      using handle_vector = std::vector<uv_poll_t *>;

    public:
      uv_loop_t     *loop    = new uv_loop_t;
      handle_vector  handles = {};

      my_uv_loop_data() = default;
      ~my_uv_loop_data()
      {
            if (loop->data)
                  delete static_cast<callback_data *>(loop->data);
            delete loop;
            for (auto const *val : handles)
                  delete val;
      }

      DELETE_COPY_CTORS(my_uv_loop_data);
      DELETE_MOVE_CTORS(my_uv_loop_data);
};

void
uv_poll_callback(uv_poll_t *hand, UNUSED int status, int events)
{
      if (events & UV_READABLE) {
            auto const *data = static_cast<callback_data *>(hand->data);
            auto const msg  = data->con->read_message();
            dump_message(msg);
            data->condition->notify_one();
      }
}

#if 1
void uvw_poll_callback (uvw::PollEvent const &ev, uvw::PollHandle &handle)
{
      if (ev.flags & uvw::details::UVPollEvent::READABLE) {
            auto       data = handle.data<callback_data>();
            auto const msg  = data->con->read_message();
            dump_message(msg);
            data->condition->notify_one();
      }
}
#endif

int my_event_loop(callback_data const *data)
{
      for (;;) {
            auto msg = data->con->read_message();
            dump_message(msg);
            data->condition->notify_all();
      }

      return 0;
}

} // namespace

NOINLINE void test03()
{
      static auto const init = emlsp::ipc::lsp::data::init_msg(test_rooturi);
      auto const buf = util::slurp_file(test_file);

#if 1
      {
            auto *const con = new con_type;
            con->spawn_connection_l(clangd_exe, "--pch-storage=memory", "--log=verbose", "--pretty");
            con->write_message(init);
            con->discard_message();

            auto  mut             = std::mutex{};
            auto  lock            = std::unique_lock{mut};
            auto *condition       = new std::condition_variable;
            auto *loop_data       = new my_uv_loop_data;
            auto *cb_data         = new callback_data{con, condition};
            auto *hand            = new uv_poll_t{};
            loop_data->loop->data = cb_data;
            hand->data            = cb_data;
            loop_data->handles.emplace_back(hand);

            uv_loop_init(loop_data->loop);
            uv_poll_init_socket(loop_data->loop, hand, con->connection().accepted());
            uv_poll_start(hand, UV_READABLE, uv_poll_callback);

            auto thrd = std::thread{uv_run, loop_data->loop, UV_RUN_DEFAULT};

            test03_2_1(*con, buf);
            condition->wait(lock);
            test03_3_1(*con);
            condition->wait(lock);
            test03_2_2(*con);
            condition->wait(lock);
            test03_2_4(*con);

            uv_poll_stop(hand);
            uv_stop(loop_data->loop);

#if defined _WIN32 && !defined HAVE_PTHREAD_H
            TerminateThread(thrd.native_handle(), 0);
#else
            pthread_cancel(thrd.native_handle());
#endif

            thrd.join();
            delete loop_data;
#if 0
            delete cb_data;
            delete condition;
            delete con;
#endif
      }
#endif

#if 0
      {
            auto *loop = new uv_loop_t;
            uv_spawn
      }
#endif

#if 0
# define AUTOC auto const
      {
            auto con = std::make_shared<con_type>();
            con->spawn_connection_l(clangd_exe, "--pch-storage=memory", "--log=verbose");
            con->write_message(init);

            auto loop      = uvw::Loop::create();
            auto handle    = loop->resource<uvw::PollHandle>(uvw::OSSocketHandle(con->connection()()));
            auto mut       = std::mutex{};
            auto lock      = std::unique_lock{mut};
            auto condition = std::make_shared<std::condition_variable>();
            auto data      = std::make_unique<callback_data>(con, condition);

            // loop->data(data);
            handle->data(std::move(data));
            handle->on<uvw::PollEvent>(std::function(uvw_poll_callback));
            handle->start(uvw::details::UVPollEvent::READABLE);

#if 0
            auto thrd = std::thread{[](std::shared_ptr<uvw::Loop> lp, std::shared_ptr<uvw::PollHandle> hand)
            {
                  lp->run();
                  hand->stop();
                  hand->close();
                  lp->close();
            }, loop, handle};
#endif

            auto thrd = std::thread{[](uvw::Loop *loop_)
            {
                  loop_->run();
            }, loop.get()};

            test03_2_1(*con, buf);
            condition->wait(lock);

            test03_2_2(*con);
            condition->wait(lock);

            test03_2_4(*con);

#ifdef _Win32
            TerminateThread(thrd.native_handle(), 0);
#else
            pthread_cancel(thrd.native_handle());
#endif

            thrd.join();
            handle->stop();
            handle->close();
            loop->close();
            // handle.reset(); loop.reset();
            loop.reset();
            con.reset();
            std::cerr << "JOINED! -->" << handle.use_count() << "  " << loop.use_count() << std::endl;

      }
#endif
}

} // namespace emlsp::event


namespace emlsp::ipc::event::test {



} // namespace emlsp::ipc::event::test
