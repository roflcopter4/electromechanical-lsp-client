#include "Common.hh"
#include "ipc/lsp/lsp-client.hh"
#include "ipc/lsp/lsp-connection.hh"
#include "ipc/lsp/static-data.hh"
#include "libevent.hh"

#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/dns.h>
#include <event2/http.h>
#include <event2/listener.h>
#include <event2/thread.h>


#ifdef timespec
#  undef timespec
#endif
#define AUTOC auto const

/*--------------------------------------------------------------------------------------*/



inline namespace emlsp {
namespace libevent {
/****************************************************************************************/

namespace {

// #undef STRCHR_IS_FASTER_THAN_MEMCHR
std::string
replace_all(std::string &&str, char const tofind, char const replacement)
{
      char *ptr = str.data();

#ifdef STRCHR_IS_FASTER_THAN_MEMCHR
      while ((ptr = strchr(ptr, tofind)))
            *ptr++ = replacement;
#else
      size_t count = str.length() + 1;
      char  *c;
      while ((c = static_cast<char *>(memchr(ptr, tofind, count)))) {
            *c     = replacement;
            count -= util::ptr_diff(c, ptr) + SIZE_C(1);
            ptr    = c + 1U;
      }
#endif

      return str;
}

UNUSED __inline std::string
fix_backslashes(std::string &&str)
{
#ifdef _WIN32
      //return replace_all(std::move(str), '\\', '/');
      return std::filesystem::canonical(str).string();
#else
      return str;
#endif
}

constexpr uintmax_t nsec_to_second = UINTMAX_C(1'000'000'000);

UNUSED constexpr ::timespec
ts_from_second_fraction(uintmax_t const sec, uintmax_t const num, uintmax_t const den)
{
      return ::timespec{static_cast<int64_t>(sec), static_cast<decltype(::timespec::tv_nsec)>(num * nsec_to_second / den)};
}

UNUSED constexpr ::timespec
ts_from_double(double const val)
{
      return ::timespec{static_cast<int64_t>(val),
                        static_cast<decltype(::timespec::tv_nsec)>((val - static_cast<double>(static_cast<uintmax_t>(val))) *
                                          static_cast<double>(NSEC2SECOND))};
}

} // namespace

#if 0

using connection_type = ipc::lsp::connection<ipc::detail::unix_socket_connection_impl>;

static void
read_cb(UNUSED socket_t sock, short const flags, void *userdata) // NOLINT(google-runtime-int)
{
      static std::atomic_uint call_no = 0;
      auto *data = static_cast<connection_type *>(userdata);
      fmt::print(FC("\033[1;31mHERE IN {}, call number {}\033[0m\n"), __func__, call_no.fetch_add(1));
      if (flags & EV_READ) {
            auto msg = data->read_message();
            fmt::print(FC("\033[1;32mGot\033[0m \"{}\"\n"), msg.data());
      }
}

static void
loop_thrd_wrapper(connection_type *connection, event_base *loop)
{
      static std::atomic_uint iter = 0;

      constexpr timeval tv = {.tv_sec = 0, .tv_usec = 100'000};

      event *rd_handle = event_new(loop, connection->raw_descriptor(), EV_READ|EV_PERSIST, &read_cb, connection);
      event_add(rd_handle, &tv);

      // event_base_loop(loop, EVLOOP_NO_EXIT_ON_EMPTY);

      for (;;) {
            fmt::print(FC("\033[1;35mHERE IN {}, iter number {}\033[0m\n"), __func__, iter.fetch_add(1));
            event_base_loop(loop, 0);
            if (event_base_got_break(loop))
                  break;
            // event_add(rd_handle, nullptr);
            // event_base_dispatch(loop);
      }

      fmt::print(FC("\033[1;34mBreaking from event loop!\033[0m\n"));
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

      std::vector<char> contents   = util::slurp_file(fname);
      std::string       fname_uri  = "file://" + fname.string();
      AUTOC        init       = ipc::lsp::data::init_msg("file://" + fname.parent_path().string());
      auto             *connection = new connection_type();

      // connection->set_stderr_devnull();
      connection->spawn_connection_l("clangd", "--pch-storage=memory", "--log=verbose", "--clang-tidy", "--background-index");
      connection->write_message(init);

      auto thrd = std::thread{loop_thrd_wrapper, connection, loop};

      //connection->write_message_l(message_1);
      constexpr ::timespec ts = ts_from_second_fraction(1, 1, 10);
#ifdef HAVE_NANOSLEEP
      nanosleep(&ts, nullptr);
#else
      thrd_sleep(&ts, nullptr);
#endif

      fmt::print(FC("Sending break sig.\n"));
      event_base_loopbreak(loop);
      thrd.join();
      fmt::print(FC("Left event loop!\n"));

      event_base_free(loop);
      delete connection;
}

/*--------------------------------------------------------------------------------------*/

static void
read_cb_2(UNUSED socket_t sock, short const flags, void *userdata) // NOLINT(google-runtime-int)
{
      static std::atomic_uint call_no = 1;
      auto *data = static_cast<ipc::lsp::unix_socket_client<> *>(userdata);
      if (flags == EV_TIMEOUT) {
            call_no.fetch_add(1, std::memory_order::relaxed);
            return;
      }

      std::string foo;
      switch (flags) {
            case 0x01: foo = "timeout";  break;
            case 0x02: foo = "read";     break;
            case 0x04: foo = "write";    break;
            case 0x08: foo = "signal";   break;
            case 0x40: foo = "finalize"; break;
            case 0x80: foo = "close";    break;
            default:   foo = "unknown";  break;
      }

      fmt::print(FC("\033[1;31mHERE IN {}, call number {} -> {} ({})\033[0m\n"),
                 __func__, call_no.fetch_add(1, std::memory_order::relaxed), foo, flags);

      if (flags & EV_CLOSED)
            return;
      if (flags & EV_READ) {
            std::vector<char> msg;
            try {
                  msg = data->read_message();
            } catch (ipc::except::connection_closed const &) {
                  data->cond_notify_one();
                  return;
            }
            fmt::print(FC("\033[1;32mGot\033[0m \"{}\"\n"), msg.data());
            data->cond_notify_one();
      }
}


NOINLINE void
testing02(std::filesystem::path const &fname)
{
      AUTOC fname_uri  = "file://"s + fix_backslashes(fname.string());
      AUTOC init       = ipc::lsp::data::init_msg("file://"s + fix_backslashes(fname.parent_path().string()));
      AUTOC contentvec = util::slurp_file(fname);
      AUTOC contents   = std::string_view{contentvec.data(), contentvec.size() - SIZE_C(1)};

      //auto              client    = std::make_unique<ipc::lsp::client<ipc::lsp::connection<ipc::detail::unix_socket_connection_impl>>>();
      //auto *            client    = new ipc::lsp::client<ipc::unix_socket_connection>();
      //auto              client    = std::make_unique<ipc::lsp::unix_socket_client<>>();

      // auto client = std::make_unique<ipc::lsp::client<connection_type, void *>>();
      // auto *client = new ipc::lsp::client<connection_type, void *>();

      auto *client = new ipc::lsp::unix_socket_client<ipc::loops::libevent, void *>{};

      try {
            // auto shit = ipc::whatever_client<connection_type, rapidjson::Document>{dynamic_cast<ipc::base_client<connection_type, rapidjson::Document> *>(client.get())};
            // auto shit = ipc::whatever_client<connection_type, rapidjson::Document>{client.get()};

            // client->set_stderr_devnull();
            client->spawn_connection_l("clangd", "--pch-storage=memory", "--log=verbose", "--clang-tidy", "--background-index");
            client->write_message(init);
            client->loop().start(&read_cb_2);
            //client->cond_wait();

            {
                  ipc::json::rapid_doc wrap;
                  wrap.add_member ("jsonrpc", "2.0");
                  wrap.add_member ("method", "textDocument/didOpen");
                  wrap.set_member ("params");
                  wrap.set_member ("textDocument");
                  wrap.add_member ("uri", rapidjson::Value(fname_uri.data(), fname_uri.size()));
                  wrap.add_member ("text", rapidjson::Value(contents.data(), contents.size()));
                  wrap.add_member ("version", 1);
                  wrap.add_member ("languageId", "cpp");

                  client->write_message(wrap.doc());
                  client->cond_wait();
            }
            {
                  ipc::json::rapid_doc wrap;
                  wrap.add_member ("jsonrpc", "2.0");
                  wrap.add_member ("id", 2);
                  wrap.add_member ("method", "textDocument/semanticTokens/full");
                  wrap.set_member ("params");
                  wrap.set_member ("textDocument");
                  wrap.add_member ("uri", rapidjson::Value(fname_uri.data(), fname_uri.size()));

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
      } catch (...) {
            delete client;
            client = nullptr;
            throw;
      }

      delete client;

}

#endif

namespace literals {
using ullong = unsigned long long; //NOLINT
constexpr auto operator""_cs(ullong const val)
{
      return std::chrono::duration<ullong, std::ratio<1, 10>>{val};
}
} // namespace literals
using literals::operator""_cs;


template <typename T> void
dispose_msg(std::unique_ptr<T> const &ptr)
{
      static char buf[65536]{};

      AUTOC nread = ptr->raw_read(buf, sizeof(buf), 0);

      fflush(stderr);
      fmt::print(stderr, FC("\033[1;31mRead {} bytes <<'_EOF_'\n\033[0;36m{}\n\033[1;31m_EOF_\033[0m\n"),
                 nread, std::string_view(buf, nread));
      fflush(stderr);
}

template <typename T> void
write_msg(std::unique_ptr<T> const &conn, ipc::json::rapid_doc<> &wrap)
{

      rapidjson::MemoryBuffer ss;
      rapidjson::Writer       writer(ss);
      wrap.doc().Accept(writer);

      char  buf[60];
      AUTOC buflen = ::sprintf(buf, "Content-Length: %zu\r\n\r\n", ss.GetSize());

      fflush(stderr);
      fmt::print(stderr, FC("\033[1;33mwriting {} bytes <<'_eof_'\n\033[0;32m{}{}\033[1;33m\n_eof_\033[0m\n"),
                 ss.GetSize() + buflen, std::string_view(buf, buflen),
                 std::string_view{ss.GetBuffer(), ss.GetSize()});
      fflush(stderr);

      conn->raw_write(buf, static_cast<size_t>(buflen));
      conn->raw_write(ss.GetBuffer(), ss.GetSize());
}


NOINLINE void
testing03(std::filesystem::path const &fname)
{
      using conn_type = ipc::connections::spawn_default;
      //using conn_type = ipc::connections::spawn_pipe;
      //using conn_type = ipc::connections::spawn_win32_named_pipe;

      //AUTOC fname_uri = /*"file://"s +*/ fix_backslashes(fname.string());
      constexpr auto fname_uri = R"(file:/D:\\ass\\VisualStudio\\Foo_Bar\\elmwin32\\src\\libevent.cc)"sv;
      constexpr auto proj_path = R"(file:/D:\\ass\\VisualStudio\\Foo_Bar\\elmwin32)"sv;

      auto con = conn_type::new_unique();

      con->set_stderr_default();
      con->spawn_connection_l(R"(D:\Program Files\Microsoft Visual Studio\2022\Preview\VC\Tools\Llvm\x64\bin\clangd.exe)",
                              "--pch-storage=memory", "--log=verbose", "--clang-tidy");

      {
            //AUTOC init = ipc::lsp::data::init_msg(/*"file://"s +*/ fix_backslashes(fname.parent_path().parent_path().string()));
            AUTOC init = ipc::lsp::data::init_msg(proj_path.data());

            char buf[60];
            auto buflen = ::sprintf(buf, "Content-Length: %zu\r\n\r\n", init.size());
            con->raw_write(buf, static_cast<size_t>(buflen));
            con->raw_write(init);
      }

      std::this_thread::sleep_for(1s);
      dispose_msg<conn_type>(con);

      {
            AUTOC contentvec = util::slurp_file(fname);
            ipc::json::rapid_doc wrap;
            wrap.add_member("jsonrpc", "2.0");
            wrap.add_member("method", "textDocument/didOpen");
            wrap.set_member("params");
            wrap.set_member("textDocument");
            wrap.add_member("uri",  rapidjson::Value ( fname_uri.data(),  static_cast<rapidjson::SizeType>(fname_uri.size()) ) );
            wrap.add_member("text", rapidjson::Value { contentvec.data(), static_cast<rapidjson::SizeType>(contentvec.size() - SIZE_C(1)) } );
            wrap.add_member("version", 1);
            wrap.add_member("languageId", "cpp");

            write_msg<conn_type>(con, wrap);
      }
      {
            ipc::json::rapid_doc wrap;
            wrap.add_member("jsonrpc", "2.0");
            wrap.add_member("id", 1);
            wrap.add_member("method", "textDocument/semanticTokens/full");
            wrap.set_member("params");
            wrap.set_member("textDocument");
            wrap.add_member("uri", rapidjson::Value(fname_uri.data(), fname_uri.size()));

            write_msg<conn_type>(con, wrap);
      }

      std::this_thread::sleep_for(100ms);
      dispose_msg<conn_type>(con);
      dispose_msg<conn_type>(con);
      fflush(stderr);
      fmt::print(stderr, FC("Finished waiting!\n"));
      fflush(stderr);
}

static void how_should_I_know(event_base *base, std::unique_ptr<ipc::connections::spawn_unix_socket> const &client);

NOINLINE void
testing04(std::filesystem::path const &fname)
{
      constexpr auto fname_uri = R"(file:/D:\\ass\\VisualStudio\\Foo_Bar\\elmwin32\\src\\libevent.cc)"sv;
      constexpr auto proj_path = R"(file:/D:\\ass\\VisualStudio\\Foo_Bar\\elmwin32)"sv;

      event_enable_debug_mode();
      evthread_enable_lock_debugging();
      event_enable_debug_logging(EVENT_DBG_ALL);
      evthread_use_windows_threads();
      //evthread_use_pthreads();
      event_base *base;
      auto con = ipc::connections::spawn_unix_socket::new_unique();
      //con->set_stderr_fd ( GetStdHandle(STD_OUTPUT_HANDLE));
      con->spawn_connection_l(R"(D:\Program Files\Microsoft Visual Studio\2022\Preview\VC\Tools\Llvm\x64\bin\clangd.exe)",
                              "--pch-storage=memory", "--log=verbose", "--clang-tidy");


      {
            //event_config *cfg = ::event_config_new();
            //::event_config_require_features(cfg, EV_FEATURE_EARLY_CLOSE);
#ifdef _WIN32
            //::event_config_set_flag(cfg, EVENT_BASE_FLAG_STARTUP_IOCP);
#endif
            //base = ::event_base_new_with_config(cfg);
            base = ::event_base_new();
            if (!base)
                  util::win32::error_exit(L"Who fucking knows");
            //::event_config_free(cfg);
      }

      bufferevent *evb = bufferevent_socket_new(base, con->raw_descriptor(), BEV_OPT_THREADSAFE);
      assert(evb != nullptr);
      bufferevent_enable(evb, EV_READ | EV_WRITE);

      {
            //AUTOC init = ipc::lsp::data::init_msg(("file:/"s + fix_backslashes(fname.parent_path().parent_path().string())).c_str());
            AUTOC init = ipc::lsp::data::init_msg(fname_uri.data());
            char buf[60];
            auto buflen = ::sprintf(buf, "Content-Length: %zu\r\n\r\n", init.size());

            bufferevent_write(evb, buf, buflen);
            bufferevent_write(evb, init.data(), init.size());
            bufferevent_flush(evb, EV_WRITE, BEV_FLUSH);

            constexpr auto slptm = 2500ms;
            std::this_thread::sleep_for(slptm);
            std::string rd;
            rd.reserve(65536);
            buflen = bufferevent_read(evb, rd.data(), rd.max_size());
            util::resize_string_hack(rd, buflen);

            fmt::print(stderr, FC("\n\nRead {} bytes <<_EOF_\n{}\n_EOF_\n\n"), buflen, rd);
      }
}


static void
dumb_callback(UNUSED evutil_socket_t sock, short const flags, void *userdata) // NOLINT(google-runtime-int)
{

}

static void
how_should_I_know(event_base *base, std::unique_ptr<ipc::connections::spawn_unix_socket> const &client)
{
      static constexpr timeval tv = {.tv_sec = 0, .tv_usec = 100'000} /* 1/10 seconds */;
      static std::atomic_uint64_t iter = UINT64_C(0);

      event *rd_handle = ::event_new(base, client->raw_descriptor(), EV_READ | EV_PERSIST, dumb_callback, nullptr);
      ::event_add(rd_handle, &tv);

      for (;;) {
            uint64_t const n = iter.fetch_add(1);
            fmt::print(FMT_COMPILE("\033[1;35mHERE IN {}, iter number {}\033[0m\n"),
                       FUNCTION_NAME, n);

            if (::event_base_loop(base, 0) == 1) [[unlikely]] {
                  fmt::print(stderr, "\033[1;31mERROR: No registered events!\033[0m\n");
                  break;
            }
            if (::event_base_got_break(base)) [[unlikely]]
                  break;
      }

      ::event_free(rd_handle);
}


/****************************************************************************************/

namespace util {

static int
ersatz_socketpair(int family, int type, int protocol, _Notnull_ socket_t fd[2]);


int
socketpair(int const          family,
           int const          type,
           int const          protocol,
           _Notnull_ socket_t fd[2])
{
#ifdef HAVE_SOCKETPAIR
	return ::socketpair(family, type, protocol, fd);
#elif defined _WIN32
	return ersatz_socketpair(family, type, protocol, fd);
#else
# error "Impossible error"
#endif
}


static int
ersatz_socketpair(int const          family,
                  int const          type,
                  int const          protocol,
                  _Notnull_ socket_t fd[2])
{
	/* This code is originally from Tor.  Used with permission. */
        /* ... and then stolen from libevent2, without permission.  */

	/* This socketpair does not work when localhost is down. So
	 * it's really not the same thing at all. But it's close enough
	 * for now, and really, when localhost is down sometimes, we
	 * have other problems too.
	 */
#ifdef _WIN32
# define ERR(e) WSA##e
#else
# define ERR(e) e
#endif
	struct sockaddr_in listen_addr;
	struct sockaddr_in connect_addr;

        socklen_t size;
	intptr_t  listener;
        intptr_t  connector   = -1;
        intptr_t  acceptor    = -1;
        int       saved_errno = -1;
        bool      family_test = family != AF_INET;

#ifdef AF_UNIX
	family_test = family_test && (family != AF_UNIX);
#endif
	if (protocol || family_test) {
		EVUTIL_SET_SOCKET_ERROR(ERR(EAFNOSUPPORT));
		return -1;
	}
	if (!fd) {
		EVUTIL_SET_SOCKET_ERROR(ERR(EINVAL));
		return -1;
	}

	listener = static_cast<intptr_t>(socket(AF_INET, type, 0));
	if (listener < 0)
		return -1;

	memset(&listen_addr, 0, sizeof(listen_addr));
        listen_addr.sin_family      = AF_INET;
        listen_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        listen_addr.sin_port        = 0; /* kernel chooses port. */

	if (bind(listener, reinterpret_cast<sockaddr *>(&listen_addr), sizeof(listen_addr)) == -1)
		goto tidy_up_and_fail;
	if (listen(listener, 1) == -1)
		goto tidy_up_and_fail;

	connector = static_cast<intptr_t>(socket(AF_INET, type, 0));
	if (connector < 0)
		goto tidy_up_and_fail;

	memset(&connect_addr, 0, sizeof(connect_addr));

	/* We want to find out the port number to connect to.  */
	size = sizeof(connect_addr);
	if (getsockname(listener, reinterpret_cast<sockaddr *>(&connect_addr), &size) == -1)
		goto tidy_up_and_fail;
	if (size != sizeof (connect_addr))
		goto abort_tidy_up_and_fail;
	if (connect(connector, reinterpret_cast<sockaddr *>(&connect_addr), sizeof(connect_addr)) == -1)
		goto tidy_up_and_fail;

	size     = sizeof(listen_addr);
	acceptor = static_cast<intptr_t>(accept(listener, reinterpret_cast<sockaddr *>(&listen_addr), &size));

	if (acceptor < 0)
		goto tidy_up_and_fail;
	if (size != sizeof(listen_addr))
		goto abort_tidy_up_and_fail;

	/* Now check we are talking to ourself by matching port
         * and host on the two sockets.	 */
	if (getsockname(connector, reinterpret_cast<sockaddr *>(&connect_addr), &size) == -1)
		goto tidy_up_and_fail;
        if (size != sizeof(connect_addr) ||
            listen_addr.sin_family != connect_addr.sin_family ||
            listen_addr.sin_addr.s_addr != connect_addr.sin_addr.s_addr ||
            listen_addr.sin_port != connect_addr.sin_port)
	{
		goto abort_tidy_up_and_fail;
        }

	closesocket(listener);
	fd[0] = connector;
	fd[1] = acceptor;

	return 0;

 abort_tidy_up_and_fail:
	saved_errno = ERR(ECONNABORTED);
 tidy_up_and_fail:
	if (saved_errno < 0)
		saved_errno = EVUTIL_SOCKET_ERROR();
	if (listener != -1)
		closesocket(listener);
	if (connector != -1)
		closesocket(connector);
	if (acceptor != -1)
		closesocket(acceptor);

	EVUTIL_SET_SOCKET_ERROR(saved_errno);
	return -1;
#undef ERR
}

} // namespace util


/****************************************************************************************/
} // namespace libevent
} // namespace emlsp
