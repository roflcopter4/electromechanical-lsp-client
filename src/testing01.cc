#include "Common.hh"
#include "ipc/basic_rpc_connection.hh"

#include "ipc/rpc/lsp.hh"
#include "ipc/rpc/lsp/static-data.hh"
#include "ipc/rpc/neovim.hh"

#include "msgpack/dumper.hh"

#include "toploop.hh"

#define AUTOC auto const


inline namespace emlsp {
namespace testing01 {
/****************************************************************************************/


namespace lazy {
#ifdef _WIN32
static constexpr auto fname_raw  = R"(D:\ass\VisualStudio\emlsp-win\src\sigh.cc)"sv;
static constexpr auto fname_uri  = R"(file://D:/ass/VisualStudio/emlsp-win/src/sigh.cc)"sv;
static constexpr auto proj_path  = R"(file://D:/ass/VisualStudio/emlsp-win)"sv;
#else
static constexpr auto fname_raw  = R"(/home/bml/files/projects/emlsp-win/src/sigh.cc)"sv;
static constexpr auto fname_uri  = R"(file://home/bml/files/projects/emlsp-win/src/sigh.cc)"sv;
static constexpr auto proj_path  = R"(file://home/bml/files/projects/emlsp-win)"sv;
#endif
} // namespace lazy


/*--------------------------------------------------------------------------------------*/


static void
dumb_callback_nvim(uv_poll_t *hand, UNUSED int const status, int const events)
{
      auto *conp = static_cast<ipc::rpc::neovim_conection<ipc::connections::inet_socket> *>(hand->data);

      if (events & UV_DISCONNECT) {
            fmt::print(stderr, "Disconnecting!\n");
            conp->notify_all();
      }
      if (events & UV_READABLE) {
            auto rawobj = conp->read_object();
            auto obj = rawobj.get();
            std::cerr << "\n\n" << obj << "\n'n";

#if 0
            fmt::print(stderr,
                        FC("\n\n\033[1;35mRead {} bytes <<_EOF_\n"
                              "\033[0;33m{}\n\033[1;35m_EOF_\033[0m\n\n"),
                        sb.GetSize(), std::string_view{sb.GetString(), sb.GetSize()});
#endif

            conp->notify_one();
      } else {
            fmt::print(stderr, "Bah?\n");
      }
      fflush(stderr);
}


static void
dumb_callback_clangd(uv_poll_t *hand, UNUSED int const status, int const events)
{
      auto *conp = static_cast<ipc::rpc::lsp_conection<ipc::connections::Default> *>(hand->data);

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
      fflush(stderr);
}

static auto idiot()
{
      // std::array<uint64_t, 3> ass = {UINT64_C(~0), UINT64_C(1)<<33, UINT64_C(5)};
      std::array<uint64_t, 3> ass = {UINT64_C(~0), UINT64_C(1)<<33, UINT64_C(5)};
      return ass;
}

NOINLINE void foo01()
{
      // auto loop = loops::main_loop::create();
      // auto nvim = ipc::rpc::neovim_conection<ipc::connections::inet_socket>::new_unique();

      auto x = idiot();
      for (auto const &i : x)
            std::cout << i << '\n';

      fclose(stdin);
      auto nvim = ipc::rpc::neovim_conection<ipc::connections::inet_socket>::new_unique();
      nvim->impl().should_connect(false);
      nvim->impl().resolve("127.0.0.1", "0");
      nvim->impl().open();
      fmt::print(stderr, FC("Have addr '{}' and port {}\n"), nvim->impl().get_addr_string(), nvim->impl().get_port());

      // fmt::print(stderr, FC("Accepted connection with fd {}\n"), nvim->impl().acceptor());
      fmt::print(FC("{}:{}"), nvim->impl().get_addr_string(), nvim->impl().get_port());
      fclose(stdout);

      nvim->impl().accept();
      fmt::print(stderr, FC("Accepted connection with fd {}\n"), nvim->impl().acceptor());
      fflush(stderr);
      // nvim->impl().close();

#ifdef HAVE_PAUSE
      pause();
#else
      for (;;)
            std::this_thread::sleep_for(100s);
#endif


      // nvim->impl().resolve(nvim_addr);
      // nvim->impl().open();
      // nvim->impl().connect();

#if 0
      auto clangd = ipc::rpc::lsp_conection<ipc::connections::Default>::new_unique();
      clangd->spawn_connection_l("clangd", "--pch-storage=memory", "--log=verbose", "--clang-tidy");

      loop->open_poll_handle("nvim", nvim->raw_descriptor(), nvim.get());
      loop->open_poll_handle("clangd", clangd->raw_descriptor(), clangd.get());
      
      loop->start_poll_handle("nvim", dumb_callback_nvim);
      loop->start_poll_handle("clangd", dumb_callback_clangd);

      loop->loop_start();
      pause();
#endif
}


using con_type  = ipc::connections::unix_socket;
using nvim_type = ipc::rpc::neovim_conection<con_type>;
using clang_type = ipc::rpc::lsp_conection<con_type>;


static void
special_start(nvim_type *con)
{
      con->impl().should_close_listnener(false);
      con->impl().should_connect(false);
      con->impl().open();

}


NOINLINE void foo02()
{
      // using con_type = ipc::rpc::neovim_conection<ipc::connections::std_streams>;
      // auto con    = rpc::neovim_conection<connections::unix_socket>::new_unique();
      // auto *bridge = new neovim_callback_bridge(std::move(con));
      // auto bridgep = std::unique_ptr<std::remove_reference_t<decltype(*bridge)>>(bridge);

      //using nvim_type  = ipc::rpc::neovim_conection<ipc::connections::unix_socket>;
      //using clang_type = ipc::rpc::lsp_conection<ipc::connections::unix_socket>;

      auto nvim  = nvim_type::new_unique();
      auto clang = clang_type::new_unique();

      nvim->impl().use_dual_sockets(true);
      clang->impl().use_dual_sockets(true);

#ifdef _WIN32
      nvim->impl().spawn_with_shim(true);
      nvim->impl().should_close_listnener(false);
      nvim->impl().should_connect(false);
      nvim->impl().open();
#endif

      nvim->spawn_connection_l(
          //R"(C:\Program Files (x86)\Microsoft Visual Studio\Shared\Python39_64\python.exe)",
          // R"(python.exe)",
          "python3.10",
          "-c",
          R"(import msgpack, sys; sys.stdout.buffer.write(msgpack.packb([0.1231, 5/3, 10/3, 9/3, {'a': 7}, True, 43, '23', ~0, -1, 18446744073709551615, []]));)");

      // auto rawobj = con->read_object();
      // util::mpack::dumper(rawobj.get());

      // auto nvim_bridge = std::make_unique<ipc::neovim_callback_bridge<nvim_type>>(nvim.release());
      // nvim = nullptr;

      // clang->set_stderr_devnull();
      //clang->spawn_connection_l("clangd", "--log=verbose");
      // auto clang_bridge = std::make_unique<ipc::lsp_callback_bridge<clang_type>>(clang.release());

      auto loop = loops::main_loop::create();
      loop->open_poll_handle("nvim", nvim->raw_descriptor(), nvim.get());
      loop->start_poll_handle("nvim", nvim->poll_callback());

#ifdef __unix__
      {
            struct sigaction act {};
            act.sa_handler = [](int signum) {
                  util::eprint(FC("Got signal {} -> {} here in sigaction handler.\n"),
                               signum, util::get_signal_name(signum));
                  fflush(stderr);
            };
            // sigaction(SIGHUP, &act, nullptr);
            // sigaction(SIGPIPE, &act, nullptr);
            // act.sa_flags = SA_NOCLDWAIT | SA_NOCLDSTOP;
            // sigaction(SIGCHLD, &act, nullptr);
      }
#endif

      //loop->open_poll_handle("clangd", clang->raw_descriptor(), clang.get());
      //loop->start_poll_handle("clangd", clang->poll_callback());
#if 0
      loop->open_poll_handle("clangd", clang->raw_descriptor(), clang.get());
      loop->start_poll_handle("clangd",
                              [](uv_poll_t *hand, UNUSED int status, int event) {
                                    if (event & UV_DISCONNECT) {
                                          ::uv_poll_stop(hand);
                                          return;
                                    }
                                    util::eprint(FC("cb called!\n"));
                                    char       buf[256];
                                    auto const n = ::recv(hand->u.fd, buf, sizeof buf, 0);
                                    assert(n > 0);
                              });
#endif

      //AUTOC init = ipc::lsp::data::init_msg(lazy::proj_path.data());
      //clang->write_string(init);

      //for (size_t i = 0; i < 10; ++i)
      //      nvim->raw_write("Stupidity"sv);

      loop->loop_start();
      // std::this_thread::sleep_for(1s);
      // pause();
}


/****************************************************************************************/
} // namespace testing01
} // namespace emlsp
