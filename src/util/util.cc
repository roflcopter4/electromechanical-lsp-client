#include "Common.hh"
#include "util/myerr.h"
#include "util/util.hh"

#include <filesystem>
#include <mutex>
#include <random>
#include <sys/stat.h>

#ifndef SOCK_CLOEXEC
#  define SOCK_CLOEXEC 0
#endif

#if defined HAVE_MKDTEMP
#  define MKDTEMP(x) mkdtemp(x)
#else
#  include <glib.h>
#  define MKDTEMP(x) g_mkdtemp(x)
#endif

#ifdef DOSISH
#  define FATAL_ERROR(msg) win32::error_exit(L ## msg)
#  define FILESEP_STR  "\\"
#else
#  define FILESEP_STR  "/"
#  define FATAL_ERROR(msg)                                              \
      do {                                                              \
            std::string s_ = std::string(msg) + ": " + strerror(errno); \
            throw std::runtime_error(s_);                               \
      } while (0)
#endif

/****************************************************************************************/

namespace emlsp::util {
namespace {

void cleanup_sighandler(int signum);

static class cleanup_c
{
    private:
      path tmp_path_{};
      std::vector<path>    delete_list_;
      std::recursive_mutex mut_{};

    public:
      void push(path const &Path)
      {
            mut_.lock();
            if (tmp_path_.empty())
                  tmp_path_ = Path;
            delete_list_.emplace_back(Path);
            mut_.unlock();
      }

      path &get_path()
      {
            mut_.lock();
            if (tmp_path_.empty())
                  push(get_temporary_directory(MAIN_PROJECT_NAME ".main."));
            mut_.unlock();
            return tmp_path_;
      }

      void do_cleanup()
      {
            mut_.lock();
            try {
                  for (auto const &Path : delete_list_) {
                        if (exists(Path)) {
                              fprintf(stderr, "\nRemoving path '%s'\n", Path.string().c_str());
                              remove_all(Path);
                        }
                  }
            } catch (std::exception &e) {
                  mut_.unlock();
                  throw e;
            }

            tmp_path_ = path{};
            delete_list_.clear();
            mut_.unlock();
      }

      cleanup_c()
      {
#ifdef DOSISH
            signal(SIGABRT, cleanup_sighandler);
            signal(SIGINT,  cleanup_sighandler);
            signal(SIGSEGV, cleanup_sighandler);
            signal(SIGTERM, cleanup_sighandler);
#else
            struct sigaction act{};
            act.sa_handler = cleanup_sighandler;
            act.sa_flags = SA_NOCLDWAIT | SA_RESETHAND | SA_RESTART;
            sigaction(SIGABRT, &act, nullptr);
            sigaction(SIGHUP,  &act, nullptr);
            sigaction(SIGINT,  &act, nullptr);
            sigaction(SIGPIPE, &act, nullptr);
            sigaction(SIGSEGV, &act, nullptr);
            sigaction(SIGTERM, &act, nullptr);
#endif
      }

      ~cleanup_c()
      {
            try {
                  do_cleanup();
            } catch (std::exception &e) {
                  std::cerr << fmt::format(FMT_COMPILE("Caught:\n{}\nwhen cleaning temporary files.\n"), e.what());
                  std::cerr.flush();
            }
      }

      DELETE_COPY_CTORS(cleanup_c);
      DELETE_MOVE_CTORS(cleanup_c);
} cleanup;


void
cleanup_sighandler(int const signum)
{
      cleanup.do_cleanup();

#ifdef DOSISH
      signal(signum, SIG_DFL);
#else
#if 0
      struct sigaction act {};
      act.sa_handler = SIG_DFL;
      sigaction(signum, &act, nullptr);
#endif
#endif

      raise(signum);
}

} // namespace

/*--------------------------------------------------------------------------------------*/

path
get_temporary_directory(char const *prefix)
{
      char       buf[PATH_MAX + 1];
      auto const sys_dir = std::filesystem::temp_directory_path();

      /* NOTE: The following _must_ use path.string().c_str() rather than just
       * path.c_str() because the latter, on Windows, returns a wide character string. */
      if (prefix) {
            snprintf(buf, sizeof(buf), "%s" FILESEP_STR "%sXXXXXX",
                     sys_dir.string().c_str(), prefix);
      } else {
            auto const str = sys_dir.string();
            memcpy(buf, str.data(), str.size()); //NOLINT(bugprone-not-null-terminated-result)
            memcpy(buf + str.size(), FILESEP_STR "XXXXXX", 8);
      }

      MKDTEMP(buf);
      path ret(buf);

      /* Paranoia. Justified paranoia: on Windows g_mkdtemp doesn't set any permissions
       * at all. */
      std::error_code code{};
      std::filesystem::permissions(ret, std::filesystem::perms::owner_all, code);
      if (code.value() != 0)
            errx(1, "std::filesystem::permissions");

      cleanup.push(ret);
      return ret;
}

path
get_temporary_filename(char const *prefix, char const *suffix)
{
      char buf[PATH_MAX + 1];
#if defined _WIN32 && defined USING_LIBUV
      even_dumber_tempname(buf, R"(\\.\pipe)", prefix, suffix);
#else
      auto const &tmp_dir = cleanup.get_path();
      even_dumber_tempname(buf, tmp_dir.string().c_str(), prefix, suffix);
#endif

      return {buf};
}

/****************************************************************************************/

namespace rpc {

socket_t
open_new_socket(char const *const path)
{
      struct sockaddr_un addr = {};
      strcpy(addr.sun_path, path);
      addr.sun_family = AF_UNIX;

      auto const connection_sock = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
#ifdef DOSISH
      if (auto const e = GetLastError(); e != 0)
            FATAL_ERROR("socket");
#else
      if (connection_sock == (-1))
           FATAL_ERROR("socket");
      int tmp = fcntl(connection_sock, F_GETFL);
      fcntl(connection_sock, F_SETFL, tmp | O_CLOEXEC);
#endif

      if (bind(connection_sock, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) == (-1))
            FATAL_ERROR("bind");
      if (listen(connection_sock, 0) == (-1))
            FATAL_ERROR("listen");

      return connection_sock;
}

socket_t
connect_to_socket(char const *const path)
{
      sockaddr_un addr{};
      strcpy(addr.sun_path, path);
      addr.sun_family = AF_UNIX;

      auto const data_sock = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
#ifdef DOSISH
      if (auto const e = GetLastError(); e != 0)
            FATAL_ERROR("socket");
#else
      if (data_sock == (-1))
           FATAL_ERROR("socket");
      int tmp = fcntl(data_sock, F_GETFL);
      fcntl(data_sock, F_SETFL, tmp | O_CLOEXEC);
#endif

      if (connect(data_sock, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) == (-1))
            FATAL_ERROR("connect");

      return data_sock;
}

} // namespace rpc

/****************************************************************************************/

extern "C" unsigned
cxx_random_device_get_random_val(void)
{
      static std::random_device rd;
      return rd();
}

#ifdef DOSISH
#include <strsafe.h>
namespace win32
{

void
error_exit(wchar_t const *lpsz_function)
{
      // Retrieve the system error message for the last-error code
      DWORD const dw      = WSAGetLastError();
      LPVOID      msg_buf = nullptr;

      FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                     nullptr, dw, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                     reinterpret_cast<LPWSTR>(&msg_buf), 0, nullptr);

      LPVOID lp_display_buf =
          LocalAlloc(LMEM_ZEROINIT,
                     (static_cast<size_t>(lstrlenW(lpsz_function)) +
                      static_cast<size_t>(lstrlenW(static_cast<LPWSTR>(msg_buf))) + SIZE_C(40)) * sizeof(WCHAR));
      assert(lp_display_buf != nullptr);
      StringCchPrintfW(static_cast<STRSAFE_LPWSTR>(lp_display_buf),
                       LocalSize(lp_display_buf) / sizeof(WCHAR), L"%s failed with error %d: %s",
                       lpsz_function, dw, static_cast<LPWSTR>(msg_buf));
      MessageBoxW(nullptr, static_cast<LPCWSTR>(lp_display_buf), L"Error", MB_OK);

      LocalFree(msg_buf);
      LocalFree(lp_display_buf);
      ExitProcess(dw);
}

} // namespace win32
#endif

void
my_err_throw(UNUSED int const     status,
             bool const           print_err,
             char const *restrict file,
             int  const           line,
             char const *restrict func,
             char const *restrict format,
             ...)
{
      int const e = errno;
      std::stringstream buf;
      mtx_lock(&util_c_error_write_mutex);

      buf << fmt::format(FMT_COMPILE("{}: ({} {} - {}):\n"), MAIN_PROJECT_NAME, file, line, func);

      {
            va_list ap;
            char c_buf[2048];
            va_start(ap, format);
            ::vsnprintf(c_buf, sizeof(buf), format, ap);
            va_end(ap);

            buf << c_buf << '\n';
      }

      if (print_err)
            buf << fmt::format(FMT_COMPILE("\n\terrno {}: \"{}\""), e, strerror(e));

      mtx_unlock(&util_c_error_write_mutex);
      throw std::runtime_error(buf.str());
}

} // namespace emlsp::util
