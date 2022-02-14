#include "Common.hh"
#include "ipc/lsp/lsp-connection.hh"

#include "util/owning_string_view.hh"

#include "util/charconv.hh"
#include <unistr.h>
#include <string>


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

UNUSED static constexpr char barline[] = "\033[1;32m--------------------------------------------------------------------------------\033[0m";

/****************************************************************************************/

inline namespace emlsp {

INITIALIZER_HACK(setup_locale)
{
      setlocale(LC_ALL, "en_US.UTF8");
      setlocale(LC_COLLATE, "en_US.UTF8");
      setlocale(LC_CTYPE, "en_US.UTF8");
      setlocale(LC_MONETARY, "en_US.UTF8");
      setlocale(LC_NUMERIC, "en_US.UTF8");
      setlocale(LC_TIME, "en_US.UTF8");

      ::localeconv();
}

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
  NOINLINE extern void testing01(std::filesystem::path const &fname);
  NOINLINE extern void testing02(std::filesystem::path const &fname);
  NOINLINE extern void testing03(std::filesystem::path const &fname);
  NOINLINE extern void testing04(std::filesystem::path const &fname);
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
 } // namespace rpc

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


namespace {

template <typename T>
void
my_free(T const *ptr)
{
      ::free(const_cast<void *>(static_cast<void const *>(ptr)));
}

} // namespace


template <typename ...T>
NORETURN static __inline constexpr void dump_and_exit(int const eval, T && ... args)
{
      fmt::print(stderr, std::forward<T &&>(args)...);
      ::exit(eval);
}

static void
dickhead(char const *const __restrict fmt,  ...)
{
      va_list ap;
      va_start(ap, fmt);
      //auto const n = vsnprintf(nullptr, 0, fmt, ap);

      char toosmall[6];

      int const n = _vsnprintf(
            toosmall, 6, fmt, ap
      );

      va_end(ap);


      std::cout << "nchars: "sv << n << "  and '"sv << util::char_repr(toosmall[5]) << "'\n"sv;
}

NOINLINE static void
brainless()
{
      namespace uni = util::unistring;

      static constexpr wchar_t teststr[] = L"NORETURN static __inline constexpr void dump_and_exit(int const eval, T && ... args)  §♀↔∞+oτ█▼↨☻♂W5Ü♠○X Æ╫╒J♫,╔EÉÜÆì>";
           //LR"(Конференция соберет широкий круг экспертов по  вопросам глобального Лорем ипсум долор сит амет, аутем дицта нуллам цу еам, стет диссентиет детерруиссет ин вис)";

      try {
            //auto const ass = util::unistring::char32_to_char(teststr, sizeof(teststr));
            //std::cout << *ass << std::endl;

            auto wos = std::wstringstream{};

            wos << L"HELLO!\n"sv;
            wos << L"HELLO!\n"sv;
            wos << teststr << L'\n';

            std::wcout << wos.view() << std::endl;

            {
                  char buf[sizeof(teststr) * 2];
                  size_t nbytes;
                  wcstombs_s(&nbytes, buf, sizeof buf, teststr, UINT64_MAX);
                  std::cout << nbytes << '\n' << buf << '\n';

                  dickhead("The quack broon fawks is %s 0x%" PRIX64 " %s\n", "hi there idiot mcfuckerface mcgee", ~UINT64_C(0), "bye");

                  auto const fmtstr = fmt::format(FC("The quack broon fawks is {} 0x{:X} {}\n"), "hi there idiot mcfuckerface mcgee", ~UINT64_C(0), "bye");
                  std::cout << "Real chars: "sv << fmtstr.size() << '\n';
            }

            std::cout << "fwrite:\n"sv;
            fwrite(teststr, 1, sizeof(teststr) - sizeof(teststr[0]), stdout);
            fputc('\n', stdout);

            auto const ass = uni::charconv<wchar_t>(teststr, uni::strlen(teststr));
            std::wcout << L"wcout rawstring:\n"sv << teststr << L"\nwcout cxx string:\n"sv << ass << L'\n';

            auto const bla = uni::charconv<char>(teststr, uni::strlen(teststr));
            std::cout << "cout converted char string:\n"sv << bla << std::endl;
      }
      catch (std::exception const &e) {
            DUMP_EXCEPTION(e);
      }
}

NORETURN static void
stupidity()
{
#ifdef _MSC_VER
      __try {
            __try {
                  brainless();
            } __except (EXCEPTION_EXECUTE_HANDLER) {
                  fprintf(stderr, "exception caught! -> %08lX or maybe 0x%08lX\n",
                          GetExceptionCode(), STATUS_ACCESS_VIOLATION);
            }
      } __finally {
            ExitProcess(42);
      }
#endif
}


/****************************************************************************************/

// NOLINTNEXTLINE(bugprone-exception-escape)
int main(int argc, char *argv[])
{
#ifdef _WIN32
      init_wsa();
      if (_setmode(0, _O_BINARY) == (-1))
            util::win32::error_exit(L"_setmode(0)");
      if (_setmode(1, _O_TEXT) == (-1))
            util::win32::error_exit(L"_setmode(1)");
      if (_setmode(2, _O_TEXT) == (-1))
            util::win32::error_exit(L"_setmode(2)");
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


      try {
            //TRY_FUNC(event::test03);
            //TRY_FUNC(ipc::lsp::test::test11);
            //TRY_FUNC(ipc::lsp::test::test12);
            //ipc::run_event_loop_cxx(0);
            //event::lets_do_this(src_file);
            //libevent::testing01(src_file);
            // test::mpack::test01(src_file);
            // test::mpack::test02(src_file);

            // libevent::testing02(src_file);

            //libevent::testing03(src_file);
            libevent::testing04({});

            // std::cout.flush(); std::cerr.flush(); std::cout << '\n' << barline << barline << "\n\n"; std::cout.flush();
            // test::mpack::test01(src_file);
            // std::cout.flush(); std::cerr.flush(); std::cout << '\n' << barline << barline << "\n\n"; std::cout.flush();
            // rpc::helpme();

      } catch (std::exception &e) {
            DUMP_EXCEPTION(e);
      }

#ifdef _WIN32
      WSACleanup();
#endif

      //stupidity();
      return 0;
}
