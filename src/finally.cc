#include "Common.hh"
#include "ipc/basic_rpc_connection.hh"

#include "ipc/rpc/lsp.hh"
#include "ipc/rpc/lsp/static-data.hh"
#include "ipc/rpc/neovim.hh"

#include "msgpack/dumper.hh"

#define AUTOC auto const


inline namespace emlsp {
namespace finally {
/****************************************************************************************/


namespace loops {

class main_loop
{
      using this_type = main_loop;

      static constexpr int signals_to_handle[] = {
            // SIGINT, SIGTERM,
            SIGHUP, SIGCHLD, SIGPIPE,
      };

      uv_loop_t   base_{};
      uv_timer_t  timer_{};
      uv_signal_t watchers_[std::size(signals_to_handle)]{};

      std::mutex       base_mtx_{};
      std::thread      thrd_;
      std::atomic_bool started_ = false;

      std::map<std::string, uv_poll_t *> handles_ {};

    public:
      static std::unique_ptr<main_loop> create()
      {
            static std::once_flag      flg{};
            std::unique_ptr<main_loop> loop = nullptr;
            std::call_once(flg, create_impl, loop);
            return loop;
      }

    private:
      std::function<void()> retardation_ = [this](){ stop(); };

      main_loop()
      {
            uv_loop_init(&base_);
            uv_timer_init(&base_, &timer_);
            base_.data = &retardation_;
            timer_.data = this;

            // for (unsigned i = 0; i < std::size(signals_to_handle); ++i) {
            //       uv_signal_init(&base_, &watchers_[i]);
            //       uv_signal_start(&watchers_[i], sighandler_wrapper,
            //                       signals_to_handle[i]);
            //       watchers_[i].data = this;
            // }
      }

      static void create_impl(std::unique_ptr<main_loop> &loop)
      {
            loop = std::unique_ptr<main_loop>(new main_loop());
      }

    public:
      ~main_loop()
      {
            if (started_) {
                  // try {
                  //       stop();
                  // } catch (...) {}
                  // thrd_.join();
                  uv_stop(&base_);
                  uv_loop_close(&base_);
            }
            for (auto &pair : handles_) {
                  auto *hand = pair.second;
                  delete hand;
            }
      }

      main_loop(main_loop const &)                = delete;
      main_loop(main_loop &&) noexcept            = delete;
      main_loop &operator=(main_loop const &)     = delete;
      main_loop &operator=(main_loop &&) noexcept = delete;

      //-------------------------------------------------------------------------------

      uv_loop_t *base() { return &base_; }

      uv_poll_t *open_poll_handle(std::string name, int const fd, void *data)
      {
            std::lock_guard lock(base_mtx_);
            auto *hand = new uv_poll_t;
#ifdef _WIN32
            uv_poll_init(&base_, hand, fd);
#else
            uv_poll_init_socket(&base_, hand, fd);
#endif
            auto ret = handles_.insert(std::pair{std::move(name), hand});
            assert(ret.second != false);

            hand->data = data;
            return hand;
      }


      void start_timer_handle(
          uv_timer_cb const cb      = [](uv_timer_t *) {},
          uint64_t const    timeout = UINT64_C(1))
      {
            uv_timer_start(&timer_, cb, timeout, timeout);
      }

      void start_poll_handle(std::string const &key, uv_poll_cb const callback)
      {
            std::lock_guard lock(base_mtx_);
            auto *hand = handles_[key];
            uv_poll_start(hand, UV_READABLE | UV_DISCONNECT, callback);
      }

      void start_async()
      {
            std::lock_guard lock(base_mtx_);
            if (started_.load())
                  throw std::logic_error("Attempt to start loop twice!");

            thrd_ = std::thread(
                [](uv_loop_t *base) {
                      uv_run(base, UV_RUN_DEFAULT);
                }, &base_
            );

            started_.store(true);
      }

      void start()
      {
            {
                  std::lock_guard lock(base_mtx_);
                  if (started_.load())
                        throw std::logic_error("Attempt to start loop twice!");
                  started_.store(true);
            }
            uv_run(&base_, UV_RUN_DEFAULT);
      }

      void stop()
      {
            if (!started_.load())
                  throw std::logic_error("Can't stop unstarted loop!");
            fmt::print(stderr, "Stop called, stopping.\n");
            fflush(stderr);

            // for (auto &hand : watchers_)
            //       uv_signal_stop(&hand);
            for (auto const &it : handles_)
                  if (it.second)
                        uv_poll_stop(it.second);

            uv_timer_stop(&timer_);

            // uv_stop(&base_);
            // uv_stop(&base_);

            started_.store(false);
      }

#ifdef _WIN32
      uv_poll_t *open_poll_handle(std::string name, SOCKET const fd, void *data)
      {
            std::lock_guard lock(base_mtx_);
            auto *hand = new uv_poll_t;
            uv_poll_init_socket(&base_, hand, fd);
            handles_.emplace(std::move(name), hand);

            hand->data = data;
            return hand;
      }
#endif

      //-------------------------------------------------------------------------------

    private:
      static void sighandler_wrapper(uv_signal_t *handle, int signum)
      {
            auto *self = static_cast<this_type *>(handle->data);
            self->sighandler(handle, signum);
      }

      void sighandler(UNUSED uv_signal_t *handle, int signum)
      {
            util::eprint(FC("Have signal {} aka SIG{}  ->  {}\n"),
                         signum,
                         // util::get_signal_name(signum),
                         // util::get_signal_explanation(signum)
                         ::sigabbrev_np(signum),
                         ::sigdescr_np(signum)
                         );

            switch (signum)
            {
            case SIGHUP:
            case SIGPIPE:
            case SIGCHLD:
                  stop();
                  break;
            case SIGINT:
            case SIGTERM:
                  ::exit(80);
            default:
                  throw std::logic_error(
                      fmt::format(FC("Somehow received unhandled signal number {} ({})"),
                                  signum, util::get_signal_name(signum)));
            }
      }
};

} // namespace loops


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
      pause();


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

      loop->start();
      pause();
#endif
}


NOINLINE void foo02()
{
      // using con_type = ipc::rpc::neovim_conection<ipc::connections::std_streams>;
      using con_type = ipc::rpc::neovim_conection<ipc::connections::inet_socket>;

      // auto con    = rpc::neovim_conection<connections::unix_socket>::new_unique();
      // auto *bridge = new neovim_callback_bridge(std::move(con));
      // auto bridgep = std::unique_ptr<std::remove_reference_t<decltype(*bridge)>>(bridge);
      auto con  = con_type::new_unique();

      con->spawn_connection_l(
          "python", "-c",
          R"(import msgpack, sys; sys.stdout.buffer.write(msgpack.packb([0.1231, 5/3, 10/3, 9/3, {'a': 7}, True, 43, '23', ~0, -1, 18446744073709551615, []])))");

      // auto rawobj = con->read_object();
      // util::mpack::dumper(rawobj.get());

      auto reet = std::make_unique<ipc::neovim_callback_bridge<con_type>>(con.release());
      con = nullptr;

      auto loop = loops::main_loop::create();
      loop->open_poll_handle("nvim", reet->con()->raw_descriptor(), reet->con());
      loop->start_poll_handle("nvim", reet->con()->poll_callback());
      loop->start_timer_handle(reet->con()->timer_callback());


      {
            struct sigaction act {};
            act.sa_handler = [](int signum) {
                  fmt::print(stderr, FC("Got signal {} -> {} here in sigaction handler.\n"), signum, util::get_signal_name(signum));
                  fflush(stderr);
            };
            // sigaction(SIGHUP, &act, nullptr);
            // sigaction(SIGPIPE, &act, nullptr);
            // act.sa_flags = SA_NOCLDWAIT | SA_NOCLDSTOP;
            // sigaction(SIGCHLD, &act, nullptr);
      }

      loop->start();
}


/****************************************************************************************/
} // namespace finally
} // namespace emlsp
