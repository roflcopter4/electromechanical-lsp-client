#include "Common.hh"
#include "ipc/lsp/lsp-connection.hh"

#include "thehellisthis.hh"

//#include <nlohmann/json.hpp>
//char const *malloc_conf = "stats_print:true,confirm_conf:true,abort_conf:true,prof_leak:true";

/****************************************************************************************/

inline namespace emlsp {
 namespace ipc {
  void run_event_loop_cxx(int);
#if 0
  namespace lsp {
   namespace test {
    NOINLINE extern void test10();
    NOINLINE extern void test11();
    NOINLINE extern void test12();
   } // namespace test
  } // namespace lsp
#endif
  namespace test {
  } // namespace test
 } // namespace ipc
 namespace event {
  NOINLINE extern void test03();
  NOINLINE extern void lets_do_this(std::filesystem::path const &fname);
 } // namespace event
 namespace libevent {
  NOINLINE void testing01(std::filesystem::path const &fname);
  NOINLINE void testing02(std::filesystem::path const &fname);
 } // namespace libevent
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

// NOLINTNEXTLINE(bugprone-exception-escape)
int main(UNUSED int argc, UNUSED char *argv[])
{
#ifdef _WIN32
      init_wsa();
      if (_setmode(0, _O_BINARY) == (-1))
            util::win32::error_exit(L"_setmode(0)");
      if (_setmode(1, _O_BINARY) == (-1))
            util::win32::error_exit(L"_setmode(1)");
      if (_setmode(2, _O_BINARY) == (-1))
            util::win32::error_exit(L"_setmode(2)");
#endif
      

      if (argc < 2) {
            fmt::print(stderr, FMT_COMPILE("ERROR: Missing paramater.\n"));
            exit(2);
      }
      if (argc > 2) {
            fmt::print(stderr, FMT_COMPILE("ERROR: Too many paramaters.\n"));
            exit(3);
      }
      if (!std::filesystem::exists(argv[1]) || std::filesystem::is_directory(argv[1])) {
            fmt::print(stderr, FMT_COMPILE("ERROR: Invalid paramater \"{}\": must be a file.\n"), argv[1]);
            exit(4);
      }

      try {
            // TRY_FUNC(event::test03);
            // TRY_FUNC(ipc::lsp::test::test11);
            // TRY_FUNC(ipc::lsp::test::test12);
            // ipc::run_event_loop_cxx(0);
            // event::lets_do_this(std::filesystem::canonical(argv[1]));
            //libevent::testing01(std::filesystem::canonical(argv[1]));
            libevent::testing02(std::filesystem::canonical(argv[1]));
      } catch (std::exception &e) {
            DUMP_EXCEPTION(e);
      }

#ifdef _WIN32
      WSACleanup();
#endif
      return 0;
}
