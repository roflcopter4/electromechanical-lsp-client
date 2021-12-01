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

class cleanup_c
{
    private:
      path                 tmp_path_{};
      std::vector<path>    delete_list_{};
      std::recursive_mutex mut_{};

    public:
      void push(path const &path_arg)
      {
            mut_.lock();
            if (tmp_path_.empty())
                  tmp_path_ = path_arg;
            delete_list_.emplace_back(path_arg);
            mut_.unlock();
      }

      path &get_path()
      {
            mut_.lock();
            if (tmp_path_.empty())
                  push(get_temporary_directory(MAIN_PROJECT_NAME "."));
            mut_.unlock();
            return tmp_path_;
      }

      void do_cleanup()
      {
            mut_.lock();
            try {
                  for (auto const &value : delete_list_) {
                        if (exists(value)) {
                              warnx("\nRemoving path '%s'\n", value.string().c_str());
                              remove_all(value);
                        }
                  }
                  fflush(stderr);
            } catch (...) {
                  mut_.unlock();
                  throw;
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
            act.sa_flags   = SA_NOCLDWAIT | SA_RESETHAND | SA_RESTART;
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

static cleanup_c cleanup{};

void
cleanup_sighandler(int const signum)
{
      cleanup.do_cleanup();

#ifdef DOSISH
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

      (void)MKDTEMP(buf);
      path ret(buf);

      /* Paranoia; justified paranoia: on Windows g_mkdtemp doesn't set any
       * permissions at all. */
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

      auto const &tmp_dir = cleanup.get_path();
      braindead_tempname(buf, tmp_dir.string().c_str(), prefix, suffix);

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

      auto const connection_sock = ::socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);

#ifdef DOSISH
      if (auto const e = ::WSAGetLastError(); e != 0)
            FATAL_ERROR("socket");
#else
      if (connection_sock == (-1))
           FATAL_ERROR("socket");
      int tmp = ::fcntl(connection_sock, F_GETFL);
      ::fcntl(connection_sock, F_SETFL, tmp | O_CLOEXEC);
#endif

      int const ret = ::bind(
          connection_sock, reinterpret_cast<sockaddr *>(&addr), sizeof(sockaddr_un)
      );

      if (ret == (-1))
            FATAL_ERROR("bind");
      if (::listen(connection_sock, 0) == (-1))
            FATAL_ERROR("listen");

      return connection_sock;
}

socket_t
connect_to_socket(sockaddr_un const *addr)
{
      auto const data_sock = ::socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);

#ifdef DOSISH
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
      sockaddr_un addr{};
      strcpy(addr.sun_path, path);
      addr.sun_family = AF_UNIX;
      return connect_to_socket(&addr);
}

} // namespace rpc

/****************************************************************************************/

extern "C" unsigned
cxx_random_device_get_random_val(void)
{
      static std::random_device rd;
      return rd();
}

extern "C" uint32_t
cxx_random_engine_get_random_val(void)
{
      static std::default_random_engine rand_engine(cxx_random_device_get_random_val());
      return static_cast<uint32_t>(rand_engine());
}

#ifdef DOSISH
#  include <strsafe.h>
namespace win32
{

void
error_exit(wchar_t const *lpsz_function)
{
      // Retrieve the system error message for the last-error code
      DWORD const dw      = WSAGetLastError();
      LPVOID      msg_buf = nullptr;

      FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                         FORMAT_MESSAGE_IGNORE_INSERTS,
                     nullptr, dw, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                     reinterpret_cast<LPWSTR>(&msg_buf), 0, nullptr);

      void *const lp_display_buf =
          LocalAlloc(LMEM_ZEROINIT,
                     (wcslen(lpsz_function) + wcslen(static_cast<LPWSTR>(msg_buf)) + 40) *
                         sizeof(WCHAR));
      assert(lp_display_buf != nullptr);

      StringCchPrintfW(static_cast<STRSAFE_LPWSTR>(lp_display_buf),
                       LocalSize(lp_display_buf) / sizeof(WCHAR),
                       L"%s failed with error %d: %s",
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
             _Notnull_ _Printf_format_string_
             char const *restrict format,
             ...)
{
      int const e = errno;
      std::string buf;
      ::mtx_lock(&util_c_error_write_mutex);

      buf += fmt::format(FMT_COMPILE("{}: ({} {} - {}):\n"),
                         MAIN_PROJECT_NAME, file, line, func);

      {
            va_list ap;
            char c_buf[2048];
            va_start(ap, format);
            vsnprintf(c_buf, sizeof c_buf, format, ap);
            va_end(ap);

            buf += c_buf;
            buf += '\n';
      }

      char *errstr;

#if defined HAVE_STRERROR_R
      char errbuf[256];
      errstr = ::strerror_r(e, errbuf, 256);
#elif defined HAVE_STRERROR_S
      char errbuf[256];
      ::strerror_s(errbuf, 256, e);
      errstr = errbuf;
#else
      errstr = strerror(e);
#endif

      if (print_err)
            buf += fmt::format(FMT_COMPILE("    errno {}: `{}'"), e, errstr);

      ::mtx_unlock(&util_c_error_write_mutex);
      throw std::runtime_error(buf);
}

/****************************************************************************************/

std::vector<char>
slurp_file(char const *fname)
{
      std::vector<char> buf;
      struct stat       st; // NOLINT(cppcoreguidelines-pro-type-member-init, hicpp-member-init)
      FILE             *fp = ::fopen(fname, "rb");
      if (!fp)
            err(1, "fopen()");

      ::fstat(::fileno(fp), &st);
      resize_vector_hack(buf, st.st_size + SIZE_C(1));
      if (::fread(buf.data(), 1, st.st_size, fp) != static_cast<size_t>(st.st_size))
            err(1, "fread()");
      ::fclose(fp);

      buf[st.st_size] = '\0';
      return buf;
}

} // namespace emlsp::util
