#include "Common.hh"
#include "ipc/connection_impl.hh"

#include <x86intrin.h>

#if defined _WIN32 && defined _MSC_VER
# include <Shlwapi.h>
#endif
//#include <vld.h>

/****************************************************************************************/

inline namespace emlsp {

 namespace testing01 {
  NOINLINE void foo02();
  NOINLINE void foo03();
 } // namespace testing01

} // namespace emlsp

/****************************************************************************************/

static void init_locale() noexcept
{
      ::setlocale(LC_ALL, "en_US.UTF8");
      ::setlocale(LC_COLLATE, "en_US.UTF8");
      ::setlocale(LC_CTYPE, "en_US.UTF8");
      ::setlocale(LC_MONETARY, "en_US.UTF8");
      ::setlocale(LC_NUMERIC, "en_US.UTF8");
      ::setlocale(LC_TIME, "en_US.UTF8");
}

#ifdef _WIN32
static void init_wsa()
{
      constexpr WORD w_version_requested = MAKEWORD(2, 2);
      WSADATA        wsa_data;

      if (::WSAStartup(w_version_requested, &wsa_data) != 0)
            util::win32::error_exit_wsa(L"Bad WinSock dll");

      if (LOBYTE(wsa_data.wVersion) != 2 || HIBYTE(wsa_data.wVersion) != 2) {
            int const e = WSAGetLastError();
            ::WSACleanup();
            util::win32::error_exit_explicit(L"Bad WinSock dll", e);
      }
}
#endif

#if 0
UNUSED static void init_libevent()
{
      //event_set_mem_functions(malloc, realloc, free);
      ::event_enable_debug_mode();
      ::event_enable_debug_logging(EVENT_DBG_ALL);

#ifdef _WIN32
      if (::evthread_use_windows_threads() != 0)
            util::win32::error_exit(L"evthread_use_windows_threads()");
#else
      if (::evthread_use_pthreads() != 0)
            err("evthread_use_pthreads()");
#endif
}
#endif

/*--------------------------------------------------------------------------------------*/

static int
do_main(UNUSED int argc, UNUSED char *argv[])
{
      init_locale();
#ifdef _WIN32
      init_wsa();
      if (_setmode(0, _O_BINARY) == (-1))
            util::win32::error_exit(L"_setmode(0)");
      if (_setmode(1, _O_BINARY) == (-1))
            util::win32::error_exit(L"_setmode(1)");
      if (_setmode(2, _O_BINARY) == (-1))
            util::win32::error_exit(L"_setmode(2)");
      if (!::SetConsoleCtrlHandler(util::win32::myCtrlHandler, true))
            util::win32::error_exit(L"SetConsoleCtrlHandler");
#endif

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

      //wchar_t sbuf[4096]{};
      //::GetShortPathNameW(long_filename[5], sbuf, std::size(sbuf));
      //fmt::print(FC("Got short name as \"{}\"\n"), util::recode<char>(sbuf));

      testing01::foo02();
      //testing01::foo03();

#ifdef _WIN32
      ::WSACleanup();
#endif

      return 0;
}


static int
do_main_wrap(int const argc, char *argv[])
{
      int r = 1;

      try {
            r = do_main(argc, argv);
      } catch (std::exception const &e) {
            DUMP_EXCEPTION(e);
      }

      return r;
}


int
main(int const argc, char *argv[])
{
#ifdef _MSC_VER
      int e = 0;
      int r = 1;

      //NOLINTNEXTLINE(clang-diagnostic-language-extension-token)
      __try {
            r = do_main_wrap(argc, argv);
      }
      __except (e = GetExceptionCode(), EXCEPTION_EXECUTE_HANDLER) {
            fprintf(stderr, "Caught SEH exception %d -> %s\n", e, "dunno");
            fflush(stderr);
      }
      return r;
#else
      return do_main_wrap(argc, argv);
#endif
}
