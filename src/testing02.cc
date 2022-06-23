// ReSharper disable CppClangTidyCppcoreguidelinesNarrowingConversions
#include "ipc/ipc_connection.hh"
#define __STDC_WANT_IEC_60559_TYPES_EXT__ 1
#include "Common.hh"
#include "ipc/rpc/lsp.hh"
#include "ipc/rpc/lsp/static-data.hh"
#include "ipc/rpc/neovim.hh"

#include "lazy.hh"
#include "toploop.hh"

#if 0
// clang-format off
#include <omp.h>       // NOLINT(llvm-include-order)
#include <omp-tools.h>
#include <ompt.h>
// clang-format on
#endif

//#include <aio.h>
//#include <cfloat>
//#include <ieee754.h>

//#include </usr/lib/gcc/x86_64-pc-linux-gnu/12.1.0/include/quadmath.h>
#include <boost/asio.hpp>
namespace asio = boost::asio;

#define AUTOC auto const

inline namespace emlsp {
namespace testing {
/****************************************************************************************/


namespace myloop {


class executor
{
      asio::basic_system_executor<asio::execution::blocking_t::never_t,
                                  asio::execution::relationship_t::fork_t,
                                  std::allocator<void>>
          exec_;

      executor() = default;

    public:
      virtual ~executor() = default;
      DEFAULT_ALL_CTORS(executor);
      static std::shared_ptr<executor> make_shared()
      {
            return std::shared_ptr<executor>{new executor};
      }
};


} // namespace myloop


/****************************************************************************************/


using connection_type = ipc::protocols::MsJsonrpc::connection<
    ipc::detail::impl::spawner_connection<ipc::detail::unix_socket_connection_impl>>;


NOINLINE void foo05()
{
      auto clangd = std::make_unique<connection_type>();
      clangd->spawn_connection_l("clangd", "--log=verbose", "--pch-storage=memory");

      auto sysexec = asio::basic_system_executor<asio::execution::blocking_t::never_t,
                                                 asio::execution::relationship_t::fork_t,
                                                 std::allocator<void>>{};
      auto sock = std::make_unique<asio::basic_stream_socket<asio::local::stream_protocol>>(sysexec);
      sock->assign(asio::local::stream_protocol{}, static_cast<socket_t>(clangd->raw_descriptor()));

      {
            auto const path = util::recode<char>(lazy::path_raw);
            auto const init = ipc::lsp::data::init_msg(path.c_str());
            clangd->write_string(init);
      }

      auto cond = std::condition_variable{};
      auto mtx  = std::mutex{};
      auto lock = std::unique_lock<std::mutex>(mtx, std::defer_lock_t());
      auto flg  = std::atomic_bool(false);

      auto const start = std::chrono::system_clock::now();
      sysexec.defer(
          [&sock = *sock, &cond, &flg, start]() -> void {
                sock.wait(sock.wait_read);
                auto  avail = sock.available();
                auto  buf   = std::make_unique<char[]>(avail);
                auto  nread = asio::read(sock, asio::buffer(buf.get(), avail));
                util::eprint(FC("Read after {:L} μs\n{}\n"),
                             duration_cast<std::chrono::microseconds>(
                                 std::chrono::system_clock::now() - start).count(),
                             std::string(buf.get(), nread));
                flg.store(true, std::memory_order::seq_cst);
                cond.notify_one();
          },
          std::allocator<void>());

      lock.lock();
      cond.wait(lock, [&flg]() -> bool {
            bool foo = true;
            return flg.compare_exchange_strong(foo, false, std::memory_order::seq_cst,
                                               std::memory_order::acquire);
      });
      assert(!flg);
      util::eprint(FC("Exiting after {:L} μs\n"),
                   duration_cast<std::chrono::microseconds>(
                       std::chrono::system_clock::now() - start).count());
}


/*--------------------------------------------------------------------------------------*/


NOINLINE void foo06()
{
}


/****************************************************************************************/
} // namespace testing
} // namespace emlsp
