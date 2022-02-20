#pragma once
#ifndef HGUARD__UTIL__UTIL_HH_
#define HGUARD__UTIL__UTIL_HH_ //NOLINT
/*--------------------------------------------------------------------------------------*/

#include "Common.hh"
#include "util/c_util.h"
#include "util/concepts.hh"
#include "util/exceptions.hh"

#include "hackish_templates.hh"

inline namespace emlsp {
namespace util {
/****************************************************************************************/

extern std::filesystem::path get_temporary_directory(char const *prefix = nullptr);
extern std::filesystem::path get_temporary_filename(char const *prefix = nullptr, char const *suffix = nullptr);

extern std::string slurp_file(char const *fname);
inline std::string slurp_file(std::string const &fname)           { return slurp_file(fname.c_str()); }
inline std::string slurp_file(std::string_view const &fname)      { return slurp_file(fname.data()); }
inline std::string slurp_file(std::filesystem::path const &fname) { return slurp_file(fname.string().c_str()); }

ND extern std::string char_repr(char ch);
ND extern std::string my_strerror(errno_t errval);
using ::my_strerror;

ND extern std::string demangle(_Notnull_ char const *raw_name) __attribute__((__nonnull__));
ND extern std::string demangle(std::type_info const &id);

/*--------------------------------------------------------------------------------------*/

namespace detail {
#if defined TIOCINQ
constexpr uint64_t ioctl_size_available = TIOCINQ;
#elif defined FIONREAD
constexpr uint64_t ioctl_size_available = FIONREAD;
#else
# error "Neither TIOCINQ nor FIONREAD exist as ioctl parameters on this system."
#endif
} // namespace detail

#ifdef _WIN32
void      close_descriptor(HANDLE &fd);
void      close_descriptor(SOCKET &fd);
ND size_t available_in_fd(SOCKET s) noexcept(false);
#endif

void      close_descriptor(int &fd);
void      close_descriptor(intptr_t &fd);
ND size_t available_in_fd(int s) noexcept(false);

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

/*--------------------------------------------------------------------------------------*/

namespace ipc {

extern socket_t  open_new_unix_socket(char const *path);
extern socket_t  connect_to_unix_socket(char const *path);
extern socket_t  connect_to_unix_socket(sockaddr_un const *addr);
extern addrinfo *resolve_addr(char const *server, char const *port);

} // namespace ipc

#ifdef _WIN32
namespace win32 {

NORETURN void error_exit_explicit(wchar_t const *msg, DWORD errval);

inline void error_exit(wchar_t const *msg)       { return error_exit_explicit(msg, GetLastError()); }
inline void error_exit_wsa(wchar_t const *msg)   { return error_exit_explicit(msg, WSAGetLastError()); }
inline void error_exit_errno(wchar_t const *msg) { return error_exit_explicit(msg, errno); }

//void error_exit(wchar_t const *lpsz_function);
//void WSA_error_exit(wchar_t const *lpsz_function);

} // namespace win32
#endif

/****************************************************************************************/
} // namespace util
} // namespace emlsp
#endif /* util.hh */
// vim: ft=cpp
