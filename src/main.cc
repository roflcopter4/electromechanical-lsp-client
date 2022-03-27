#include "Common.hh"

#include "util/charconv.hh"

//#include <vld.h>

/****************************************************************************************/

inline namespace emlsp {

 namespace libevent {
  NOINLINE extern void testing01(std::filesystem::path const &fname);
  NOINLINE extern void testing02(std::filesystem::path const &fname);
  NOINLINE extern void testing03();
  NOINLINE extern void testing04();
  NOINLINE extern void testing05();
 } // namespace libevent

 namespace rpc {
  namespace mpack {
   NOINLINE void test01();
  } // namespace mpack
 } // namespace rpc

 namespace sigh {
  NOINLINE extern void sigh01();
  NOINLINE extern void sigh02();
  NOINLINE extern void sigh03();
  NOINLINE extern void sigh04();
  NOINLINE extern void sigh05();
  NOINLINE extern void sigh06();
 } // namespace sigh

 namespace fool {
  NOINLINE void fool01();
 } // namespace fool

 namespace finally {
  NOINLINE void foo01();
  NOINLINE void foo02();
 } // namespace finally

} // namespace emlsp

/****************************************************************************************/

static void init_locale()
{
      ::setlocale(LC_ALL,      "en_US.UTF8");
      ::setlocale(LC_COLLATE,  "en_US.UTF8");
      ::setlocale(LC_CTYPE,    "en_US.UTF8");
      ::setlocale(LC_MONETARY, "en_US.UTF8");
      ::setlocale(LC_NUMERIC,  "en_US.UTF8");
      ::setlocale(LC_TIME,     "en_US.UTF8");
}

#ifdef _WIN32
static void init_wsa()
{
      constexpr WORD w_version_requested = MAKEWORD(2, 2);
      WSADATA        wsa_data;

      if (auto const err1 = WSAStartup(w_version_requested, &wsa_data) != 0) {
            std::cerr << "WSAStartup failed with error: " <<  err1 << '\n';
            util::win32::error_exit(L"Bad winsock dll");
      }

      if (LOBYTE(wsa_data.wVersion) != 2 || HIBYTE(wsa_data.wVersion) != 2) {
            std::cerr << "Could not find a usable version of Winsock.dll\n";
            WSACleanup();
            util::win32::error_exit(L"Bad winsock dll");
      }
}
#endif

UNUSED static void init_libevent()
{
      //event_set_mem_functions(malloc, realloc, free);
      event_enable_debug_mode();
      event_enable_debug_logging(EVENT_DBG_ALL);

#ifdef _WIN32
      if (evthread_use_windows_threads() != 0)
            util::win32::error_exit(L"evthread_use_windows_threads()");
#else
      if (evthread_use_pthreads() != 0)
            err(1, "evthread_use_pthreads()");
#endif
}

/*--------------------------------------------------------------------------------------*/

static int
do_main(UNUSED int argc, UNUSED char *argv[])
{
      init_locale();
#ifdef _WIN32
      init_wsa();
      if (_setmode(0, _O_BINARY) == (-1))
            util::win32::error_exit(L"_setmode(0)");
      if (_setmode(1, _O_TEXT) == (-1))
            util::win32::error_exit(L"_setmode(1)");
      if (_setmode(2, _O_TEXT) == (-1))
            util::win32::error_exit(L"_setmode(2)");
#endif
      //init_libevent();

      // if (argc != 2 || !argv[1] || !*argv[1])
      //       exit(1);

#if 0
      if (argc < 2)
            dump_and_exit(2, FC("ERROR: Missing paramater.\n"));
      if (argc > 2)
            dump_and_exit(3, FC("ERROR: Too many paramaters.\n"));

      std::error_code e;
      auto const src_file = std::filesystem::canonical(argv[1], e);

      if (e)
            dump_and_exit(4, FC("ERROR: Invalid paramater \"{}\": {}\n"), argv[1], e.message());
      if (!std::filesystem::exists(src_file))
            dump_and_exit(5, FC("ERROR: Invalid paramater \"{}\": Dangling symbolic link.\n"), argv[1]);
      if (!std::filesystem::is_regular_file(src_file))
            dump_and_exit(6, FC("ERROR: Invalid paramater \"{}\": Must be a regular file.\n"), src_file.string());
#endif


      // fool::fool01();
      // sigh::sigh04();
      finally::foo02();


#ifdef _WIN32
      WSACleanup();
#endif

      return 0;
}


int
main(int const argc, char *argv[])
{
      try {
            do_main(argc, argv);
      } catch (std::exception &e) {
            DUMP_EXCEPTION(e);
            return 1;
      }

      return 0;
}
