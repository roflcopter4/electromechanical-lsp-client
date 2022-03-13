#include "Common.hh"

//#include <vld.h>

#if defined __GNUC__ || defined __clang__
# ifdef __clang__
#  define ATTRIBUTE_PRINTF(...) __attribute__((__format__(__printf__, __VA_ARGS__)))
# else
#  define ATTRIBUTE_PRINTF(...) __attribute__((__format__(__gnu_printf__, __VA_ARGS__)))
# endif
#else
# define ATTRIBUTE_PRINTF(...)
#endif


#if defined USE_JEMALLOC && defined DEBUG
static constexpr
char my_malloc_conf[] = "abort_conf:true"
                        ",stats_print:true"
                        ",confirm_conf:true"
                        ",junk:true"
                        ",xmalloc:true"
                        ",prof:true"
                        ",prof_leak:true"
                        ",prof_final:true"
                        ;
char const *malloc_conf = my_malloc_conf;
#endif

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
  namespace FUCK {
   NOINLINE extern void foo();
  } // namespace FUCK
 } // namespace event
 namespace libevent {
  NOINLINE extern void testing01(std::filesystem::path const &fname);
  NOINLINE extern void testing02(std::filesystem::path const &fname);
  NOINLINE extern void testing03();
  NOINLINE extern void testing04();
  NOINLINE extern void testing05();
 } // namespace libevent

 namespace ipc::rpc {
  // NOINLINE void test01();
 } // namespace ipc::rpc

 namespace moron::test {
  NOINLINE extern void test1();
 } // namespace moron::test

 namespace test::mpack {
  NOINLINE extern void test01(std::filesystem::path const &fname);
  NOINLINE extern void test02(std::filesystem::path const &fname);
 } // namespace test::mpack

 namespace rpc {
  NOINLINE extern void helpme();
  namespace mpack {
   NOINLINE void test01();
  } // namespace mpack
 } // namespace rpc
 namespace test {
  namespace suck {
   NOINLINE extern void test01();
  } // namespace suck
  namespace nonexistant {}
 } // namespace test 
 namespace sigh {
  NOINLINE extern void sigh01();
  NOINLINE extern void sigh02();
  NOINLINE extern void sigh03();
  NOINLINE extern void sigh04();
  NOINLINE extern void sigh05();
  NOINLINE extern void sigh06();
 } // namespace sigh

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
            throw std::runtime_error("Bad winsock dll");
      }

      if (LOBYTE(wsa_data.wVersion) != 2 || HIBYTE(wsa_data.wVersion) != 2) {
            std::cerr << "Could not find a usable version of Winsock.dll\n";
            WSACleanup();
            throw std::runtime_error("Bad winsock dll");
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
do_main()
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

      //TRY_FUNC(event::test03);
      //TRY_FUNC(ipc::lsp::test::test11);
      //TRY_FUNC(ipc::lsp::test::test12);
      //ipc::run_event_loop_cxx(0);
      //event::lets_do_this(src_file);
      //libevent::testing01(src_file);
      // test::mpack::test01(src_file);
      // test::mpack::test02(src_file);


      // libevent::testing02(src_file);

      //libevent::testing03();
      //libevent::testing04();

      // emlsp::event::FUCK::foo();

      // test::suck::test01();
      //::rpc::mpack::test01();

      sigh::sigh04();

      // std::cout.flush(); std::cerr.flush(); std::cout << '\n' << barline << barline << "\n\n"; std::cout.flush();
      // test::mpack::test01(src_file);
      // std::cout.flush(); std::cerr.flush(); std::cout << '\n' << barline << barline << "\n\n"; std::cout.flush();
      // rpc::helpme();

#ifdef _WIN32
      WSACleanup();
#endif

      return 0;
}


// NOLINTNEXTLINE(bugprone-exception-escape)
int main(UNUSED int argc, UNUSED char *argv[])
{
      //int const orig_dbg_flags = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
      //_CrtSetDbgFlag(orig_dbg_flags | _CRTDBG_ALLOC_MEM_DF | _CRTDBG_CHECK_ALWAYS_DF | _CRTDBG_LEAK_CHECK_DF);

      try {
            do_main();
      } catch (std::exception &e) {
            DUMP_EXCEPTION(e);
      }

      //_CrtSetDbgFlag(orig_dbg_flags);
}
