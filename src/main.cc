// ReSharper disable CppClangTidyConcurrencyMtUnsafe
#include "Common.hh"

#if defined _WIN32 && defined _MSC_VER
//# include <Shlwapi.h>
//# include <vld.h>
#endif

/****************************************************************************************/

inline namespace emlsp {

 namespace testing {
  NOINLINE void foo02();
  NOINLINE void foo03();
  NOINLINE void foo04();
  NOINLINE void foo05();
  NOINLINE void foo06();
  NOINLINE void foo10(char const *);
  NOINLINE void foo20();
  NOINLINE int  foo100();
  NOINLINE void foo101();
 } // namespace testing
} // namespace emlsp

/****************************************************************************************/

static void init_locale() noexcept
{
      // FIXME: Change this at some point. Obviously.
      static constexpr char    locale_name[] = "en_US.UTF8";

      ::setlocale(LC_ALL,      locale_name);
      ::setlocale(LC_COLLATE,  locale_name);
      ::setlocale(LC_CTYPE,    locale_name);
      ::setlocale(LC_MONETARY, locale_name);
      ::setlocale(LC_NUMERIC,  locale_name);
      ::setlocale(LC_TIME,     locale_name);
#ifdef LC_MESSAGES
      ::setlocale(LC_MESSAGES, locale_name);
#endif

#ifndef __MINGW64__
      std::locale::global(std::locale{locale_name});
#endif
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

static void init_crt_debugging()
{
#if 0
      _CrtSetReportMode(_CRT_WARN,   _CRTDBG_MODE_FILE);
      _CrtSetReportMode(_CRT_ERROR,  _CRTDBG_MODE_FILE);
      _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
      _CrtSetReportFile(_CRT_WARN,   _CRTDBG_FILE_STDOUT);
      _CrtSetReportFile(_CRT_ERROR,  _CRTDBG_FILE_STDOUT);
      _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDOUT);

      int const flag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
      _CrtSetDbgFlag(flag | _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

      //VLDRefreshModules();
      //auto const opts = VLDGetOptions();
      //util::eprint(FC("{:b}\n"), opts);
}
#endif

NOINLINE static void
call_initializers(UU int &argc, UU char **&argv)
{
      init_crt_debugging();
      init_locale();

#ifdef _WIN32
      init_wsa();
      //std::atexit(reinterpret_cast<void (*)()>(::WSACleanup));
      if (::_setmode(0, _O_BINARY) == (-1))
            util::win32::error_exit(L"_setmode(0)");
      if (::_setmode(1, _O_BINARY) == (-1))
            util::win32::error_exit(L"_setmode(1)");
      if (::_setmode(2, _O_TEXT) == (-1))
            util::win32::error_exit(L"_setmode(2)");
      if (!::SetConsoleCtrlHandler(&util::win32::myCtrlHandler, true))
            util::win32::error_exit(L"SetConsoleCtrlHandler");
#endif
}

/*--------------------------------------------------------------------------------------*/

static int
do_main(int argc, char *argv[])
{
      call_initializers(argc, argv);

      if (argc < 2)
            errx_nothrow("ERROR: Missing parameter.\n");
      if (argc > 2)
            errx_nothrow("ERROR: Too many parameters.\n");

      testing::foo101();

      // util::win32::warning_box_explicit("Die in fire:", WSAENOTSOCK);
      // util::win32::warning_box_explicit("Die in fire:", WSAECONNREFUSED);
      // util::win32::warning_box_explicit("Die in fire:", WSAEADDRINUSE);
      // util::win32::error_exit_explicit("Foo. Bar. Baz.", ENOENT);

      testing::foo02();
      // testing::foo04();
      // testing::foo05();
      // testing::foo06();
      // testing::foo10(argv[1]);

      //testing::foo20();

      return 0;
}

int
main(int argc, char *argv[])
{
      int r = 1;

      try {
            r = do_main(argc, argv);
      } catch (std::exception const &e) {
            DUMP_EXCEPTION(e);
      } catch (...) {
            eprintf("We really shouldn't have been able to get here. Like, really really shouldn't.\n");
      }

      return r;
}
