#include "Common.hh"
#include "lsp-protocol/lsp-connection.hh"
#include "lsp-protocol/static-data.hh"

//#include <asio.hpp>
// #include <glib.h>

#if 0
#include <nanomsg/bus.h>
#include <nanomsg/inproc.h>
#include <nanomsg/ipc.h>
#include <nanomsg/pair.h>
#include <nanomsg/pipeline.h>
#include <nanomsg/pubsub.h>
#include <nanomsg/reqrep.h>
#include <nanomsg/survey.h>
#include <nanomsg/tcp.h>
#include <nanomsg/ws.h>
#include <nanomsg/nn.h>

#include <libusockets.h>
#endif

#if 0
#include <evpp/libevent.h>

#include <evpp/connector.h>
#include <evpp/event_loop.h>
#include <evpp/sockets.h>

#include <hv/hv.h>
#include <hv/EventLoop.h>
#endif

#if 0
#include <glibmm.h>
#include <glibmm/bytearray.h>
#include <glibmm/bytes.h>
#include <glibmm/checksum.h>
#include <glibmm/class.h>
#include <glibmm/containerhandle_shared.h>
#include <glibmm/convert.h>
#include <glibmm/date.h>
#include <glibmm/datetime.h>
#include <glibmm/dispatcher.h>
#include <glibmm/enums.h>
#include <glibmm/error.h>
#include <glibmm/exceptionhandler.h>
#include <glibmm/fileutils.h>
#include <glibmm/interface.h>
#include <glibmm/iochannel.h>
#include <glibmm/init.h>
#include <glibmm/keyfile.h>
#include <glibmm/main.h>
#include <glibmm/markup.h>
#include <glibmm/miscutils.h>
#include <glibmm/module.h>
#include <glibmm/nodetree.h>
#include <glibmm/objectbase.h>
#include <glibmm/object.h>
#include <glibmm/optioncontext.h>
#include <glibmm/pattern.h>
#include <glibmm/property.h>
#include <glibmm/propertyproxy_base.h>
#include <glibmm/propertyproxy.h>
#include <glibmm/quark.h>
#include <glibmm/random.h>
#include <glibmm/regex.h>
#include <glibmm/refptr.h>
#include <glibmm/shell.h>
#include <glibmm/signalproxy_connectionnode.h>
#include <glibmm/signalproxy.h>
#include <glibmm/spawn.h>
#include <glibmm/stringutils.h>
#include <glibmm/timer.h>
#include <glibmm/timezone.h>
#include <glibmm/uriutils.h>
#include <glibmm/ustring.h>
#include <glibmm/value.h>
#include <glibmm/variant.h>
#include <glibmm/variantdict.h>
#include <glibmm/variantiter.h>
#include <glibmm/varianttype.h>
#include <glibmm/vectorutils.h>
#include <glibmm/wrap.h>
#endif


namespace emlsp::ipc::event
{

#if 0
void test1()
{
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

      asio::posix::stream_descriptor desc((asio::any_io_executor()));
      desc.assign(1);
      asio::write(desc, foo);
      fsync(1);

      asio::local::stream_protocol proto{};
      auto sock = asio::basic_seq_packet_socket<asio::generic::seq_packet_protocol>(asio::any_io_executor());
      auto sock = asio::basic_seq_packet_socket<asio::generic::seq_packet_protocol>(asio::any_io_executor());
      auto proto = asio::generic::seq_packet_protocol(AF_UNIX, AF_UNIX);
      sock.open(proto);
}
#endif

void
test02()
{
}

} // namespace emlsp::ipc::event
