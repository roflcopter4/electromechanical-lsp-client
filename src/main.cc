#include "Common.hh"
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


/****************************************************************************************/

#include "util/myerr.h"
// NO--LINT
// NO--LINTNEXTLINE(altera-struct-pack-align)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define TRY_FUNC(fn) try_func((fn), #fn)

using int128   = __int128;
using uint128  = unsigned __int128;
using float128 = __float128;

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

#define PRINT(FMT, ...) fmt::print(FMT_COMPILE(FMT) __VA_OPT__(,) __VA_ARGS__)

int
main(UNUSED int argc, UNUSED char *argv[])
{
      try {
#if 0
            TRY_FUNC(emlsp::rpc::json::nloh::test1);
            TRY_FUNC(emlsp::rpc::json::nloh::test2);
            TRY_FUNC(emlsp::rpc::json::rapid::test1);
            TRY_FUNC(emlsp::rpc::json::rapid::test2);
#endif
            //TRY_FUNC(emlsp::rpc::json::test1);
            //TRY_FUNC(emlsp::rpc::json::test2);
            TRY_FUNC(emlsp::rpc::json::test3);
            // TRY_FUNC(emlsp::rpc::event::test1);
      } catch (...) {
      }

      try {
            errno = ENOMEM;
            err(1, "foo. bar. baz. %d", 3);
      } catch (std::exception &e) {
            try {
                  PRINT("Caught exception in {}: {}\n", __PRETTY_FUNCTION__, e.what());
            } catch (...) {}
      }

      fflush(stdout);
      fflush(stderr);

      return 0;
}