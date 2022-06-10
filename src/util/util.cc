// ReSharper disable CppClangTidyConcurrencyMtUnsafe
// ReSharper disable CppClangTidyPerformanceNoIntToPtr
// ReSharper disable CppTooWideScopeInitStatement

#include "Common.hh"
#include "util/myerr.h"
#include "util/util.hh"

#include <backtrace.h>
#include <charconv>

#include <boost/stacktrace.hpp>

#define AUTOC auto const

#if defined HAVE_EXECINFO_H
# include <execinfo.h>
#elif defined _WIN32
# include <DbgHelp.h>
#endif

#if defined HAVE_MKDTEMP
#  define MKDTEMP(x) ::mkdtemp(x)
#else
#  include <glib.h>
#  define MKDTEMP(x) ::g_mkdtemp(x)
#endif

#ifndef _MSC_VER
#  define _Printf_format_string_
#endif

#ifdef _WIN32
#  define FATAL_ERROR(msg) win32::error_exit_wsa(L ## msg)
#  define FILESEP_CHR  '\\'
#  define FILESEP_STR  "\\"
#else
#  define FILESEP_CHR  '/'
#  define FILESEP_STR  "/"
#  define FATAL_ERROR(msg)                                                             \
      do {                                                                             \
            char buf[128];                                                             \
            std::string s_ = std::string(msg) + ": " + ::my_strerror(errno, buf, 128); \
            throw std::runtime_error(s_);                                              \
      } while (0)
#endif

/****************************************************************************************/

inline namespace emlsp {
namespace util {

namespace constants {
bool const coloured_stderr = ::isatty(2);
} // namespace constants


namespace {

void cleanup_sighandler(int signum);

class cleanup_c
{
      std::filesystem::path             tmp_path_{};
      std::stack<std::filesystem::path> delete_list_{};
      std::recursive_mutex              mtx_{};

      template <typename ...Types>
      static constexpr void dbg_log(UNUSED Types &&...args)
      {
#ifndef NDEBUG
            fmt::print(stderr, std::forward<Types &&>(args)...);
#endif
      }

    public:
      void push(std::filesystem::path const &path_arg)
      {
            std::lock_guard lock(mtx_);
            if (tmp_path_.empty())
                  tmp_path_ = path_arg;
            if (delete_list_.empty() || path_arg != delete_list_.top())
                  delete_list_.emplace(path_arg);
      }

      std::filesystem::path const &get_path() &
      {
            std::lock_guard lock(mtx_);
            if (tmp_path_ == L"")
                  push(get_temporary_directory(MAIN_PROJECT_NAME "."));
            return tmp_path_;
      }

      void do_cleanup()
      {
            std::lock_guard lock(mtx_);
            dbg_log(FC("\n"));

            while (!delete_list_.empty()) {
                  auto const value = delete_list_.top();
                  auto const name  = value.string();
                  delete_list_.pop();
                  dbg_log(FC("Removing path '{}'\n"), name);

                  std::error_code e;
                  std::filesystem::remove(value, e);
                  if (e)
                        dbg_log(FC("Failed to remove file '{}': \"{}\"\n"), name, e);
            }

            fflush(stderr);
            tmp_path_ = std::filesystem::path{};
      }

      ~cleanup_c() noexcept
      {
            try {
                  do_cleanup();
            } catch (std::exception &e) {
                  try {
                        std::cerr << "Caught exception:\n    " << e.what()
                                  << "\nwhen cleaning temporary files.\n";
                        std::cerr.flush();
                  } catch (...) {}
            }
      }

      cleanup_c() noexcept
      {
#ifdef _WIN32
            signal(SIGABRT,  cleanup_sighandler);
            signal(SIGBREAK, cleanup_sighandler);
            signal(SIGFPE,   cleanup_sighandler);
            signal(SIGILL,   cleanup_sighandler);
            signal(SIGINT,   cleanup_sighandler);
            signal(SIGSEGV,  cleanup_sighandler);
            signal(SIGTERM,  cleanup_sighandler);
#else
            struct sigaction act{};
            act.sa_handler = cleanup_sighandler;
            act.sa_flags   = static_cast<int>(SA_NOCLDWAIT | SA_RESETHAND | SA_RESTART);
            sigaction(SIGABRT, &act, nullptr);
            sigaction(SIGFPE,  &act, nullptr);
            sigaction(SIGILL,  &act, nullptr);
            sigaction(SIGINT,  &act, nullptr);
            sigaction(SIGQUIT, &act, nullptr);
            sigaction(SIGSEGV, &act, nullptr);
            sigaction(SIGTERM, &act, nullptr);
            sigaction(SIGPIPE, &act, nullptr);
#endif
      }

      DELETE_COPY_CTORS(cleanup_c);
      DELETE_MOVE_CTORS(cleanup_c);
};

cleanup_c cleanup{};

void
cleanup_sighandler(int const signum)
{
      cleanup.do_cleanup();

#ifdef _WIN32
      signal(signum, SIG_DFL);
#else
      struct sigaction act {};
      act.sa_handler = SIG_DFL;  //NOLINT(*-union-access)
      sigaction(signum, &act, nullptr);
#endif

      raise(signum);
}

} // namespace

/*--------------------------------------------------------------------------------------*/

static __inline std::filesystem::path
do_mkdtemp(std::filesystem::path const &dir)
{
      char        buf[PATH_MAX + 1];
      auto const &dir_str = dir.string();
      size_t      size    = dir_str.size();

      memcpy(buf, dir_str.data(), size); // NOLINT(bugprone-not-null-terminated-result)
      memcpy(buf + size, "XXXXXX", SIZE_C(7));
      size += SIZE_C(6);

      errno = 0;
      char const *str = MKDTEMP(buf);
      if (!str)
            err_nothrow("g_mkdtemp");

      return { std::string_view{str, size} };
}

std::filesystem::path
get_temporary_directory(char const *prefix)
{
      auto tmp_dir = std::filesystem::temp_directory_path();
      if (prefix)
            tmp_dir /= prefix;

      auto ret = do_mkdtemp(tmp_dir);

      /* Paranoia; justified paranoia: on Windows g_mkdtemp doesn't set any
       * permissions at all. */
      permissions(ret, std::filesystem::perms::owner_all);

      cleanup.push(ret);
      return ret;
}

std::filesystem::path
get_temporary_filename(char const *__restrict prefix, char const *__restrict suffix)
{
      char        buf[PATH_MAX];
      auto const &dir_str  = cleanup.get_path().string();
      auto const  size     = ::braindead_tempname(buf, dir_str.c_str(), prefix, suffix);
      auto        ret      = std::filesystem::path{std::string_view{buf, size}};

      cleanup.push(ret);
      return ret;
}

/****************************************************************************************/

namespace ipc {

static constexpr socket_t invalid_socket = static_cast<socket_t>(-1);

static __forceinline void
init_sockaddr_un(struct sockaddr_un &addr, char const *const path)
{
      size_t const len = util::strlcpy(addr.sun_path, path);
      addr.sun_family  = AF_UNIX;

      if (len >= sizeof(addr.sun_path))
            throw std::logic_error(
                fmt::format(FC("The file path \"{}\" (len: {}) is too large to "
                               "fit into a UNIX socket address (size: {})"),
                            path, len + 1, sizeof(addr.sun_path)));
}

static __forceinline socket_t
open_socket(int dom, int type, int proto)
{
#ifdef _WIN32
      auto const sock = ::WSASocketW(dom, type, proto, nullptr, 0,
                                     WSA_FLAG_OVERLAPPED | WSA_FLAG_NO_HANDLE_INHERIT);
#else
      auto const sock = ::socket(dom, type, proto);
#endif
      if (sock == invalid_socket) [[unlikely]]
            FATAL_ERROR("socket");
#ifndef _WIN32
      int tmp = ::fcntl(sock, F_GETFL);
      ::fcntl(sock, F_SETFL, tmp | O_CLOEXEC);
#endif
      return sock;
}

socket_t
open_new_unix_socket(char const *const path, int dom, int type, int proto)
{
      struct sockaddr_un addr{};
      init_sockaddr_un(addr, path);
      socket_t const sock = open_socket(dom, type, proto);

      int const ret = ::bind(
          sock, reinterpret_cast<struct sockaddr *>(&addr),
          sizeof(struct sockaddr_un)
      );

      if (ret == (-1)) [[unlikely]]
            FATAL_ERROR("bind");
      if (::listen(sock, 1) == (-1)) [[unlikely]]
            FATAL_ERROR("listen");

      return sock;
}

socket_t
connect_to_unix_socket(struct sockaddr_un const *addr, int dom, int type, int proto)
{
      socket_t const sock = open_socket(dom, type, proto);

      int const ret = ::connect(
          sock, reinterpret_cast<struct sockaddr const *>(addr),
          sizeof(struct sockaddr_un)
      );

      if (ret == (-1))
            FATAL_ERROR("connect");

      return sock;
}

socket_t
connect_to_unix_socket(char const *const path, int dom, int type, int proto)
{
      struct sockaddr_un addr{};
      init_sockaddr_un(addr, path);
      return connect_to_unix_socket(&addr, dom, type, proto);
}


struct addrinfo *
resolve_addr(char const *server, char const *port, int const type)
{
      struct addrinfo  hints{};
      struct addrinfo *res = nullptr;

      if (auto const portnum = xatoi(port); portnum >= 65536)
            errx("Error: port %ju is not within the range (1, 65536).", portnum);

      hints.ai_flags    = AI_NUMERICSERV | AI_NUMERICHOST;
      hints.ai_socktype = type;

      int rc = getaddrinfo(server, port, &hints, &res);
      if (rc != 0)
            errx("Host not found for ('%s') and ('%s') --> %s",
                 server, port, gai_strerror(rc));

      return res;
}


} // namespace ipc


/****************************************************************************************/
/* RNG helper utils for C code */


extern "C" unsigned
cxx_random_device_get_random_val(void)
{
      static std::random_device rand_device;
      return rand_device();
}

extern "C" uint32_t
cxx_random_engine_get_random_val_32(void)
{
      static std::mt19937 rand_engine_32(cxx_random_device_get_random_val());
      return rand_engine_32();
}

extern "C" uint64_t
cxx_random_engine_get_random_val_64(void)
{
      static std::mt19937_64 rand_engine_64(
          (static_cast<uint64_t>(cxx_random_device_get_random_val()) << 32) |
          static_cast<uint64_t>(cxx_random_device_get_random_val()));

      return rand_engine_64();
}


/****************************************************************************************/


static void add_backtrace(std::string &buf);


void  //NOLINTNEXTLINE(cert-dcl50-cpp)
my_err_throw(_In_    bool const           print_err,
             _In_z_  char const *restrict file,
             _In_    int  const           line,
             _In_z_  char const *restrict func,
             _In_z_ _Printf_format_string_
             char const *restrict format,
             ...)
{
#ifdef _WIN32
      int const e = static_cast<int>(GetLastError());
#else
      int const e = errno;
#endif

      std::string buf;
      buf.reserve(2048);

      buf += fmt::format(FC("\033[1;31m" "Exception at:"
                            "\033[0m"    " `{}:{}' - func `{}':\n  "
                            "\033[1;32m" "what():"
                            "\033[0m"    " `"),
                         file, line, func);

      {
            va_list ap;
            char c_buf[2048];
            va_start(ap, format);
            auto const len = vsnprintf(c_buf, sizeof c_buf, format, ap);
            va_end(ap);

            buf.append(c_buf, len);
            buf.push_back('\'');
      }

      if (print_err)
            buf += fmt::format(FC("`  (errno {}: `{}')"), e,
                               std::error_code(e, std::system_category()).message());

      add_backtrace(buf);
      throw std::runtime_error(buf);
}


#if 0

static void
add_backtrace(std::string &buf)
{
      //g_on_error_stack_trace(program_invocation_short_name);
      std::stringstream ss;
      ss << boost::stacktrace::stacktrace();
      buf.push_back('\n');
      buf.append(ss.str());
}

#elif defined HAVE_EXECINFO_H

static void
add_backtrace(std::string &buf)
{
      void     *arr[256];
      int const num  = ::backtrace(arr, 256);
      char **symbols = ::backtrace_symbols(const_cast<void *const *>(arr), num);

      buf.append("\n\n\033[1mBACKTRACE:\033[0m"sv);

      for (int i = 0; i < num; ++i) {
            char *ptr = ::strchr(symbols[i], '(');
            if (!ptr || !*++ptr)
                  continue;

            ptrdiff_t const len = ptr - symbols[i];

            if (ptr[0] == '_' && ptr[1] == 'Z') {
                  buf += fmt::format(FC("\n{:3}: {}"),
                                     i, std::string_view(symbols[i], len));

                  char *end = ::strchr(ptr, '+');
                  if (end) {
                        bool success = true;
                        *end = '\0';
                        try {
                              buf += fmt::format(FC("\033[0;36m{}\033[0m + "),
                                                 util::demangle(ptr));
                        } catch (...) {
                              success = false;
                        }
                        if (success)
                              ptr = end + 1;
                        else
                              *end = '+';
                  }

                  buf += ptr;
            } else if (ptr[0] != '+') {
                  char *end = ::strchr(ptr, '+');
                  if (end) {
                        *end = '\0';
                        buf += fmt::format(FC("\n{:3}: {}\033[0;36m{}\033[0m + "),
                                           i, std::string_view(symbols[i], len),
                                           std::string_view(ptr, end - ptr));
                        ptr = end + 1;
                  }
                  buf += ptr;
            } else {
                  buf += fmt::format(FC("\n{:3}: {}"), i, symbols[i]);
            }
      }

      //NOLINTNEXTLINE(hicpp-no-malloc, cppcoreguidelines-no-malloc)
      free(symbols);
}

#elif defined _WIN32

static void
add_backtrace(std::string &buf)
{
      void *stack[256];
      AUTOC process = ::GetCurrentProcess();
      SymInitialize(process, nullptr, true);

      AUTOC frames = ::CaptureStackBackTrace(0, 256, stack, nullptr);
      auto *symbol = xmalloc<SYMBOL_INFO>(sizeof(SYMBOL_INFO) + (SIZE_C(1024) * sizeof(char)));
      memset(symbol, 0, sizeof(SYMBOL_INFO));

      symbol->MaxNameLen   = 1023;
      symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
      buf.append("\n\n\033[1mBACKTRACE:\033[0m"sv);

      for (unsigned i = 0; i < frames; i++) {
            ::SymFromAddr(process, reinterpret_cast<DWORD64>(stack[i]), nullptr, symbol);
            buf += fmt::format(FC("\n{:3}: {}  -  [0x{:08X}]"),
                               frames - i - 1,
                               demangle(symbol->Name),
                               symbol->Address);
      }

      ::free(symbol);
}

#else

static void add_backtrace(std::string const &) {}

#endif


#ifdef _WIN32
# include <strsafe.h>
namespace win32 {

NORETURN void
error_exit_explicit(wchar_t const *msg, DWORD const errval)
{
      WCHAR *msg_buf = nullptr;

      // Retrieve the system error message for the last-error code
      FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                     nullptr, errval, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                     reinterpret_cast<LPWSTR>(&msg_buf), 0, nullptr);

      auto *display_buf = static_cast<LPWSTR>(
            LocalAlloc(LMEM_ZEROINIT, (wcslen(msg) + wcslen(msg_buf) + SIZE_C(40)) * sizeof(WCHAR)));
      assert(display_buf != nullptr);

      StringCchPrintfW(display_buf, LocalSize(display_buf) / sizeof(WCHAR),
                       L"%s failed with error %u: %s",
                       msg, errval, msg_buf);

      ::MessageBoxW(nullptr, display_buf, L"Error", MB_OK);
      ::LocalFree(msg_buf);
      ::LocalFree(display_buf);
      ::exit(errval);
}


NORETURN void
error_exit_explicit(char const *msg, DWORD const errval)
{
      char *msg_buf = nullptr;

      // Retrieve the system error message for the last-error code
      FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                     nullptr, errval, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                     reinterpret_cast<LPSTR>(&msg_buf), 0, nullptr);

      auto *display_buf = static_cast<char *>(
            LocalAlloc(LMEM_ZEROINIT, strlen(msg) + strlen(msg_buf) + 40));
      assert(display_buf != nullptr);

      StringCchPrintfA(display_buf, LocalSize(display_buf),
                       "%s failed with error %u: %s",
                       msg, errval, msg_buf);

      ::MessageBoxA(nullptr, display_buf, "Error", MB_OK);
      ::LocalFree(msg_buf);
      ::LocalFree(display_buf);
      ::exit(errval);
}


NORETURN void
error_exit_message(wchar_t const *msg)
{
      ::MessageBoxW(nullptr, msg, L"Error", MB_OK);
      ::exit(EXIT_FAILURE);
}


NORETURN void
error_exit_message(char const *msg)
{
      ::MessageBoxA(nullptr, msg, "Error", MB_OK);
      ::exit(EXIT_FAILURE);
}

} // namespace win32
#endif


/****************************************************************************************/


std::string
slurp_file(char const *const fname, bool const binary)
{
      //NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init, hicpp-member-init)
      struct stat  st;
      std::string  buf;
      FILE        *fp = ::fopen(fname, binary ? "rb" : "r");
      if (!fp)
            err("fopen()");

      ::fstat(::fileno(fp), &st);
      buf.reserve(size_t(st.st_size) + SIZE_C(1));

      auto const nread = ::fread(buf.data(), 1, size_t(st.st_size), fp);
      if ((binary && nread != size_t(st.st_size)) || ssize_t(nread) <= 0)
            err("fread()");

      ::fclose(fp);
      resize_string_hack(buf, nread);

      /* This really does have to be buf.data()[...] because the NUL has to go after
       * the end of the string, which would throw an exception if done via operator[]. */
      buf.data()[nread] = '\0';  //NOLINT(readability-simplify-subscript-expr)
      return buf;
}

std::string char_repr(char const ch)
{
      switch (ch) {
      case '\n': return "\\n";
      case '\r': return "\\r";
      case '\0': return "\\0";
      case '\t': return "\\t";
      case ' ':  return "<SPACE>";
      default:   return std::string{ch};
      }
}

std::string
my_strerror(errno_t const errval)
{
      char buf[128];
      return {::my_strerror(errval, buf, sizeof(buf))};
}

uintmax_t
xatoi(char const *const number, bool const strict)
{
      char      *endptr;
      auto const val = ::strtoumax(number, &endptr, 0);

      if ((endptr == number) || (strict && *endptr != '\0'))
            errx("Invalid integer \"%s\".\n", number);

      return val;
}


/****************************************************************************************/

#ifdef _WIN32
void
close_descriptor(HANDLE &fd) noexcept
{
      if (reinterpret_cast<intptr_t>(fd) > 0 && fd != GetStdHandle(STD_ERROR_HANDLE))
            CloseHandle(fd);
      fd = reinterpret_cast<HANDLE>(-1);
}

void
close_descriptor(SOCKET &fd) noexcept
{
      if (static_cast<intptr_t>(fd) > 0)
            closesocket(fd);
      fd = static_cast<SOCKET>(-1);
}

void
close_descriptor(intptr_t &fd) noexcept
{
      if (fd > 0)
            closesocket(fd);
      fd = -1;
}

size_t
available_in_fd(SOCKET const s) noexcept(false)
{
      unsigned long value  = 0;
      int const     result = ::ioctlsocket(s, detail::ioctl_size_available, &value);
      if (result < 0)
            throw std::system_error(
                std::error_code{WSAGetLastError(), std::system_category()});
      return value;
}

size_t
available_in_fd(HANDLE const s) noexcept(false)
{
      DWORD avail;
      if (!PeekNamedPipe(s, nullptr, 0, nullptr, &avail, nullptr))
            throw std::system_error(
                std::error_code{WSAGetLastError(), std::system_category()});
      return avail;
}

#else

void
close_descriptor(intptr_t &fd) noexcept
{
      if (fd > 0)
            close(static_cast<int>(fd));
      fd = static_cast<int>(-1);
}
#endif

void
close_descriptor(int &fd) noexcept
{
      if (fd > 2)
            ::close(fd);
      fd = (-1);
}


/*--------------------------------------------------------------------------------------*/


size_t available_in_fd(int const s) noexcept(false)
{
#ifdef _WIN32
      HANDLE hand = _get_osfhandle(s);
      return available_in_fd(hand);
#else
      int value  = 0;
      int result = ::ioctl(s, detail::ioctl_size_available, &value);
      if (result < 0)
            throw std::system_error(
                std::error_code{errno, std::system_category()});

      return static_cast<size_t>(value);
#endif
}


int
kill_process(detail::procinfo_t &pid) noexcept
{
      int status = 0;

#ifdef _WIN32
      if (pid.hProcess && pid.hProcess != INVALID_HANDLE_VALUE) {
            ::TerminateProcess(pid.hProcess, 0);
            ::WaitForSingleObject(pid.hProcess, INFINITE);
            ::GetExitCodeProcess(pid.hProcess, reinterpret_cast<DWORD *>(&status));
            if (pid.hThread && pid.hThread != INVALID_HANDLE_VALUE)
                  ::CloseHandle(pid.hThread);
            ::CloseHandle(pid.hProcess);

            memset(&pid, 0, sizeof(pid));
            pid.hProcess = pid.hThread = HANDLE(-1);
      }
#else
      if (pid) {
            ::kill(pid, SIGTERM);
            ::waitpid(pid, &status, 0);
            status = WEXITSTATUS(status);
            pid    = 0;
      }
#endif

      return status;
}


#ifdef _WIN32
int
waitpid(HANDLE proc_hand)
{
      DWORD status;
      WaitForSingleObject(proc_hand, INFINITE);
      GetExitCodeProcess (proc_hand, &status);
      return static_cast<int>(status);
}
#endif


int
waitpid(detail::procinfo_t const &pid)
{
      int status;
#ifdef _WIN32
      status = waitpid(pid.hProcess);
#else
      ::waitpid(pid, &status, 0);
      status = WEXITSTATUS(status);
#endif
      return status;
}


/*--------------------------------------------------------------------------------------*/
/* Win32 CtrlHandler BS */


#ifdef _WIN32
namespace win32 {

BOOL WINAPI
myCtrlHandler(DWORD const type)
{
      bool action;

      switch (type) {
      case CTRL_C_EVENT:
            eprint(
                FC("\n{} {}\n\n"),
                styled("Note:"sv, fg(fmt::color::cyan) | fmt::emphasis::bold),
                styled("ctrl-c pressed. Exiting with moderate passive-aggressiveness."sv,
                       fg(fmt::color::beige))
            );
            action = true;
            break;

      // CTRL-CLOSE: confirm that the user wants to exit.
      case CTRL_CLOSE_EVENT:
            eprint(FC("\nCtrl-Close event\n"));
            action = true;
            break;

      // Pass other signals to the next handler.
      case CTRL_BREAK_EVENT:
            ::MessageBeep(MB_ICONERROR);
            eprint(FC("\nCtrl-Break event\n"));
            action = false;
            break;

      case CTRL_LOGOFF_EVENT:
            ::MessageBeep(MB_ICONQUESTION);
            eprint(FC("\nLogoff event?\n"));
            action = false;
            break;

      case CTRL_SHUTDOWN_EVENT:
            ::MessageBeep(MB_ICONQUESTION);
            eprint(FC("\nShutdown event?!\n"));
            action = false;
            break;

      default: {
            wchar_t buf[64];
            auto *it = fmt::format_to(buf, FC(L"Fatal error: unknown signal 0x{:08x}"), type);
            *it      = L'\0';
            error_exit(buf);
      }
      } // switch (type)

      if (action)
            ::exit(1);

      return action;
}

} // namespace win32
#endif


/****************************************************************************************/
} // namespace util
} // namespace emlsp
