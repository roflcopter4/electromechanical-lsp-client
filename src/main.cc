#include "Common.hh"
#include "util/myerr.h"
#include "util/util.hh"

#include <boost/format.hpp>

#include <fmt/format.h>

#include <fmt/color.h>
#include <fmt/compile.h>
#include <fmt/printf.h>

#include "lsp-protocol/lsp-connection.hh"

//char const *malloc_conf = "stats_print:true,confirm_conf:true,abort_conf:true,prof_leak:true";

#define DEFINITELY_DONT_INLINE(...) \
       [[noinline]] extern __attribute__((noinline)) __VA_ARGS__

#pragma GCC diagnostic ignored "-Wattributes"

/****************************************************************************************/

namespace emlsp::rpc {
namespace json {
DEFINITELY_DONT_INLINE(void test1());
DEFINITELY_DONT_INLINE(void test2());
DEFINITELY_DONT_INLINE(void test3());
namespace nloh  { DEFINITELY_DONT_INLINE(void test1()); }
namespace rapid { DEFINITELY_DONT_INLINE(void test1()); }
} // namespace json

namespace event {
DEFINITELY_DONT_INLINE(void test1());
} // namespace event

namespace test {
DEFINITELY_DONT_INLINE(void test01());
DEFINITELY_DONT_INLINE(void test02());
} // namespace test
} // namespace emlsp::rpc


namespace emlsp::event {
DEFINITELY_DONT_INLINE(void test01());
DEFINITELY_DONT_INLINE(void test02());
} // namespace emlsp::event

#ifdef _WIN32
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
#endif

/****************************************************************************************/

#define TRY_FUNC(fn) try_func((fn), #fn)
#define PRINT(FMT, ...)  fmt::print(FMT_COMPILE(FMT), ##__VA_ARGS__)
#define FATAL_ERROR(msg) emlsp::util::win32::error_exit(L ## msg)

[[maybe_unused]] __attribute__((nonnull))
static void try_func(_Notnull_ void (*fn)(), _Notnull_ char const *repr) noexcept;

static void
try_func(void (*const fn)(), char const *const repr) noexcept
{
      try {
            fn();
            std::cout << '\n';
            std::cout.flush();
      } catch (std::exception &e) {
            std::cerr << "Caught exception running function \"" << repr << "\":\n\t"
                      << e.what() << std::endl;
      }
}

#if 0
template <size_t Nstr>
void templ_test1(UNUSED int unused_, char const *(&argv)[Nstr])
{
      fmt::print(FMT_COMPILE("Got {} strings...\n"), Nstr);
      for (unsigned i = 0; i < Nstr; ++i)
            fmt::print(FMT_COMPILE("String {}: \"{}\"\n"), i, argv[i]);
}

template <typename... Types>
void templ_test3(UNUSED int unused_, Types &&...args)
{
      char   const  *arr[] = {args...};
      size_t const   nelem = sizeof(arr) / sizeof(*arr);

      for (unsigned i = 0; i < nelem; ++i)
            std::cerr << "Have:\t" << arr[i] << '\n';
}
#endif

[[maybe_unused]]
static constexpr
char const initial_msg[] =
R"({
      "jsonrpc": "2.0",
      "id": 1,
      "method": "initialize",
      "params": {
            "trace":     "on",
            "locale":    "en_US.UTF8",
            "clientInfo": {
                  "name": ")" MAIN_PROJECT_NAME R"(",
                  "version": ")" MAIN_PROJECT_VERSION_STRING R"("
            },
            "capabilities": {
            }
      }
})";

struct bigthing {
    public:
      uint64_t volatile foo[8192]{};

      bigthing() = delete;
      ~bigthing() = default;

      template <size_t N>
      explicit bigthing(int const (&in)[N])
      {
            std::copy(in, in + N, foo);
      }

      template <size_t N>
      explicit bigthing(int const(&&in)[N])
      {
            std::copy(in, in + N, foo);
      }

      DELETE_COPY_CTORS(bigthing);
      DEFAULT_MOVE_CTORS(bigthing);
};

#if 0
#include <concepts>
#include <fstream>
#include <functional>
#include <istream>
#include <ostream>
#include <streambuf>
#include <string>
#include <string_view>
#include <strstream>
#endif

int
main(UNUSED int argc, UNUSED char *argv[])
{
#ifdef _WIN32
      init_wsa();
#endif

      {
            UNUSED std::vector foo = {"clangd", "--pretty", "--pch-storage=memory", "--log=verbose"};

            auto client = emlsp::rpc::lsp::unix_socket_connection{};
            client.spawn_connection_l("clangd", "--pretty", "--pch-storage=memory", "--log=verbose");
            client.write_message(initial_msg, sizeof(initial_msg) - 1);

            char  *buf;
            size_t len = client.read_message(buf);
            std::string msg {buf, len};
            std::cout.flush();
            std::cerr.flush();
            std::cout << '\n' << "Read msg of length " << len << '\n' << msg << "\n------ END MSG ------\n\n";
            std::cout.flush();
            delete[] buf;
      }


#ifdef _WIN32
      WSACleanup();
#endif
      return 0;
}
