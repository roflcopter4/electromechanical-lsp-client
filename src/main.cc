#include "Common.hh"
#include "lsp-protocol/lsp-connection.hh"
#include "lsp-protocol/static-data.hh"
#include "rapid.hh"

#include <nlohmann/json.hpp>

//char const *malloc_conf = "stats_print:true,confirm_conf:true,abort_conf:true,prof_leak:true";

/****************************************************************************************/

namespace emlsp {
 namespace rpc {
  namespace json {
   NOINLINE extern void test1();
   NOINLINE extern void test2();
   NOINLINE extern void test3();
   namespace nloh {
    NOINLINE extern void test1();
   } // namespace nloh 
   namespace rapid {
    NOINLINE extern void test1();
   } // namespace rapid 
  } // namespace json
  namespace event {
   NOINLINE extern void test1();
  } // namespace event
  namespace test {
   NOINLINE extern void test01();
   NOINLINE extern void test02();
  } // namespace test
  namespace lsp {
   NOINLINE extern void test10();
  } // namespace lsp
 } // namespace rpc
 namespace event {
  NOINLINE extern void test01();
  NOINLINE extern void test02();
  NOINLINE extern void test03();
 } // namespace event
 namespace test {
  NOINLINE extern void test01();
 } // namespace test 
} // namespace emlsp

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

using emlsp::rpc::lsp::unix_socket_connection;
using emlsp::rpc::lsp::pipe_connection;

int
main(UNUSED int argc, UNUSED char *argv[])
{
#ifdef _WIN32
      init_wsa();
#endif

      try {
            // emlsp::rpc::lsp::client<unix_socket_connection> client;
#if 0
            unix_socket_connection con;
            emlsp::rpc::lsp::client client(con);

            client.con().spawn_connection_l("clangd", "--pch-storage=memory", "--log=verbose");
            client.con().write_message_l(initialization_message);

            char  *buf;
            size_t len = client.con().read_message(buf);
            std::string msg {buf, len};
            std::cout.flush();
            std::cerr.flush();
            std::cout.flush();

            {
                  rapidjson::Document     obj;
                  rapidjson::StringBuffer ss;
                  rapidjson::PrettyWriter writer(ss);
                  obj.ParseInsitu(buf);
                  obj.Accept(writer);

                  std::cout << '\n' << ss.GetString() << '\n';
            }

            delete[] buf;
#endif

            TRY_FUNC(emlsp::event::test03);
      }
      catch (std::exception &e) {
            DUMP_EXCEPTION(e);
      }



#ifdef _WIN32
      WSACleanup();
#endif
      return 0;
}
