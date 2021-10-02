#include "Common.hh"
#include "rapid.hh"

#include <asio.hpp>
#include <glib.h>

#undef unix

namespace emlsp::rpc::event
{

void test1()
{
#if 0
      auto buf = asio::streambuf();
      std::ostream os(&buf);
      os << "HELLO, IDIOT!\n";
      os << "HELLO, IDIOT!\n";
      os << "HELLO, IDIOT!\n";
      os << "HELLO, IDIOT!\n";

      auto exe = asio::io_context();
      auto desc = asio::posix::stream_descriptor( exe );

      desc.assign(STDOUT_FILENO);
      assert(desc.is_open());
      size_t n;

      do {
            auto foo = asio::const_buffer(buf.data());
            n = desc.write_some(foo);
            buf.consume(n);
      } while (n > 0);

      fsync(desc.native_handle());
#endif


#if 0
      asio::posix::stream_descriptor desc((asio::any_io_executor()));
      desc.assign(1);
      asio::write(desc, foo);
      fsync(1);
#endif

#if 0
      asio::local::stream_protocol proto{};
      auto sock = asio::basic_seq_packet_socket<asio::generic::seq_packet_protocol>(asio::any_io_executor());
#endif

#if 0
      auto sock = asio::basic_seq_packet_socket<asio::generic::seq_packet_protocol>(asio::any_io_executor());
      auto proto = asio::generic::seq_packet_protocol(AF_UNIX, AF_UNIX);
      sock.open(proto);
#endif
}

static void
test_read(GPid const pid, int const writefd, int const readfd)
{
      size_t msglen;
      {
            char ch;
            char buf[128];
            char *ptr = buf;
            read(readfd, buf, 16); // Discard
            read(readfd, &ch, 1);  // Read 1st potential digit

            while (isdigit(ch)) {
                  *ptr++ = ch;
                  read(readfd, &ch, 1);
            }

            if (ptr == buf)
                  throw std::runtime_error("No message length specified in jsonrpc header.");

            *ptr = '\0';
            msglen = strtoull(buf, nullptr, 10);
            read(readfd, buf, 3);
      }

      char *msg = new char[msglen + 1];
      read(readfd, msg, msglen);
      msg[msglen] = '\0';
      {
            auto obj = rapidjson::Document();
            obj.ParseInsitu<rapidjson::kParseInsituFlag>(msg);

            rapidjson::StringBuffer ss;
            auto writer = rapidjson::PrettyWriter(ss);
            rapidjson::Value{};
            obj.Accept(writer);

            std::cout << "\n\n----- Object (rapidjson):\n" << ss.GetString() << std::endl;
      }

      delete[] msg;
      close(writefd);
      close(readfd);
      waitpid(pid, nullptr, 0);
}

} // namespace emlsp::rpc::event
