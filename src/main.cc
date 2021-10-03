#include "Common.hh"
#include "util/myerr.h"
#include "util/util.hh"

#include <boost/format.hpp>

#include <fmt/format.h>

#include <fmt/color.h>
#include <fmt/compile.h>
#include <fmt/printf.h>

char const *malloc_conf = "stats_print:true,confirm_conf:true,abort_conf:true,prof_leak:true";

#define DEFINITELY_DONT_INLINE(...) \
      extern __attribute__((__noinline__)) __VA_ARGS__ [[noinline]]

#pragma GCC diagnostic ignored "-Wattributes"

/****************************************************************************************/

namespace emlsp::rpc::json {
DEFINITELY_DONT_INLINE(void test1());
DEFINITELY_DONT_INLINE(void test2());
DEFINITELY_DONT_INLINE(void test3());
namespace nloh {
DEFINITELY_DONT_INLINE(void test1());
DEFINITELY_DONT_INLINE(void test2());
} // namespace nloh
namespace rapid {
DEFINITELY_DONT_INLINE(void test1());
DEFINITELY_DONT_INLINE(void test2());
} // namespace rapid
} // namespace emlsp::rpc::json

namespace emlsp::rpc::event {
DEFINITELY_DONT_INLINE(void test1());
} // namespace emlsp::rpc::event

static void init_wsa()
{
      constexpr WORD w_version_requested = MAKEWORD(2, 2);
      WSADATA        wsa_data;

      if (auto const err1 = WSAStartup(w_version_requested, &wsa_data) != 0) {
            std::cerr << "WSAStartup failed with error: " <<  err1 << '\n';
            throw std::runtime_error("Bad winsock dll");
      }

      if (LOBYTE(wsa_data.wVersion) != 2 || HIBYTE(wsa_data.wVersion) != 2) {
            std::cerr << "Could not find a usable version of Winsock.dll\n";
            WSACleanup();
            throw std::runtime_error("Bad winsock dll");
      }
}

/****************************************************************************************/

#define TRY_FUNC(fn) try_func((fn), #fn)
#define PRINT(FMT, ...)  fmt::print(FMT_COMPILE(FMT), ##__VA_ARGS__)
#define FATAL_ERROR(msg) emlsp::util::win32::error_exit(L ## msg)

__attribute__((__nonnull__)) static void
try_func(_Notnull_ void (*fn)(), _Notnull_ char const *const repr) noexcept
{
      try {
            fn();
      } catch (std::exception &e) {
            std::cerr << "Caught exception running function \"" << repr << "\":\n\t"
                      << e.what() << std::endl;
      }
      try {
            std::cout << "\n-------------------------------------------------------------"
                         "-------------------\n\n";
      } catch (...) {
      }
}

int
main(UNUSED int argc, UNUSED char *argv[])
{
      init_wsa();

      try {
#if 0
            TRY_FUNC(emlsp::rpc::json::nloh::test1);
            TRY_FUNC(emlsp::rpc::json::nloh::test2);
            TRY_FUNC(emlsp::rpc::json::rapid::test1);
            TRY_FUNC(emlsp::rpc::json::rapid::test2);
            TRY_FUNC(emlsp::rpc::json::test1);
            TRY_FUNC(emlsp::rpc::json::test2);
            TRY_FUNC(emlsp::rpc::event::test1);
#endif
            TRY_FUNC(emlsp::rpc::json::test3);
      } catch (...) {
      }

      errx(1, "FUCKING MORON IDIOT HEAD! %lld\n", 3);

      //try {
      //      errno = ENOMEM;
      //      err(1, "foo. bar. baz. %d", 3);
      //} catch (std::exception &e) {
      //      try {
      //            PRINT("Caught exception in {}: {}\n", __func__, e.what());
      //      } catch (...) {}
      //}

      fflush(stdout);
      fflush(stderr);

      WSACleanup();
      return 0;
}
