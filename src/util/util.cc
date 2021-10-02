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
constexpr static void my_mkdtemp(char *templ) { mkdtemp(templ); }
#else
#  include <glib.h>
constexpr static void my_mkdtemp(char *templ) { g_mkdtemp(templ); }
#endif

#ifdef _MSC_VER
#  define __function__ __FUNCTION__
#else
#  define __function__ __extension__ __PRETTY_FUNCTION__
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
      path              tmp_path_{};
      std::mutex        mut_{};
      std::vector<path> delete_list_;

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
                        if (std::filesystem::exists(Path)) {
                              fprintf(stderr, "Removing path '%s'\n", Path.c_str());
                              std::filesystem::remove_all(Path);
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
            do_cleanup();
      }

      DELETE_COPY_CTORS(cleanup_c);
} cleanup;


void
cleanup_sighandler(int const signum)
{
      cleanup.do_cleanup();

#ifdef DOSISH
      signal(signum, SIG_DFL);
#elif 0
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
      char buf[PATH_MAX + 1];
      auto sys_dir = std::filesystem::temp_directory_path();

#if defined HAVE_MKDTEMP || defined __G_LIB_H__
      if (prefix) {
            snprintf(buf, sizeof(buf), "%s" FILESEP_STR "%sXXXXXX", sys_dir.c_str(), prefix);
      } else {
            auto str = sys_dir.string();
            memcpy(buf, str.data(), str.size()); //NOLINT(bugprone-not-null-terminated-result)
            memcpy(buf + str.size(), FILESEP_STR "XXXXXX", 8);
      }

      my_mkdtemp(buf);
      path ret(buf);
#else
      even_dumber_tempname(buf, sys_dir.c_str(), prefix, nullptr);

      path ret(buf);
      std::error_code code{};

      if (!std::filesystem::create_directory(ret, code) || code.value() != 0)
            errx(1, "std::filesystem::create_directory");

      std::filesystem::permissions(ret, std::filesystem::perms::owner_all, code);
      if (code.value() != 0)
            errx(1, "std::filesystem::permissions");
#endif

      cleanup.push(ret);
      return ret;
}

path
get_temporary_filename(char const *prefix, char const *suffix)
{
      char       buf[PATH_MAX + 1];
      auto const tmp_dir = cleanup.get_path();
      even_dumber_tempname(buf, tmp_dir.c_str(), prefix, suffix);

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
      if (connection_sock == (-1))
           FATAL_ERROR("socket");

#ifndef DOSISH
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
      if (data_sock == (-1))
            FATAL_ERROR("socket");

#ifndef DOSISH
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
namespace win32
{

void
error_exit(wchar_t const *lpsz_function)
{
      // Retrieve the system error message for the last-error code
      LPVOID lpMsgBuf;
      DWORD  dw = GetLastError();

      FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                        FORMAT_MESSAGE_IGNORE_INSERTS,
                    nullptr, dw, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                    reinterpret_cast<LPTSTR>(&lpMsgBuf), 0, nullptr);

      // Display the error message and exit the process
      LPVOID const lpDisplayBuf = LocalAlloc(
          LMEM_ZEROINIT, (lstrlen(static_cast<LPCTSTR>(lpMsgBuf)) +
                          lstrlenW(lpsz_function) + 40) * sizeof(TCHAR)
      );
      StringCchPrintf(static_cast<LPTSTR>(lpDisplayBuf),
                      LocalSize(lpDisplayBuf) / sizeof(TCHAR),
                      TEXT("%s failed with error %d: %s"), lpsz_function, dw, lpMsgBuf);
      MessageBox(nullptr, static_cast<LPCTSTR>(lpDisplayBuf), TEXT("Error"), MB_OK);

      LocalFree(lpMsgBuf);
      LocalFree(lpDisplayBuf);
      ExitProcess(dw);
}

} //namespace win32
#endif

} // namespace emlsp::util
