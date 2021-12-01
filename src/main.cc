#include "Common.hh"
#include "lsp-protocol/lsp-connection.hh"

//#include <nlohmann/json.hpp>

//char const *malloc_conf = "stats_print:true,confirm_conf:true,abort_conf:true,prof_leak:true";

#ifdef _WIN32
static std::string const home_dir = getenv("USERPROFILE");
#else
static std::string const home_dir = getenv("HOME");
#endif

/****************************************************************************************/

namespace emlsp {
 namespace ipc {
  void run_event_loop_cxx(int const);
  namespace lsp {
   namespace test {
    NOINLINE extern void test10();
    NOINLINE extern void test11();
    NOINLINE extern void test12();
   } // namespace test
  } // namespace lsp
  namespace test {
  } // namespace test
 } // namespace ipc
 namespace event {
  NOINLINE extern void test03();
  NOINLINE extern void lets_do_this(std::filesystem::path const &fname);
 } // namespace event
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


UNUSED __attribute__((nonnull))
static void
try_func(_Notnull_ void (*fn)(), _Notnull_ char const *repr)
{
      try {
            fn();
            std::cout << '\n';
            std::cout.flush();
      } catch (std::exception &e) {
            // Might as well flush the stream here.
            fmt::print(stderr, FMT_COMPILE("Caught exception running function \"{}\":  `{}'\n"), repr, e.what());
            std::cerr <<  "{}"_format(3);
            std::cerr.flush();
      }
}


// NOLINTNEXTLINE(bugprone-exception-escape)
int main(UNUSED int argc, UNUSED char *argv[])
{
#ifdef _WIN32
      init_wsa();
      if (_setmode(0, _O_BINARY) == (-1))
            emlsp::util::win32::error_exit(L"_setmode(0)");
      if (_setmode(1, _O_BINARY) == (-1))
            emlsp::util::win32::error_exit(L"_setmode(1)");
      if (_setmode(2, _O_BINARY) == (-1))
            emlsp::util::win32::error_exit(L"_setmode(2)");
#endif

      if (argc != 2)
            errx(1, "You are a freaking moron, you know that?");

      try {
            // TRY_FUNC(emlsp::event::test03);
            // TRY_FUNC(emlsp::ipc::lsp::test::test11);
            // TRY_FUNC(emlsp::ipc::lsp::test::test12);
            // emlsp::ipc::run_event_loop_cxx(0);
            emlsp::event::lets_do_this(std::filesystem::canonical(argv[1]));
      } catch (std::exception &e) {
            DUMP_EXCEPTION(e);
      }

#ifdef _WIN32
      WSACleanup();
#endif
      return 0;
}
