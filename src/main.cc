#include "Common.hh"
#include "util/myerr.h"
#include "util/util.hh"

#include <boost/format.hpp>

#include <fmt/format.h>

#include <fmt/color.h>
#include <fmt/compile.h>
#include <fmt/printf.h>

#include "lsp-protocol/lsp-protocol.hh"

char const *malloc_conf = "stats_print:true,confirm_conf:true,abort_conf:true,prof_leak:true";

#define DEFINITELY_DONT_INLINE(...) \
      extern __attribute__((__noinline__)) __VA_ARGS__ [[noinline]]

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

[[maybe_unused]] __attribute__((__nonnull__)) static void
try_func(_Notnull_ void (*fn)(), _Notnull_ char const *repr) noexcept;

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

template <size_t Nstr>
void templ_test1(UNUSED int unused_, char const *(&&argv)[Nstr])
{
      fmt::print(FMT_COMPILE("Got {} strings...\n"), Nstr);
      for (unsigned i = 0; i < Nstr; ++i)
            fmt::print(FMT_COMPILE("String {}: \"{}\"\n"), i, argv[i]);
}

template <typename... Types>
void templ_test2(UNUSED int unused_, Types &&...args)
{
      int          arr[] = {args...};
      size_t const nelem = sizeof(arr) / sizeof(*arr);

      for (unsigned i = 0; i < nelem; ++i)
            std::cerr << "Have:\t" << arr[i] << '\n';
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


int
main(UNUSED int argc, UNUSED char *argv[])
{
#ifdef _WIN32
      init_wsa();
#endif

#if 0
      std::map<std::string, std::any> thing;
      thing.insert_or_assign( {"hello"}, std::string{"hello"});
      thing.insert_or_assign( {"fak"}, 3 );
      std::cout << std::any_cast<std::string>(thing["hello"]) << std::endl;;
      std::cout << std::any_cast<int>(thing["fak"]) << std::endl;;
#endif

      {
            using emlsp::rpc::unix_socket_connection;
            using emlsp::rpc::pipe_connection;
            using client_type = emlsp::client::client_type;

            std::vector<char const *> foo = {"clangd", "--pretty", "--pch-storage=memory", "--log=verbose", nullptr};
            pipe_connection myclient{client_type::LSP};
            myclient.spawn_connection_l("gopls", "-vv", "serve", "-rpc.trace");
            std::cerr.flush();

            std::string const header = "Content-Length: " + std::to_string(sizeof(initial_msg) - 1) + "\r\n\r\n" ;
            myclient.write(header.data(), header.size());
            myclient.write(initial_msg, sizeof(initial_msg) - 1);

            char buf[8192];
            myclient.read(buf, 3);
      }

      try {
#if 0
            TRY_FUNC(emlsp::rpc::json::nloh::test1);
            TRY_FUNC(emlsp::rpc::json::nloh::test2);
            TRY_FUNC(emlsp::rpc::json::rapid::test1);
            TRY_FUNC(emlsp::rpc::json::rapid::test2);
            TRY_FUNC(emlsp::rpc::json::test1);
            TRY_FUNC(emlsp::rpc::json::test2);
            TRY_FUNC(emlsp::rpc::event::test1);
            TRY_FUNC(emlsp::rpc::json::test3);
            TRY_FUNC(emlsp::rpc::test::test01);
#endif
      } catch (...) {
      }

#ifdef _WIN32
      WSACleanup();
#endif

      return 0;
}
