#include "Common.hh"
#include "ipc/lsp/lsp-client.hh"
#include "ipc/lsp/lsp-connection.hh"
#include "ipc/lsp/static-data.hh"
#include "ipc/rpc/basic_wrapper.hh"
#include "ipc/rpc/basic_io_connection.hh"
#include "ipc/totality.hh"

#include <boost/asio.hpp>
#include <uv.h>

#define AUTOC auto const

inline namespace emlsp {
namespace sigh {
/****************************************************************************************/

namespace asio = boost::asio;

using con_type = ipc::rpc::basic_io_connection<ipc::connections::inet_ipv4_socket, ipc::rpc::ms_jsonrpc_io_wrapper>;

static constexpr auto timeout_len = UINT64_C(1000);
static constexpr auto fname_raw   = R"(D:\ass\VisualStudio\emlsp\src\sigh.cc)"sv;
static constexpr auto fname_uri   = R"(file:/D:/ass/VisualStudio/emlsp/src/sigh.cc)"sv;
static constexpr auto fname_path  = R"(file:/D:/ass/VisualStudio/emlsp)"sv;


UNUSED static void
dump_read(std::string_view const &str)
{
      fmt::print(stderr,
                 FC("\n\n\033[1;35mRead {} bytes <<_EOF_\n"
                    "\033[0;33m{}\n\033[1;35m_EOF_\033[0m\n\n"),
                 str.size(), str);
      fflush(stderr);
}

UNUSED static void
dump_read(void const *const buf, size_t const len)
{
      fmt::print(stderr,
                 FC("\n\n\033[1;35mRead {} bytes <<_EOF_\n"
                    "\033[0;33m{}\n\033[1;35m_EOF_\033[0m\n\n"),
                 len, std::string_view{static_cast<char const *>(buf), len});
      fflush(stderr);
}

UNUSED static void
dump_read(rapidjson::Document &obj)
{
      rapidjson::MemoryBuffer mbuf;
      rapidjson::Writer wr{mbuf};
      obj.Accept(wr);

      fmt::print(stderr,
                 FC("\n\n\033[1;35mRead {} bytes <<_EOF_\n"
                    "\033[0;33m{}\n\033[1;35m_EOF_\033[0m\n\n"),
                 mbuf.GetSize(), std::string_view{mbuf.GetBuffer(), mbuf.GetSize()});
      fflush(stderr);
}


static void
poll_throw(WSAPOLLFD *data, uint16_t events = 0, ULONG nobjs = 1, INT time = (-1))
{
      if (events != 0)
            data[0].events = events;
      data[0].revents = 0;

      auto ret = WSAPoll(data, nobjs, time);
      if (ret == SOCKET_ERROR)
            util::win32::error_exit_wsa(L"WSAPoll()");
}


NOINLINE void sigh01()
{
      auto con = std::make_unique<con_type>();
      con->impl().open();
      //WSAPOLLFD wsa_data[] = {
          //{.fd = con->raw_descriptor(), .events = POLLRDNORM, .revents = 0}
      //};

      con->spawn_connection_l(
                              //R"(D:\Program Files\Microsoft Visual Studio\2022\Preview\VC\Tools\Llvm\x64\bin\clangd.exe)",
                              R"(D:\ass\GIT\llvm-project\llvm\turd\bin\clangd.exe)",
                              "--pch-storage=memory", "--log=verbose", "--clang-tidy");

      AUTOC init = ipc::lsp::data::init_msg(fname_path.data());
      con->write_string(init);

      //poll_throw(wsa_data);
      //fmt::print(FC("revents is 0x{:X}\n"), wsa_data[0].revents);

      auto obj = con->read_object();
      dump_read(obj);

      {
            AUTOC contentvec = ::util::slurp_file(fname_raw);
            auto wrap = con->get_packer();
            wrap().add_member("jsonrpc", "2.0");
            wrap().add_member("method", "textDocument/didOpen");
            wrap().set_member("params");
            wrap().set_member("textDocument");
            wrap().add_member("uri",  rapidjson::Value ( fname_uri.data(),  static_cast<rapidjson::SizeType>(fname_uri.size()) ) );
            wrap().add_member("text", rapidjson::Value { contentvec.data(), static_cast<rapidjson::SizeType>(contentvec.size()) } );
            wrap().add_member("version", 1);
            wrap().add_member("languageId", "cpp");
            //poll_throw(wsa_data, POLLWRNORM, 1, INFINITE);
            con->write_object(wrap);
      }

      //poll_throw(wsa_data, POLLRDNORM);
      obj = con->read_object();
      dump_read(obj);
}


NOINLINE void sigh05()
{
      //using Socket = asio::basic_stream_socket<asio::local::stream_protocol>;
      using Socket = asio::basic_stream_socket<asio::ip::tcp>;

      static constexpr uint64_t timeout_len = 1000;
      std::mutex mtx1;

      auto con = std::make_unique<con_type>();
      con->impl().open();
      //con->spawn_connection_l("clangd", "--log=verbose");
      //con->spawn_connection_l("rls");
      con->spawn_connection_l(
                              R"(D:\ass\GIT\llvm-project\llvm\gcc\bin\clangd.exe)",
                              //"--sync",
                              "--log=verbose",
                              "--pch-storage=memory", "--clang-tidy", "--background-index"
      );

#if 0
      fmt::print(FC("With sockets {} and {}. Also '{}'.\n"),
                 con->impl().fd(), con->impl().peer(),
                 util::demangle(typeid(std::remove_reference_t<decltype(*con)>)));

      auto write_thread = std::thread{[&cond1, &mtx1](socket_t const sock) {
            auto lock = std::unique_lock(mtx1);
            send(sock, SLS("Hello, idiot mcfuckerface!\n"), 0);
            cond1.wait(lock);
            std::this_thread::sleep_for(1s);
            send(sock, SLS("Hello, idiot mcfuckerface!\n"), 0);
            cond1.wait(lock);
      }, con->impl().peer()};

      struct my_uv_data {
            uv_loop_t  loop{};
            uv_poll_t  poll_hand{};
            uv_timer_t timer_hand{};
      };

      auto const data = std::make_unique<my_uv_data>();
      fmt::print(stderr,
                 FC("The bloated ass structure is size {}, alignment {}. The last field "
                    "(1024 bytes in, theoretically?) is at offset {}.\n"),
                 sizeof(my_uv_data), alignof(my_uv_data), __builtin_offsetof(my_uv_data, timer_hand));
#endif

#if 0
      rapidjson::MemoryBuffer membuf;
      auto iox  = asio::io_context{};
      auto sock = Socket{iox};
      sock.assign(asio::ip::tcp::v4(), con->raw_descriptor());

      std::remove_cvref_t<decltype(con->get_packer())> *to_write;

      //auto rdfn = [&](boost::system::error_code const &) {
      auto rdfn = [&sock, &mtx1]() {
            std::lock_guard lock{mtx1};
            char       rawbuf[65536];
            auto const buffer = asio::mutable_buffer(rawbuf, sizeof rawbuf);
            auto const nread  = sock.read_some(buffer);

          fmt::print(stderr, FC("\n\n\033[1;35mRead {} bytes <<_EOF_\n\033[0;33m{}\n\033[1;35m_EOF_\033[0m\n\n"),
                     nread, std::string_view{rawbuf, nread});
      };

      auto read_thread = std::thread{[&con, &iox, &sock, rdfn]() {
            try {
                  for (;;) {
                        sock.wait(Socket::wait_read);
                        auto strand = asio::make_strand(iox);
                        strand.dispatch(rdfn, std::allocator<void>());
                        iox.run();
                        //con->notify_one();
                  }
            } catch (boost::system::system_error &e) {
                  if (e.code().value() != WSAEINTR) {
                        fmt::print(stderr, "Unexpected exception {}\n", e.what());
                        throw;
                  }
            }
      }};

      //con->wait();

      //sock.async_wait(Socket::wait_read, rdfn);

      auto wrfn = [&con, &membuf, &mtx1](boost::system::error_code const &) {
            std::lock_guard lock{mtx1};
            con->write_string(membuf.GetBuffer(), membuf.GetSize());
      };
#endif

#if 0
      auto watch_thread = std::thread{[&]() {
            sock.wait(Socket::wait_error);
            auto foo = asio::make_strand(iox);
            
            sock.async_wait(Socket::wait_read, [](boost::system::error_code const &c){});
      }};
#endif

      auto read_thread = std::thread{[&con]() {
            for (;;) {
                  auto doc = con->read_object();
                  rapidjson::StringBuffer sb;
                  rapidjson::Writer       wr{sb};
                  doc.Accept(wr);

                  fmt::print(stderr,
                             FC("\n\n\033[1;35mRead {} bytes <<_EOF_\n"
                                "\033[0;33m{}\n\033[1;35m_EOF_\033[0m\n\n"),
                             sb.GetSize(),
                             std::string_view{sb.GetString(), sb.GetSize()});
                  con->notify_one();
            }
      }};

      AUTOC init = ipc::lsp::data::init_msg(fname_path.data());
      con->write_string(init);
      con->wait();

      //write_thread.join();
      {
            AUTOC contentvec = ::util::slurp_file(fname_raw);
            auto wrap = con->get_packer();
            wrap().add_member("jsonrpc", "2.0");
            wrap().add_member("method", "textDocument/didOpen");
            wrap().set_member("params");
            wrap().set_member("textDocument");
            wrap().add_member("uri",  rapidjson::Value ( fname_uri.data(),  static_cast<rapidjson::SizeType>(fname_uri.size()) ) );
            wrap().add_member("text", rapidjson::Value { contentvec.data(), static_cast<rapidjson::SizeType>(contentvec.size()) } );
            wrap().add_member("version", 1);
            wrap().add_member("languageId", "cpp");

            //rapidjson::MemoryBuffer membuf;
            //rapidjson::Writer wr{membuf};
            //wrap().doc().Accept(wr);
            con->write_object(wrap);
            //con->wait();
            //sock.async_wait(Socket::wait_write, wrfn);
            //con->wait();
      }
      {
            auto wrap = con->get_packer();
            wrap().add_member("jsonrpc", "2.0");
            wrap().add_member("id", 1);
            wrap().add_member("method", "textDocument/semanticTokens/full");
            wrap().set_member("params");
            wrap().set_member("textDocument");
            wrap().add_member("uri", rapidjson::Value(fname_uri.data(), fname_uri.size()));
            con->write_object(wrap);
            con->wait();
      }



      std::this_thread::sleep_for(10000ms);
      con->impl().close();
      //iox.stop();
      read_thread.join();

#if 0
      uv_loop_init(&data->loop);
      uv_poll_init_socket(&data->loop, &data->poll_hand, con->impl().fd());
      uv_timer_init(&data->loop, &data->timer_hand);

      uv_timer_start(
          &data->timer_hand,
          [](uv_timer_t *) { /*NOP*/ },
          timeout_len, timeout_len
      );

      data->poll_hand.data = &cond1;

      uv_poll_start(
          &data->poll_hand, UV_READABLE | UV_DISCONNECT,
          [](uv_poll_t *hand, int const status, int const events) -> void
          {
                auto *cnd = static_cast<std::condition_variable *>(hand->data);
                fmt::print(FC("{} - {}\n"), status, events);
                if (events & UV_DISCONNECT) {
                      fmt::print(stderr, "Disconnecting!\n");
                }
                if (events & UV_READABLE) {
                      fmt::print(stderr, "It mostly worked maybe.\n");
                      char       buf[1024];
                      auto const len = recv(hand->socket, buf, 1024, 0);
                      if (len <= 0)
                            util::win32::error_exit_wsa(L"recv()");
                      fmt::print(stderr, FC("Got \"{}\"\n"), std::string_view{buf, static_cast<size_t>(len)});
                } else {
                      fmt::print(stderr, "Bah?\n");
                      uv_poll_stop(hand);
                }
                cnd->notify_one();
          });

      auto loop_thread = std::thread{uv_run, &data->loop, UV_RUN_DEFAULT};

      //std::this_thread::sleep_for(3s);
      write_thread.join();

      uv_poll_stop(&data->poll_hand);
      uv_timer_stop(&data->timer_hand);
      uv_stop(&data->loop);
      //con->impl().close();
      loop_thread.join();
      //Sleep(2000);
#endif
}



NOINLINE void sigh02()
{
      static constexpr uint64_t timeout_len = 1000;
      std::mutex mtx1;
      std::condition_variable cond1;

      auto con = std::make_unique<ipc::connections::unix_socket>();
      con->impl().open();

      fmt::print(FC("With sockets {} and {}. Also '{}'.\n"),
                 con->impl().fd(), con->impl().peer(),
                 util::demangle(typeid(std::remove_reference_t<decltype(*con)>)));

      auto write_thread = std::thread{[&cond1, &mtx1](socket_t const sock) {
            auto lock = std::unique_lock(mtx1);
            send(sock, SLS("Hello, idiot mcfuckerface!\n"), 0);
            cond1.wait(lock);
            std::this_thread::sleep_for(1s);
            send(sock, SLS("Hello, idiot mcfuckerface!\n"), 0);
            cond1.wait(lock);
      }, con->impl().peer()};

      struct my_uv_data {
            uv_loop_t  loop{};
            uv_poll_t  poll_hand{};
            uv_timer_t timer_hand{};
      };

      auto const data = std::make_unique<my_uv_data>();
      fmt::print(stderr,
                 FC("The bloated ass structure is size {}, alignment {}. The last field "
                    "(1024 bytes in, theoretically?) is at offset {}.\n"),
                 sizeof(my_uv_data), alignof(my_uv_data), __builtin_offsetof(my_uv_data, timer_hand));

      uv_loop_init(&data->loop);
      uv_poll_init_socket(&data->loop, &data->poll_hand, con->impl().fd());
      uv_timer_init(&data->loop, &data->timer_hand);

      uv_timer_start(
          &data->timer_hand,
          [](uv_timer_t *) { /*NOP*/ },
          timeout_len, timeout_len
      );

      data->poll_hand.data = &cond1;

      uv_poll_start(
          &data->poll_hand, UV_READABLE | UV_DISCONNECT,
          [](uv_poll_t *hand, int const status, int const events) -> void
          {
                auto *cnd = static_cast<std::condition_variable *>(hand->data);
                fmt::print(FC("{} - {}\n"), status, events);
                if (events & UV_DISCONNECT) {
                      fmt::print(stderr, "Disconnecting!\n");
                }
                if (events & UV_READABLE) {
                      fmt::print(stderr, "It mostly worked maybe.\n");
                      char       buf[1024];
                      auto const len = recv(hand->socket, buf, 1024, 0);
                      if (len <= 0)
                            util::win32::error_exit_wsa(L"recv()");
                      fmt::print(stderr, FC("Got \"{}\"\n"), std::string_view{buf, static_cast<size_t>(len)});
                } else {
                      fmt::print(stderr, "Bah?\n");
                      uv_poll_stop(hand);
                }
                cnd->notify_one();
          });

      auto loop_thread = std::thread{uv_run, &data->loop, UV_RUN_DEFAULT};

      //std::this_thread::sleep_for(3s);
      write_thread.join();

      uv_poll_stop(&data->poll_hand);
      uv_timer_stop(&data->timer_hand);
      uv_stop(&data->loop);
      //con->impl().close();
      loop_thread.join();
      //Sleep(2000);
}


NOINLINE void sigh03()
{
      auto const con = std::make_unique<ipc::connections::unix_socket>();
      //auto const con = std::make_unique<ipc::connections::pipe>();
      //auto const con = std::make_unique<ipc::connections::win32_named_pipe>();
      //auto const con = std::make_unique<ipc::connections::inet_ipv6_socket>();

      con->spawn_connection_l(R"(D:\ass\VisualStudio\Repos\DumpWSAStuff\x64\Debug\DumpWSAStuff.exe)");

      char buf[8192];
      auto count = con->raw_read(buf, sizeof buf, 0);
      assert(count > 0);
      buf[count] = '\0';
      fmt::print(FC("S: Read {} bytes:\t\"{}\"\n"), count, std::string_view(buf, count));
      fflush(stderr);

      count = con->raw_write(SLS("Goodbye, idiot mcmoronface."));
      assert(count > 0);
      fmt::print(FC("S: Wrote {} bytes...\n"), count);

      DWORD code = 0;
      WaitForSingleObject(con->pid().hProcess, INFINITE);
      GetExitCodeProcess(con->pid().hProcess, &code);
      fmt::print(FC("S: Process {} has ended with raw status {}.\n"), con->pid().dwProcessId, code);
}


NOINLINE void sigh04()
{
      putenv("RUST_BACKTRACE=full");

      //auto const path = util::get_temporary_filename(nullptr, ".sock").string();
      auto con = std::make_unique<con_type>();
      con->impl().should_close_listnener() = false;
      con->impl().should_open_listener() = false;
      con->impl().should_connect() = false;
      con->impl().addr().sin_port = htons(62010);
      con->impl().open();

      //std::string const listen_str = fmt::format("--listen=unix:{}", con->impl().path());
      //std::string const listen_str = fmt::format("--listen=[::1]:{}", ntohs(con->impl().addr().sin6_port));
      std::string const listen_str = fmt::format("--listen=127.0.0.1:{}", ntohs(con->impl().addr().sin_port));
      char const *const args[]     = {
          R"(D:\ass\GIT\llvm-project\llvm\gcc\bin\clangd.exe)",
          "--log=verbose",
          "--debug",
          "--pch-storage=memory",
          "--clang-tidy",
          listen_str.c_str()};

      con->spawn_connection(std::size(args), const_cast<char **>(args));
      std::this_thread::sleep_for(1s);

#if 0
      sockaddr_un addr{};
      addr.sun_family = AF_UNIX;
      memcpy(addr.sun_path, path.data(), path.size() + 1);
      socket_t sock = socket(AF_UNIX, SOCK_STREAM, 0);
      auto ret = connect(sock, reinterpret_cast<sockaddr *>(&addr), sizeof(sockaddr_un));
      if (ret == -1)
            util::win32::error_exit_wsa(L"connect()");
#endif

      con->impl().connect();

      //con->impl().

      //con->spawn_connection_l(//R"(D:\Program Files\Microsoft Visual Studio\2022\Preview\VC\Tools\Llvm\x64\bin\clangd.exe)",
      //                        //R"(D:\ass\GIT\llvm-project\llvm\turd\bin\clangd.exe)",
      //                        //"gopls", "-vv", "-logfile=foo.txt", "-rpc.trace"
      //                        //"--sync",
      //                        R"(D:\ass\GIT\llvm-project\llvm\gcc\bin\clangd.exe)",
      //                        "--log=verbose", "--debug",
      //                        "--pch-storage=memory", "--clang-tidy",
      //);

      struct my_uv_data {
            uv_loop_t  loop{};
            uv_poll_t  poll_hand{};
            uv_timer_t timer_hand{};
      };

      auto const data = std::make_unique<my_uv_data>();

      uv_loop_init(&data->loop);
      uv_poll_init_socket(&data->loop, &data->poll_hand, con->raw_descriptor());
      //uv_poll_init_socket(&data->loop, &data->poll_hand, con->raw_descriptor());
      //uv_poll_init_socket(&data->loop, &data->poll_hand, (SOCKET)con->raw_descriptor());
      //int fd = _open_osfhandle((intptr_t)con->raw_descriptor(), _O_RDWR | _O_BINARY | _O_APPEND);
      //uv_poll_init(&data->loop, &data->poll_hand, fd);
      uv_timer_init(&data->loop, &data->timer_hand);

      uv_timer_start(
          &data->timer_hand,
          [](uv_timer_t *) { /*NOP*/ },
          timeout_len, timeout_len
      );

      data->poll_hand.data = con.get();

      uv_poll_start(
          &data->poll_hand, UV_READABLE | UV_DISCONNECT,
          [](uv_poll_t *hand, int const status, int const events) -> void
          {
                auto *conp = static_cast<con_type *>(hand->data);

                //fmt::print(FC("{} - {}\n"), status, events);
                if (events & UV_DISCONNECT) {
                      fmt::print(stderr, "Disconnecting!\n");
                      conp->notify_all();
                }
                if (events & UV_READABLE) {
                      auto doc = conp->read_object();
                      rapidjson::StringBuffer sb;
                      rapidjson::Writer       wr{sb};
                      doc.Accept(wr);

                      fmt::print(stderr,
                                 FC("\n\n\033[1;35mRead {} bytes <<_EOF_\n"
                                    "\033[0;33m{}\n\033[1;35m_EOF_\033[0m\n\n"),
                                 sb.GetSize(), std::string_view{sb.GetString(), sb.GetSize()});
                      conp->notify_one();
                } else {
                      fmt::print(stderr, "Bah?\n");
                }
          });

      auto loop_thread = std::thread{uv_run, &data->loop, UV_RUN_DEFAULT};

      {
            AUTOC init = ipc::lsp::data::init_msg(fname_path.data());
            con->write_string(init);
#if 0
            auto wrap = con->get_packer();
            wrap().add_member("id", 0);
            wrap().add_member("jsonrpc", "2.0");
            wrap().add_member("method", "initialize");
            wrap().set_member("params");
            wrap().add_member("processId", getpid());
            wrap().add_member("rootUri", rapidjson::Value{fname_path.data(), fname_path.size()});
            wrap().add_member("locale", "en_US.UTF8");
            wrap().set_member("capabilities");
            con->write_object(wrap);
#endif
            con->wait();
      }
      {
            AUTOC contentvec = ::util::slurp_file(fname_raw);
            auto wrap = con->get_packer();
            wrap().add_member("jsonrpc", "2.0");
            wrap().add_member("method", "textDocument/didOpen");
            wrap().set_member("params");
            wrap().set_member("textDocument");
            wrap().add_member("uri",  rapidjson::Value ( fname_uri.data(),  static_cast<rapidjson::SizeType>(fname_uri.size()) ) );
            wrap().add_member("text", rapidjson::Value { contentvec.data(), static_cast<rapidjson::SizeType>(contentvec.size()) } );
            wrap().add_member("version", 1);
            wrap().add_member("languageId", "cpp");
            con->write_object(wrap);
            con->wait();
      }
      {
            auto wrap = con->get_packer();
            wrap().add_member("jsonrpc", "2.0");
            wrap().add_member("id", 1);
            wrap().add_member("method", "textDocument/semanticTokens/full");
            wrap().set_member("params");
            wrap().set_member("textDocument");
            wrap().add_member("uri", rapidjson::Value(fname_uri.data(), fname_uri.size()));
            con->write_object(wrap);
            con->wait();
      }


      fflush(stderr);
      loop_thread.join();
}


static void
my_alloc_cb(uv_handle_t *handle, size_t suggested, uv_buf_t *buf)
{
      buf->base = static_cast<char *>(malloc(suggested));
      buf->len  = suggested;
}


static void
my_read_cb(uv_stream_t *uvstream, ssize_t cnt, uv_buf_t const *buf)
{
      if (cnt <= 0) {
            // cnt == 0 means libuv asked for a buffer and decided it wasn't needed:
            // http://docs.libuv.org/en/latest/stream.html#c.uv_read_start.
            //
            // We don't need to do anything with the RBuffer because the next call
            // to `alloc_cb` will return the same unused pointer(`rbuffer_produced`
            // won't be called)
            if (cnt == UV_ENOBUFS || cnt == 0) {

            } else if (cnt == UV_EOF && uvstream->type == UV_TTY) {
                  // The TTY driver might signal EOF without closing the stream
                  //invoke_read_cb(stream, 0, true);
            } else {
                  //DLOG("closing Stream (%p): %s (%s)", (void *)stream, uv_err_name((int)cnt), os_strerror((int)cnt));
                  // Read error or EOF, either way stop the stream and invoke the callback
                  // with eof == true
                  uv_read_stop(uvstream);
                  //invoke_read_cb(stream, 0, true);
            }

            return;
      }

      dump_read(buf->base, cnt);
}


NOINLINE void sigh06()
{
      struct my_uv_data {
            uv_loop_t    loop{};
            uv_pipe_t    rdpipe{};
            uv_pipe_t    wrpipe{};
            uv_process_t proc{};
      };

      auto data = std::make_unique<my_uv_data>();
      uv_loop_init(&data->loop);
      uv_pipe_init(&data->loop, &data->rdpipe, 0);
      uv_pipe_init(&data->loop, &data->wrpipe, 0);

      auto argv = std::array{"clangd.exe", "--pch-storage=memory", "--log=verbose", "--clang-tidy", "--background-index", static_cast<char const *>(0)};
      uv_stdio_container_t cont[] = {
            //{.flags = static_cast<uv_stdio_flags>(UV_CREATE_PIPE | UV_READABLE_PIPE | UV_WRITABLE_PIPE | UV_OVERLAPPED_PIPE),
            // .data = { .fd = 0, }},
            //{.flags = static_cast<uv_stdio_flags>(UV_CREATE_PIPE | UV_READABLE_PIPE | UV_WRITABLE_PIPE | UV_OVERLAPPED_PIPE),
            // .data = { .fd = 1, }},
            {.flags = static_cast<uv_stdio_flags>(UV_CREATE_PIPE | UV_READABLE_PIPE | UV_OVERLAPPED_PIPE),
             .data = { .stream = reinterpret_cast<uv_stream_t *>(&data->wrpipe), }},
            {.flags = static_cast<uv_stdio_flags>(UV_CREATE_PIPE | UV_WRITABLE_PIPE | UV_READABLE_PIPE | UV_OVERLAPPED_PIPE),
             .data = { .stream = reinterpret_cast<uv_stream_t *>(&data->rdpipe), }},
            {.flags = static_cast<uv_stdio_flags>(UV_INHERIT_FD), .data = { .fd = 2, }},
      };
      uv_process_options_t opts = {
          .file = "clangd.exe",
          .args = const_cast<char **>(argv.data()),
          .flags = UV_PROCESS_WINDOWS_HIDE,
          .stdio_count = std::size(cont),
          .stdio = cont,
      };

      if (int const status = uv_spawn(&data->loop, &data->proc, &opts))
            errx_nothrow(1, "uv_spawn() failed: %s", uv_strerror(status));

      fmt::print("I think it workded?  ->  {}\n", data->proc.pid);

      //auto con = std::make_unique <con_type>();
      //con->impl().

      {
            AUTOC init = ipc::lsp::data::init_msg(fname_path.data());
            char  buf[60];
            AUTOC buflen = ::sprintf(buf, "Content-Length: %zu\r\n\r\n", init.size());

            WriteFile(data->wrpipe.handle, buf, buflen, nullptr, nullptr);
            WriteFile(data->wrpipe.handle, init.data(), init.size(), nullptr, nullptr);

            //uv_buf_t ubuf = {static_cast<ULONG>(buflen), buf};
            //uv_try_write(reinterpret_cast<uv_stream_t *>(&data->pipe), &ubuf, 1);
            //ubuf.base = const_cast<char *>(init.data());
            //ubuf.len  = init.size();
            //uv_try_write(reinterpret_cast<uv_stream_t *>(&data->pipe), &ubuf, 1);
      }

      uv_read_start(reinterpret_cast<uv_stream_t *>(&data->rdpipe), my_alloc_cb, my_read_cb);
      auto loop_thread = std::thread{uv_run, &data->loop, UV_RUN_DEFAULT};

#if 0
      {
            AUTOC contentvec = ::util::slurp_file(fname_raw);
            auto wrap = con->get_packer();
            wrap().add_member("jsonrpc", "2.0");
            wrap().add_member("method", "textDocument/didOpen");
            wrap().set_member("params");
            wrap().set_member("textDocument");
            wrap().add_member("uri",  rapidjson::Value ( fname_uri.data(),  static_cast<rapidjson::SizeType>(fname_uri.size()) ) );
            wrap().add_member("text", rapidjson::Value { contentvec.data(), static_cast<rapidjson::SizeType>(contentvec.size()) } );
            wrap().add_member("version", 1);
            wrap().add_member("languageId", "cpp");
            con->write_object(wrap);
            con->wait();
      }

      {
            OVERLAPPED ov;
            SecureZeroMemory(&ov, sizeof ov);
            ov.hEvent = CreateEvent(nullptr, true, false, nullptr);

            DWORD n = 0;
            char  buf[8192];
            bool  b = ReadFile(data->rdpipe.handle, buf, sizeof buf, &n, &ov);

            if (!b)
                  warnx("%s\n", uv_strerror(GetLastError()));

            DWORD trans = 0;
            b = GetOverlappedResult(data->rdpipe.handle, &ov, &trans, true);
            if (!b || trans <= 0)
                  util::win32::error_exit(L"GetOverlappedResult()");

            dump_read(buf, trans);
            Sleep(50000);
      }
#endif
}


/****************************************************************************************/
} // namespace sigh
} // namespace emlsp
