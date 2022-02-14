// ReSharper disable CppClangTidyPerformanceNoIntToPtr
// ReSharper disable CppTooWideScopeInitStatement

#include "Common.hh"
#include "util/myerr.h"
#include "util/util.hh"

#if defined HAVE_MKDTEMP
#  define MKDTEMP(x) mkdtemp(x)
#else
#  include <glib.h>
#  define MKDTEMP(x) g_mkdtemp(x)
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
#  define FATAL_ERROR(msg)                                                           \
      do {                                                                           \
            char buf[128];                                                           \
            std::string s_ = std::string(msg) + ": " + ::my_strerror(errno, buf, 128); \
            throw std::runtime_error(s_);                                            \
      } while (0)
#endif
#define IF if constexpr

/****************************************************************************************/

inline namespace emlsp {
namespace util {
namespace {

void cleanup_sighandler(int signum);

class cleanup_c
{
      std::filesystem::path             tmp_path_{};
      std::stack<std::filesystem::path> delete_list_{};
      std::recursive_mutex              mut_{};

    public:
      void push(std::filesystem::path const &path_arg)
      {
            mut_.lock();
            if (tmp_path_.empty())
                  tmp_path_ = path_arg;
            if (delete_list_.empty() || path_arg != delete_list_.top())
                  delete_list_.emplace(path_arg);
            mut_.unlock();
      }

      std::filesystem::path &get_path()
      {
            mut_.lock();
            if (tmp_path_ == "")
                  push(get_temporary_directory(MAIN_PROJECT_NAME "."));
            mut_.unlock();
            return tmp_path_;
      }

      void do_cleanup()
      {
            mut_.lock();
            try {
                  while (!delete_list_.empty()) {
                        auto const &value = delete_list_.top();
#ifndef NDEBUG
                        fmt::print(stderr, FC("\nRemoving path '{}'\n"), value.string());
#endif
                        std::error_code e;
                        std::filesystem::remove(value, e);
                        if (e)
                              fmt::print(stderr, "Failed to remove file '{}': \"{}\"\n", value, e);
                        delete_list_.pop();
                  }
                  fflush(stderr);
            } catch (...) {
                  mut_.unlock();
                  throw;
            }

            tmp_path_ = std::filesystem::path{};
            mut_.unlock();
      }

      cleanup_c()
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
            act.sa_handler = cleanup_sighandler; //NOLINT
            act.sa_flags   = static_cast<int>(SA_NOCLDWAIT | SA_RESETHAND | SA_RESTART);
            sigaction(SIGABRT, &act, nullptr);
            sigaction(SIGFPE,  &act, nullptr);
            sigaction(SIGHUP,  &act, nullptr);
            sigaction(SIGILL,  &act, nullptr);
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
                  try {
                        std::cerr << "Caught exception:\n    " << e.what()
                                  << "\nwhen cleaning temporary files.\n";
                        std::cerr.flush();
                  } catch (...) {}
            }
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
      act.sa_handler = SIG_DFL;
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
            err_nothrow(1, "g_mkdtemp");

      return { std::string_view{str, size} };
}

std::filesystem::path
get_temporary_directory(char const *prefix)
{
      auto tmp_dir = std::filesystem::temp_directory_path();
      if (prefix)
            tmp_dir /= prefix;

      auto ret = std::filesystem::path(do_mkdtemp(tmp_dir));

      /* Paranoia; justified paranoia: on Windows g_mkdtemp doesn't set any
       * permissions at all. */
      std::filesystem::permissions(ret, std::filesystem::perms::owner_all);

      cleanup.push(ret);
      return ret;
}

std::filesystem::path
get_temporary_filename(char const *prefix, char const *suffix)
{
      char        buf[PATH_MAX + 1];
      auto const &dir_str  = cleanup.get_path().string();
      auto const  size     = braindead_tempname(buf, dir_str.c_str(), prefix, suffix);
      auto        ret      = std::filesystem::path{std::string_view{buf, size}};

      cleanup.push(ret);
      return ret;
}

/****************************************************************************************/

namespace ipc {

socket_t
open_new_socket(char const *const path)
{
      struct sockaddr_un addr = {};
      size_t const       len  = util::strlcpy(addr.sun_path, path);
      addr.sun_family         = AF_UNIX;

      if (len >= sizeof(addr.sun_path))
            throw std::logic_error(
                fmt::format(FMT_COMPILE("The file path \"{}\" (len: {}) is too large to "
                                        "fit into a UNIX socket address (size: {})"),
                            path, len + 1, sizeof(addr.sun_path)));

      auto const connection_sock = ::socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);

#ifdef _WIN32
      if (auto const e = ::WSAGetLastError(); e != 0) [[unlikely]]
            FATAL_ERROR("socket");
#else
      if (connection_sock == (-1)) [[unlikely]]
            FATAL_ERROR("socket");
      int tmp = ::fcntl(connection_sock, F_GETFL);
      ::fcntl(connection_sock, F_SETFL, tmp | O_CLOEXEC);
#endif

      int const ret = ::bind(
          connection_sock, reinterpret_cast<sockaddr *>(&addr), sizeof(sockaddr_un)
      );

      if (ret == (-1)) [[unlikely]]
            FATAL_ERROR("bind");
      if (::listen(connection_sock, 0) == (-1)) [[unlikely]]
            FATAL_ERROR("listen");

      return connection_sock;
}

socket_t
connect_to_socket(sockaddr_un const *addr)
{
      auto const data_sock = ::socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);

#ifdef _WIN32
      if (auto const e = ::WSAGetLastError(); e != 0)
            FATAL_ERROR("socket");
#else
      if (data_sock == (-1))
           FATAL_ERROR("socket");
      int tmp = ::fcntl(data_sock, F_GETFL);
      ::fcntl(data_sock, F_SETFL, tmp | O_CLOEXEC);
#endif

      int const ret = ::connect(
          data_sock, reinterpret_cast<sockaddr const *>(addr), sizeof(sockaddr_un)
      );

      if (ret == (-1))
            FATAL_ERROR("connect");

      return data_sock;
}

socket_t
connect_to_socket(char const *const path)
{
      sockaddr_un  addr{};
      size_t const len = util::strlcpy(addr.sun_path, path);
      addr.sun_family  = AF_UNIX;

      if (len >= sizeof(addr.sun_path))
            throw std::logic_error(
                fmt::format(FMT_COMPILE("The file path \"{}\" (len: {}) is too large to "
                                        "fit into a UNIX socket address (max size: {})"),
                            path, len + 1, sizeof(addr.sun_path)));

      return connect_to_socket(&addr);
}

} // namespace ipc

/****************************************************************************************/

extern "C" uint32_t cxx_random_device_get_random_val(void)
      __attribute__((__visibility__("hidden")));

static std::random_device rand_device;
static std::mt19937       rand_engine_32(rand_device());
static std::mt19937_64    rand_engine_64(static_cast<uint64_t>(rand_device()) << 32 |
                                         static_cast<uint64_t>(rand_device()));

extern "C" unsigned
cxx_random_device_get_random_val(void)
{
      return rand_device();
}

extern "C" uint32_t
cxx_random_engine_get_random_val_32(void)
{
      return rand_engine_32();
}

extern "C" uint64_t
cxx_random_engine_get_random_val_64(void)
{
      return rand_engine_64();
}

/****************************************************************************************/

#ifdef _WIN32
#  include <strsafe.h>
namespace win32
{

NORETURN void
error_exit_explicit(wchar_t const *msg, DWORD const errval)
{
      void *msg_buf = nullptr;

      // Retrieve the system error message for the last-error code
      FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                         FORMAT_MESSAGE_IGNORE_INSERTS,
                     nullptr, errval, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                     reinterpret_cast<LPWSTR>(&msg_buf), 0, nullptr);

      void *lp_display_buf = LocalAlloc(
          LMEM_ZEROINIT,
          (wcslen(msg) + wcslen(static_cast<LPWSTR>(msg_buf)) + 40) * sizeof(WCHAR));
      assert(lp_display_buf != nullptr);

      StringCchPrintfW(static_cast<STRSAFE_LPWSTR>(lp_display_buf),
                       LocalSize(lp_display_buf) / sizeof(WCHAR),
                       L"%s failed with error %d: %s",
                       msg, errval, static_cast<LPWSTR>(msg_buf));

      MessageBoxW(nullptr, static_cast<LPCWSTR>(lp_display_buf), L"Error", MB_OK);
      LocalFree(msg_buf);
      LocalFree(lp_display_buf);
      ExitProcess(errval);
}

} // namespace win32
#endif

void
my_err_throw(UNUSED int const     status,
             bool const           print_err,
             char const *restrict file,
             int  const           line,
             char const *restrict func,
             _Notnull_ _Printf_format_string_
             char const *restrict format,
             ...)
{
      int const e = errno;
      std::string buf;
      ::mtx_lock(&util_c_error_write_mutex);

      buf += fmt::format(FC("\033[1;31m" "Exception at:"
                            "\033[0m"    " `{}:{}' - func `{}':\n  "
                            "\033[1;32m" "what():"
                            "\033[0m"    " '"),
                         file, line, func);

      {
            va_list ap;
            char c_buf[2048];
            va_start(ap, format);
            vsnprintf(c_buf, sizeof c_buf, format, ap);
            va_end(ap);

            buf += c_buf;
      }

      char        errbuf[128];
      char const *errstr = ::my_strerror(e, errbuf, 128);

      if (print_err)
            buf += fmt::format(FMT_COMPILE("`  (errno {}: `{}')"), e, errstr);

      ::mtx_unlock(&util_c_error_write_mutex);
      throw std::runtime_error(buf);
}

/****************************************************************************************/

std::string
slurp_file(char const *fname)
{
      std::string  buf;
      struct stat  st; // NOLINT(cppcoreguidelines-pro-type-member-init, hicpp-member-init)
      FILE        *fp = ::fopen(fname, "rb");
      if (!fp)
            err(1, "fopen()");

      ::fstat(::fileno(fp), &st);
      buf.reserve(static_cast<size_t>(st.st_size) + SIZE_C(1));
      auto const nread = ::fread(buf.data(), 1, static_cast<size_t>(st.st_size), fp);
      if (nread != static_cast<size_t>(st.st_size))
            err(1, "fread()");
      ::fclose(fp);

      resize_string_hack(buf, static_cast<size_t>(st.st_size));
      buf[static_cast<size_t>(st.st_size)] = '\0';
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

/****************************************************************************************/

#ifdef _WIN32
void
close_descriptor(HANDLE &fd)
{
      if (reinterpret_cast<intptr_t>(fd) > 0 && fd != GetStdHandle(STD_ERROR_HANDLE))
            CloseHandle(fd);
      fd = reinterpret_cast<HANDLE>(-1);
}

void
close_descriptor(SOCKET &fd)
{
      if (static_cast<intptr_t>(fd) > 0)
            closesocket(fd);
      fd = static_cast<SOCKET>(-1);
}

size_t
available_in_fd(SOCKET const s) noexcept(false)
{
      unsigned long value  = 0;
      int const     result = ::ioctlsocket(s, detail::ioctl_size_available, &value);
      if (result < 0)
            err(1, "ioctlsocket(TIOCINQ aka FIONREAD)");

      return value;
}
#endif

void
close_descriptor(int &fd)
{
      if (fd > 2)
            ::close(fd);
      fd = (-1);
}


#define WIN32_ERRMSG "The ioctl() function is not available for non-sockets on this platform."

#if defined _WIN32 && !defined __clang__
__attribute__((__error__(WIN32_ERRMSG)))
#endif
size_t available_in_fd(UNUSED int const s) noexcept(false)
{
#ifdef _WIN32
      //DeviceIoControl(nullptr, 0, nullptr, 0, nullptr, 0, nullptr, nullptr);
      throw except::not_implemented(P99_PASTE_2(WIN32_ERRMSG, s));
#else
      int value  = 0;
      int result = ::ioctl(s, detail::ioctl_size_available, &value);
      if (result < 0)
            err(1, "ioctl(TIOCINQ aka FIONREAD)");

      return static_cast<size_t>(value);
#endif
}

#undef WIN32_ERRMSG


/****************************************************************************************/
} // namespace util
} // namespace emlsp
