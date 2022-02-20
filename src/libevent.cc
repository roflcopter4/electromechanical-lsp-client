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

#include <uv.h>

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

/*--------------------------------------------------------------------------------------*/

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


/*--------------------------------------------------------------------------------------*/


NOINLINE void
testing03(std::filesystem::path const &fname)
{
      //using conn_type = ipc::connections::spawn_default;
      using conn_type = ipc::connections::pipe;
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
      (void)fflush(stderr);
      fmt::print(stderr, FC("Finished waiting!\n"));
}

/*--------------------------------------------------------------------------------------*/

static void how_should_I_know(event_base *base, std::unique_ptr<ipc::connections::unix_socket> const &client);

NOINLINE void
testing04()
{
      using conn_type = ipc::connections::inet_socket;

      // constexpr auto fname_uri = R"(file:/D:\\ass\\VisualStudio\\Foo_Bar\\elmwin32\\src\\libevent.cc)"sv;
      // constexpr auto proj_path = R"(file:/D:\\ass\\VisualStudio\\Foo_Bar\\elmwin32)"sv;

      UNUSED static constexpr auto fname_uri = R"(file://home/bml/files/Projects/sigh-elm/src/libevent.cc)"sv;
      UNUSED static constexpr auto proj_path = R"(file://home/bml/files/Projects/sigh-elm)"sv;
      UNUSED static constexpr auto raw_fname = R"(/home/bml/files/Projects/sigh-elm/src/libevent.cc)"sv;

      // event_enable_debug_mode();
      // evthread_enable_lock_debugging();
      // event_enable_debug_logging(EVENT_DBG_ALL);
      // evthread_use_windows_threads();
      evthread_use_pthreads();

      event_base *base;
      auto        con = conn_type::new_unique();
      con->impl().resolve("127.0.0.1:59231");

      //con->set_stderr_fd ( GetStdHandle(STD_OUTPUT_HANDLE));
      // con->spawn_connection_l(R"(D:\Program Files\Microsoft Visual Studio\2022\Preview\VC\Tools\Llvm\x64\bin\clangd.exe)",
      //                         "--pch-storage=memory", "--log=verbose", "--clang-tidy");

      con->spawn_connection_l("clangd", "--pch-storage=memory", "--log=verbose", "--clang-tidy", "--sync");


      {
            event_config *cfg = ::event_config_new();
            ::event_config_require_features(cfg, EV_FEATURE_EARLY_CLOSE);
#ifdef _WIN32
            //::event_config_set_flag(cfg, EVENT_BASE_FLAG_STARTUP_IOCP);
#endif
            base = ::event_base_new_with_config(cfg);
            if (!base)
                  err(1, "who knows");
            ::event_config_free(cfg);
      }

      bufferevent *evb = bufferevent_socket_new(base, con->raw_descriptor(), BEV_OPT_THREADSAFE);
      assert(evb != nullptr);

      ::bufferevent_setcb(
          evb,
          [](UNUSED bufferevent *bufev, UNUSED void *data) {
                fmt::print(stderr, FC("Read callback called?\n"));
                (void)fflush(stderr);
          },
          [](UNUSED bufferevent *bufev, UNUSED void *data) {
                fmt::print(stderr, FC("Write callback called?\n"));
                (void)fflush(stderr);
          },
          // [](UNUSED bufferevent *, UNUSED short [>NOLINT(google-runtime-int)<], UNUSED void *) {},
          nullptr,
          nullptr);


      void *(*loop_routine)(void *) = {[] (void *arg) -> void *
      {
            auto *evbuf  = static_cast<bufferevent *>(arg);
            auto *evbase = ::bufferevent_get_base(evbuf);

            for (;;) {
                  static std::atomic_uint64_t iter = UINT64_C(0);
                  ::bufferevent_enable(evbuf, EV_READ | EV_WRITE);

                  uint64_t const n = iter.fetch_add(1);
                  fmt::print(stderr, FC("\033[1;35mHERE IN '{}`, iter number {}\033[0m\n"), FUNCTION_NAME, n);

                  ::event_base_loop(evbase, 0);
                  std::this_thread::sleep_for(10ms);
                  //if (::event_base_loop(evbase, 0) == 1) [[unlikely]] {
                  //      fmt::print(stderr, "\033[1;31mERROR: No registered events!\033[0m\n");
                  //      break;
                  //}
                  if (::event_base_got_break(evbase)) [[unlikely]] {
                        fmt::print(stderr, "\033[1;33mBreaking from the loop...\033[0m\n");
                        break;
                  }
            }

            ::pthread_exit(nullptr);
      }};

      pthread_t thrd;
      ::pthread_create(&thrd, nullptr, loop_routine, evb);

      auto const dispose = [evb]() {
            constexpr auto slptm = 2500ms;
            std::this_thread::sleep_for(slptm);
            std::string rd;
            rd.reserve(65536);
            size_t const buflen = bufferevent_read(evb, rd.data(), rd.max_size());
            util::resize_string_hack(rd, buflen);

            fmt::print(stderr, FC("\n\n\033[0;33mRead {} bytes <<_EOF_\n{}\n_EOF_\033[0m\n\n"), buflen, rd);
      };
      auto const writemsg = [evb](ipc::json::rapid_doc<> & wrap) {
            rapidjson::MemoryBuffer ss;
            rapidjson::Writer       writer(ss);
            wrap.doc().Accept(writer);

            char  buf[60];
            AUTOC buflen = ::sprintf(buf, "Content-Length: %zu\r\n\r\n", ss.GetSize());

            (void)fflush(stderr);
            fmt::print(stderr, FC("\033[1;33mwriting {} bytes <<'_eof_'\n\033[0;32m{}{}\033[1;33m\n_eof_\033[0m\n"),
                        ss.GetSize() + buflen, std::string_view(buf, buflen),
                        std::string_view{ss.GetBuffer(), ss.GetSize()});
            (void)fflush(stderr);
            ::bufferevent_write(evb, buf, buflen);
            ::bufferevent_write(evb, ss.GetBuffer(), ss.GetSize());
            ::bufferevent_flush(evb, EV_WRITE, BEV_FLUSH);
      };

      {
            //AUTOC init = ipc::lsp::data::init_msg(("file:/"s + fix_backslashes(fname.parent_path().parent_path().string())).c_str());

            AUTOC init = ipc::lsp::data::init_msg(fname_uri.data());
            char buf[60];
            auto buflen = static_cast<size_t>(::sprintf(buf, "Content-Length: %zu\r\n\r\n", init.size()));

            ::bufferevent_write(evb, buf, buflen);
            ::bufferevent_write(evb, init.data(), init.size());
            ::bufferevent_flush(evb, EV_WRITE, BEV_FLUSH);

            dispose();
      }
      {
            AUTOC contentvec = util::slurp_file(raw_fname);
            ipc::json::rapid_doc wrap;
            wrap.add_member("jsonrpc", "2.0");
            wrap.add_member("method", "textDocument/didOpen");
            wrap.set_member("params");
            wrap.set_member("textDocument");
            wrap.add_member("uri",  rapidjson::Value ( fname_uri.data(),  static_cast<rapidjson::SizeType>(fname_uri.size()) ) );
            wrap.add_member("text", rapidjson::Value { contentvec.data(), static_cast<rapidjson::SizeType>(contentvec.size() - SIZE_C(1)) } );
            wrap.add_member("version", 1);
            wrap.add_member("languageId", "cpp");

            writemsg(wrap);
            dispose();
      }

      ::event_base_loopbreak(base);
      ::pthread_join(thrd, nullptr);
      ::bufferevent_disable(evb, EV_READ|EV_WRITE);
      ::bufferevent_free(evb);
      ::event_base_free(base);
}


static void
dumb_callback(UNUSED evutil_socket_t sock, short const flags, void *userdata) // NOLINT(google-runtime-int)
{

}

static void
how_should_I_know(event_base *base, std::unique_ptr<ipc::connections::unix_socket> const &client)
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

extern socket_t fuck_my_ass(char *servername);
extern int socketpair       (int family, int type, int protocol, _Notnull_ socket_t fd[2]);
extern int ersatz_socketpair(int family, int type, int protocol, _Notnull_ socket_t fd[2]);


socket_t fuck_my_ass(char *servername)
{
      char const *server   = servername;
      char       *servport = strchr(servername, ':');
      if (!servport)
            errx(1, "no port identified");
      *servport++ = '\0';

      socket_t sd = -1;
      int      rc;

      struct in6_addr  serveraddr = {};
      struct addrinfo  hints      = {};
      struct addrinfo *res        = nullptr;

      do {
            memset(&serveraddr, 0, sizeof(serveraddr));
            memset(&hints, 0, sizeof(hints));
            hints.ai_flags    = AI_NUMERICSERV | AI_NUMERICHOST;
            hints.ai_family   = AF_INET;
            hints.ai_socktype = SOCK_STREAM;

            rc = inet_pton(AF_INET, server, &serveraddr);

            /* valid IPv4 text address? */
            if (rc == 1) {
                  hints.ai_family = AF_INET;
                  hints.ai_flags |= AI_NUMERICHOST;
            } else {
                  rc = inet_pton(AF_INET, server, &serveraddr);

                  /* valid IPv6 text address? */
                  if (rc == 1) {
                        hints.ai_family = AF_INET6;
                        hints.ai_flags |= AI_NUMERICHOST;
                  }
            }

            rc = getaddrinfo(server, servport, &hints, &res);
            if (rc != 0) {
                  warnx("Host not found --> %s", gai_strerror(rc));
                  if (rc == EAI_FAIL)
                        err(1, "getaddrinfo() failed");
                  break;
            }

            sd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
            if (sd == static_cast<socket_t>(-1)) {
                  err(1, "socket() failed");
                  // err(1, "connect() %s", gai_strerror(WSAGetLastError()));
                  break;
            }

            rc = connect(sd, res->ai_addr, res->ai_addrlen);
            if (rc < 0) {
                  /*****************************************************************/
                  /* Note: the res is a linked list of addresses found for server. */
                  /* If the connect() fails to the first one, subsequent addresses */
                  /* (if any) in the list can be tried if required.               */
                  /*****************************************************************/
                  // err(1, "connect() %s", gai_strerror(WSAGetLastError()));
                  err(1, "connect()");
                  break;
            }
      } while (false);


      if (res != nullptr)
            freeaddrinfo(res);
      return sd;
}




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
#error "Impossible error"
#endif
}


int
ersatz_socketpair(int const family, int const type, int const protocol, _Notnull_ socket_t fd[2])
{
      /* This code is originally from Tor.  Used with permission. */
      /* ... and then stolen from libevent2, without permission.  */

      /* This socketpair does not work when localhost is down. So
       * it's really not the same thing at all. But it's close enough
       * for now, and really, when localhost is down sometimes, we
       * have other problems too.
       */
#ifdef _WIN32
#define ERR(e) WSA##e
#else
#define ERR(e) e
#endif

      socklen_t size;
      intptr_t  listener;
      intptr_t  connector   = -1;
      intptr_t  acceptor    = -1;
      int       saved_errno = -1;
      bool      family_test = family != AF_INET6;

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

      listener = static_cast<intptr_t>(socket(AF_INET6, type, 0));
      if (listener < 0)
            return -1;

      struct sockaddr_in6 listen_addr  = {};
      struct sockaddr_in6 connect_addr = {};

      listen_addr.sin6_addr   = IN6ADDR_LOOPBACK_INIT;
      listen_addr.sin6_port   = 0;
      listen_addr.sin6_family = AF_INET6;


      if (bind(static_cast<socket_t>(listener), reinterpret_cast<sockaddr *>(&listen_addr),
               sizeof(listen_addr)) == -1)
            goto tidy_up_and_fail;

      if (listen(static_cast<socket_t>(listener), 1) == -1)
            goto tidy_up_and_fail;

      connector = static_cast<intptr_t>(socket(AF_INET6, type, 0));
      if (connector < 0)
            goto tidy_up_and_fail;

      /* We want to find out the port number to connect to.  */
      size = sizeof(connect_addr);
      if (getsockname(static_cast<socket_t>(listener), reinterpret_cast<sockaddr *>(&connect_addr),
                      &size) == -1)
            goto tidy_up_and_fail;
      if (size != sizeof(connect_addr))
            goto abort_tidy_up_and_fail;

      if (connect(static_cast<socket_t>(connector), reinterpret_cast<sockaddr *>(&connect_addr),
                  sizeof(connect_addr)) == -1)
            goto tidy_up_and_fail;

      size     = sizeof(listen_addr);
      acceptor = static_cast<intptr_t>(accept(static_cast<socket_t>(listener),
                                              reinterpret_cast<sockaddr *>(&listen_addr), &size));

      if (acceptor < 0)
            goto tidy_up_and_fail;
      if (size != sizeof(listen_addr))
            goto abort_tidy_up_and_fail;

      /* Now check we are talking to ourself by matching port
       * and host on the two sockets. */
      if (getsockname(static_cast<socket_t>(connector), reinterpret_cast<sockaddr *>(&connect_addr),
                      &size) == -1)
            goto tidy_up_and_fail;

      if (sizeof(connect_addr) != size || listen_addr.sin6_family != connect_addr.sin6_family ||
          listen_addr.sin6_port != connect_addr.sin6_port ||
          ::memcmp(&listen_addr.sin6_addr, &connect_addr.sin6_addr,
                   sizeof(listen_addr.sin6_addr)) != 0) {
            goto abort_tidy_up_and_fail;
      }

      ::util::close_descriptor(listener);
      fd[0] = static_cast<socket_t>(connector);
      fd[1] = static_cast<socket_t>(acceptor);

      return 0;

abort_tidy_up_and_fail:
      saved_errno = ERR(ECONNABORTED);
tidy_up_and_fail:
      if (saved_errno < 0)
            saved_errno = EVUTIL_SOCKET_ERROR();
      if (listener != -1)
            ::util::close_descriptor(listener);
      if (connector != -1)
            ::util::close_descriptor(connector);
      if (acceptor != -1)
            ::util::close_descriptor(acceptor);

      EVUTIL_SET_SOCKET_ERROR(saved_errno);
      return -1;
#undef ERR
}

} // namespace util


/*--------------------------------------------------------------------------------------*/

#define ASSERT_THROW(expr) ((expr) ? errx(1, "Assertion failed \"%s\"", #expr) : ((void)0))

static void *stupid_wrapper(void *varg)
{
      // auto *handle = static_cast<uvw::TCPHandle *>(varg);

      // handle->write((char *)SLS("Hello there moran\n"));

      pthread_exit(nullptr);
}

NOINLINE void testing05()
{
      // auto loop = uvw::Loop::getDefault();
      // // auto other = uvw::Loop::create();
      // auto listener  = loop->resource<uvw::TCPHandle>();
      // auto connector = loop->resource<uvw::TCPHandle>();
      // auto acceptor = loop->resource<uvw::TCPHandle>();
      // listener->init();
      // connector->init();
      // acceptor->init();
      // listener->bind<uvw::IPv6>("127.0.0.1", 50101U);
      // listener->listen();
      // connector->connect<uvw::IPv4>("127.0.0.1", 50101U);
      // listener->accept(*acceptor);

      ASSERT_THROW(evthread_use_pthreads());

      auto loop = std::make_unique<uv_loop_t>();
      uv_loop_init(loop.get());

      // loop->run<uvw::Loop::Mode::ONCE>();

      // fmt::print(stderr, FC("{} - {} - {}\n"), listener->fd(), connector->fd(), acceptor->fd());

      // std::thread thrd{[&connector](){
      //       connector->read();
      //       // other->run<uvw::Loop::Mode::ONCE>();
      // }};

      // pthread_t tid;
      // pthread_create(&tid, nullptr, stupid_wrapper, connector.get());

      // acceptor->read();
      // pthread_join(tid, nullptr);

      int     fds[2];
      ssize_t ret;
#if 0
      auto *loop  = event_base_new();
      auto *evdns = evdns_base_new(loop, 1);

      ret = evdns_base_nameserver_ip_add(evdns, "127.0.0.1:55000");
      assert(ret == 0);

      struct sockaddr_in addr{};

      ret = evdns_base_get_nameserver_addr(evdns, 0, reinterpret_cast<struct sockaddr *>(&addr), sizeof addr);
      fmt::print(stderr, "Ret is {}\n", ret);
#endif

      ret = util::ersatz_socketpair(AF_INET6, SOCK_STREAM, 0, fds);


#if 0
      assert(ret == 0);
      ret = send(fds[0], SLS("hello"), 0);
      assert(ret > 0);
      ret = recv(fds[1], buf, 64, 0);
      assert(ret > 0);

      fmt::print(stderr, FC("{} - {} - {}\n"), fds[0], fds[1], std::string_view{buf, static_cast<size_t>(ret)});

      close(fds[0]);
      close(fds[1]);
#endif
}




/****************************************************************************************/
} // namespace libevent
} // namespace emlsp
