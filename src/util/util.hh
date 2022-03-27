#pragma once
#ifndef HGUARD__UTIL__UTIL_HH_
#define HGUARD__UTIL__UTIL_HH_ //NOLINT
/*--------------------------------------------------------------------------------------*/

#include "Common.hh"
#include "util/c_util.h"
#include "util/concepts.hh"
#include "util/exceptions.hh"
#include "util/formatters.hh"
#include "util/strings.hh"

#include "hackish_templates.hh"

#include "util/charconv.hh"

inline namespace emlsp {
namespace util {
/****************************************************************************************/

extern std::filesystem::path get_temporary_directory(char const *prefix = nullptr);
extern std::filesystem::path get_temporary_filename(char const *prefix = nullptr, char const *suffix = nullptr);

extern std::string slurp_file(char const *fname, bool binary = false);
inline std::string slurp_file(std::string const &fname, bool const binary = false)           { return slurp_file(fname.c_str(), binary); }
inline std::string slurp_file(std::string_view const &fname, bool const binary = false)      { return slurp_file(fname.data(), binary); }
inline std::string slurp_file(std::filesystem::path const &fname, bool const binary = false) { return slurp_file(fname.string().c_str(), binary); }

ND extern std::string char_repr(char ch);
ND extern std::string my_strerror(errno_t errval);
using ::my_strerror;

ND extern std::string demangle(_Notnull_ char const *raw_name) __attribute__((__nonnull__));
ND extern std::string demangle(std::type_info const &id);


/* I wouldn't normally check the return of malloc & friends, but MSVC loudly complains
 * about possible null pointers. So I'll make an exception. Damn it Microsoft. */
template <typename T>
ND __forceinline T *
xcalloc(size_t const nmemb = SIZE_C(1), size_t const size = sizeof(T)) noexcept(false)
{
      //NOLINTNEXTLINE(hicpp-no-malloc,cppcoreguidelines-no-malloc)
      void *ret = calloc(nmemb, size);
      if (ret == nullptr) [[unlikely]]
            throw std::runtime_error("Out of memory.");
      return static_cast<T *>(ret);
}

/*--------------------------------------------------------------------------------------*/

namespace detail {
#if defined TIOCINQ
constexpr uint64_t ioctl_size_available = TIOCINQ;
#elif defined FIONREAD
constexpr uint64_t ioctl_size_available = FIONREAD;
#else
# error "Neither TIOCINQ nor FIONREAD exist as ioctl parameters on this system."
#endif
#ifdef _WIN32
using procinfo_t = PROCESS_INFORMATION;
#else
using procinfo_t = pid_t;
#endif
} // namespace detail

#ifdef _WIN32
void      close_descriptor(HANDLE &fd);
void      close_descriptor(SOCKET &fd);
ND size_t available_in_fd(SOCKET s) noexcept(false);
ND size_t available_in_fd(HANDLE s) noexcept(false);
#endif

void      close_descriptor(int &fd);
void      close_descriptor(intptr_t &fd);
ND size_t available_in_fd(int s) noexcept(false);

int kill_process(detail::procinfo_t const &pid);

/*--------------------------------------------------------------------------------------*/

inline int putenv(char const *str) { return ::putenv(const_cast<char *>(str)); }

/*--------------------------------------------------------------------------------------*/

ND __attr_access((__write_only__, 1)) __attribute__((__artificial__, __nonnull__))
__forceinline size_t
strlcpy(_Notnull_ char       *__restrict dst,
        _Notnull_ char const *__restrict src,
        size_t const                     size)
{
#ifdef HAVE_STRLCPY
      return ::strlcpy(dst, src, size);
#else
      return ::emlsp_strlcpy(dst, src, size);
#endif
}

template <size_t N>
ND __attr_access((__write_only__, 1)) __attribute__((__artificial__, __nonnull__))
__forceinline size_t
strlcpy(_Notnull_ char                  (&dst)[N],
        _Notnull_ char const *__restrict src)
{
#if defined HAVE_STRLCPY
      return ::strlcpy(dst, src, N);
#elif defined HAVE_STRCPY_S && 0
      return strcpy_s(dst, src);
#else
      return ::emlsp_strlcpy(dst, src, N);
#endif
}


template <typename T>
ND __attribute__((__const__, __artificial__, __nonnull__))
__forceinline constexpr uintptr_t
ptr_diff(_Notnull_ T const *ptr1, _Notnull_ T const *ptr2)
{
      return static_cast<uintptr_t>(reinterpret_cast<ptrdiff_t>(ptr1) -
                                    reinterpret_cast<ptrdiff_t>(ptr2));
}

template <typename ...Types>
void eprint(Types &&...args)
{
      fmt::print(stderr, args...);
}

/*--------------------------------------------------------------------------------------*/

namespace ipc {

extern socket_t  open_new_unix_socket(char const *path);
extern socket_t  connect_to_unix_socket(char const *path);
extern socket_t  connect_to_unix_socket(sockaddr_un const *addr);
extern struct addrinfo *resolve_addr(char const *server, char const *port);

} // namespace ipc

#ifdef _WIN32
namespace win32 {

NORETURN void error_exit_explicit(wchar_t const *msg, DWORD errval);

NORETURN inline void error_exit(wchar_t const *msg)       { error_exit_explicit(msg, GetLastError()); }
NORETURN inline void error_exit_wsa(wchar_t const *msg)   { error_exit_explicit(msg, WSAGetLastError()); }
NORETURN inline void error_exit_errno(wchar_t const *msg) { error_exit_explicit(msg, errno); }

NORETURN inline void error_exit(std::wstring const &msg)       { error_exit_explicit(msg.c_str(), GetLastError()); }
NORETURN inline void error_exit_wsa(std::wstring const &msg)   { error_exit_explicit(msg.c_str(), WSAGetLastError()); }
NORETURN inline void error_exit_errno(std::wstring const &msg) { error_exit_explicit(msg.c_str(), errno); }

//void error_exit(wchar_t const *lpsz_function);
//void WSA_error_exit(wchar_t const *lpsz_function);

} // namespace win32
#endif

/****************************************************************************************/
} // namespace util
} // namespace emlsp


using emlsp::util::putenv;


/****************************************************************************************/
#endif /* util.hh */
// vim: ft=cpp
