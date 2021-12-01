#include "Common.hh"
#include "client.hh"
#include "event/loop.hh"
#include "lsp-protocol/lsp-connection.hh"
#include "lsp-protocol/static-data.hh"
#include "msgpack/dumper.hh"
#include "util/exceptions.hh"

#include <uvw.hpp>

#include <msgpack.hpp>

#define AUTOC auto const

namespace emlsp::ipc::mpack {
std::string encode_fmt(unsigned size_hint, char const *__restrict fmt, ...);
} // namespace emlsp::ipc::mpack

namespace emlsp::ipc::lsp::test {

namespace {

UNUSED constexpr char test_rooturi[] = R"("file:///D:/ass/VisualStudio/elemwin")";
UNUSED constexpr char test_uri[]     = R"(file:///D:/ass/VisualStudio/elemwin/src/main.cc)";
UNUSED constexpr char test_file[]    = R"(D:\ass\VisualStudio\elemwin\src\main.cc)";
UNUSED constexpr char clangd_exe[]   = R"(D:\new_msys64\ucrt64\bin\clangd.exe)";

template <emlsp::ipc::detail::BaseConnectionSubclass ConnectionType>
void connection_read_loop(UNUSED ConnectionType &con)
{
}

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
            fprintf(stdout, "\n\033[1;32m------ READ MSG OF LENGTH %zu ------\033[0m\n%.*s\n"
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

UNUSED __inline void
write_and_dump(unix_socket_connection &con, rapidjson::Document const &doc)
{
      rapidjson::MemoryBuffer ss;
      rapidjson::Writer writer(ss);
      doc.Accept(writer);
      dump_message<true>(ss.GetBuffer(), ss.GetSize());
      con.write_message(ss.GetBuffer(), ss.GetSize());
}

} // namespace

class client_ptr : public std::shared_ptr<rapidjson_unix_socket_client<>>
{
    public:
      unix_socket_connection &operator()() const { return get()->con(); }
};

struct callback_data
{
    private:
      using condition_ptr = std::shared_ptr<std::condition_variable>;

    public:
      client_ptr    client     = {};
      condition_ptr condition  = {};

      callback_data() = default;
      callback_data(client_ptr cl, condition_ptr cnd)
          : client(std::move(cl)), condition(std::move(cnd))
      {assert(false);}
};

void uvw_poll_callback (uvw::PollEvent const &ev, uvw::PollHandle &handle)
{
      if (ev.flags & uvw::details::UVPollEvent::READABLE) {
            char *msg;
            auto const data = handle.data<callback_data>();
            // auto const msg  = data->client().read_message();
            data->client().read_message(&msg);
            dump_message(msg);
            data->condition->notify_one();

            rapidjson::Document doc;
            doc.ParseInsitu<rapidjson::kParseInsituFlag>(msg);
      }
}

static constexpr char test_filename[] = R"(D:\Vim\dein\repos\github.com\roflcopter4\tag-highlight.nvim\tag-highlight\MSVC\whatever)";
//static constexpr char test_filename[] = R"(/home/bml/files/Projects/Again/foo)";

NOINLINE void test10()
{
      AUTOC init = data::init_msg(test_rooturi);
      auto  con  = unix_socket_connection{};
      con.spawn_connection_l(clangd_exe, "--pch-storage=memory", "--log=verbose");
      con.write_message(init);

      AUTOC loop      = uvw::Loop::create();
      AUTOC handle    = loop->resource<uvw::PollHandle>(uvw::OSSocketHandle(con()()));
      auto  mut       = std::mutex{};
      auto  lock      = std::unique_lock{mut};
      auto  condition = std::make_shared<std::condition_variable>();
      auto  client    = client_ptr { std::make_shared<rapidjson_unix_socket_client<>>(std::move(con)) };
      AUTOC data      = std::make_shared<callback_data>(std::move(client), condition);

      loop->data(data);
      handle->data(data);
      handle->on<uvw::PollEvent>(std::function(uvw_poll_callback));
      handle->start(uvw::details::UVPollEvent::READABLE);
}

NOINLINE void test11()
{

      msgpack::unpacker unpack{
            [](msgpack::type::object_type, size_t, void *) {
                  return true;
            },
            nullptr, 65536
      };

      auto *fp    = std::fopen(test_filename, "rb");
      auto  nread = std::fread(unpack.buffer(), 1, 65536, fp);
      std::fclose(fp);

      //fp = std::fopen(test_filename, "wb");
      //std::fwrite(unpack.buffer(), 1, nread, fp);
      //std::fwrite(unpack.buffer(), 1, nread, fp);
      //fclose(fp);

      msgpack::unpacked unpacked;
      unpack.buffer_consumed(nread);

      unpack.next(unpacked);
      mpack::dumper(&unpacked.get());
      unpack.next(unpacked);
      mpack::dumper(&unpacked.get());
}


NOINLINE void test12()
{
#if 0
#if 0
      std::stringstream ss;
      msgpack::packer   pack{ss};

      pack.pack_array(4);

      pack.pack_array(0);

      pack.pack_array(1);
      pack.pack_nil();

      pack.pack_array(4);
      pack << UINT64_MAX << static_cast<int>(~1U + 1U) << -13 << 321.992;

      pack.pack_map(3);
      pack << "key" << "value";
      pack << "key" << 99;
      pack << "blabla";
      pack.pack_map(0);

      std::cout << '"' << ss.str() << "\"\n\n";
#endif

      std::string foo = "turd";
      auto str = emlsp::ipc::mpack::encode_fmt(0, "[{c:d, c:d}, l, l, u, s]", "one", 1, "two", 2, 3LL, 4LL, 5ULL, &foo);

      std::cout << '"' << str << "\"\n\n";

      {
            msgpack::unpacker unpack;
            unpack.reserve_buffer(str.size());
            memcpy(unpack.buffer(), str.data(), str.size());
            unpack.buffer_consumed(str.size());
#if 0
            unpack.reserve_buffer(ss.str().size());
            memcpy(unpack.buffer(), ss.str().data(), ss.str().size());
            unpack.buffer_consumed(ss.str().size());
#endif

            {
                  msgpack::unpacked unpacked;
                  unpack.next(unpacked);
                  mpack::dumper(&unpacked.get());
            }
      }
#endif
}

} // namespace emlsp::ipc::lsp::test
