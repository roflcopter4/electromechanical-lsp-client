#pragma once
#ifndef HGUARD__UTIL__UTIL_HH_
#define HGUARD__UTIL__UTIL_HH_  //NOLINT
/*--------------------------------------------------------------------------------------*/

#include "Common.hh"
#include "util/c/c_util.h"
#include "util/concepts.hh"
#include "util/hackish_templates.hh"
#include "util/misc.hh"
#include "util/socket.hh"

#include "util/recode/recode.hh"

#define BARE_DECLTYPE(obj) std::remove_cvref_t<decltype(obj)>

inline namespace MAIN_PACKAGE_NAMESPACE {
namespace util {
/****************************************************************************************/


namespace hacks {
typedef std::filesystem::path path;  //NOLINT
typedef std::string_view      sview; //NOLINT
} // namespace hacks


namespace constants {
extern bool const coloured_stderr;
} // namespace constants


extern hacks::path get_temporary_directory(char const *prefix = nullptr);
extern hacks::path get_temporary_filename(char const *__restrict prefix = nullptr,
                                          char const *__restrict suffix = nullptr);


extern std::string slurp_file(char const         *fname, bool binary = false);
extern std::string slurp_file(std::string const  &fname, bool binary = false);
extern std::string slurp_file(hacks::path const  &fname, bool binary = false);
extern std::string slurp_file(hacks::sview const &fname, bool binary = false);


ND extern uintmax_t   xatoi(char const *number, bool strict = true);
ND extern std::string char_repr(char ch);
ND extern std::string my_strerror(errno_t errval);
using ::my_strerror;


ND extern std::string demangle(_In_z_ char const *raw_name) __attribute__((__nonnull__));
ND extern std::string demangle(std::type_info const &id);

/*--------------------------------------------------------------------------------------*/

namespace detail {
#if defined FIONREAD
constexpr uint64_t ioctl_size_available = FIONREAD;
#elif defined TIOCINQ
constexpr uint64_t ioctl_size_available = TIOCINQ;
#else
# error "Neither TIOCINQ nor FIONREAD exist as ioctl parameters on this system."
#endif
#ifdef _WIN32
using procinfo_t = PROCESS_INFORMATION;
#else
using procinfo_t = pid_t;
#endif
} // namespace detail


extern int  kill_process(detail::procinfo_t &pid) noexcept;
extern void close_descriptor(int &fd) noexcept;
extern void close_descriptor(intptr_t &fd) noexcept;

extern int waitpid(detail::procinfo_t const &pid);

#ifdef _WIN32

extern    void   close_descriptor(HANDLE &fd) noexcept;
extern    void   close_descriptor(SOCKET &fd) noexcept;
ND extern size_t available_in_fd (SOCKET s);
ND extern size_t available_in_fd (HANDLE s);
extern    int    waitpid(HANDLE pid);

#endif

ND extern size_t available_in_fd(int s);

/*--------------------------------------------------------------------------------------*/

#ifndef _WIN32
inline int putenv(char const *str) { return ::putenv(const_cast<char *>(str)); }
#endif

/*--------------------------------------------------------------------------------------*/


ND __attr_access((__write_only__, 1)) __attribute__((__artificial__, __nonnull__))
__forceinline size_t
strlcpy(_Out_z_cap_(size) char       *__restrict dst,
        _In_z_            char const *__restrict src,
        _In_              size_t const           size) noexcept
{
#ifdef HAVE_STRLCPY
      return ::strlcpy(dst, src, size);
#else
      return ::emlsp_strlcpy(dst, src, size);
#endif
}


template <_In_ size_t N>
ND __attr_access((__write_only__, 1)) __attribute__((__artificial__, __nonnull__))
__forceinline size_t
strlcpy(_Out_z_cap_(N) char (&dst)[N],
        _In_z_         char const *__restrict src) noexcept
{
#if defined HAVE_STRLCPY
      return ::strlcpy(dst, src, N);
#elif defined HAVE_STRCPY_S && false  // Apparently strcpy_s doesn't work like strlcpy
      return strcpy_s(dst, src);
#else
      return ::emlsp_strlcpy(dst, src, N);
#endif
}


template <typename T>
ND __attribute__((__const__, __artificial__, __nonnull__))
__forceinline constexpr uintptr_t
ptr_diff(_Notnull_ T const *ptr1, _Notnull_ T const *ptr2) noexcept
{
      return static_cast<uintptr_t>(reinterpret_cast<ptrdiff_t>(ptr1) -
                                    reinterpret_cast<ptrdiff_t>(ptr2));
}


template <typename Elem, size_t N>
      requires concepts::Integral<Elem>
__forceinline size_t
fwrite(Elem const (&buffer)[N], FILE *dst)
{
      return ::fwrite(buffer, sizeof(Elem), N - SIZE_C(1), dst);
}


template <typename Cont>
      requires (concepts::NonTrivial<Cont> &&
                concepts::Integral<typename Cont::value_type>)
__forceinline size_t
fwrite(Cont const &container, FILE *dst)
{
      return ::fwrite(container.data(), sizeof(typename Cont::value_type),
                      container.size(), dst);
}


namespace data {
extern FILE *const            stderr_file;
extern std::string_view const projstr;
} // namespace data


template <typename S, typename... Args>
    //requires concepts::is_compiled_string_c<S>
NOINLINE void
eprint(S const &format_str, Args const &...args)
{
      using data::projstr, data::stderr_file;

#ifdef _WIN32
# define LOCK_FILE(x)   _lock_file(x)
# define UNLOCK_FILE(x) _unlock_file(x)
#else
# define LOCK_FILE(x)   flockfile(x)
# define UNLOCK_FILE(x) funlockfile(x)
#endif

      LOCK_FILE(stderr_file);

      try {
            auto const tmp = fmt::format(std::forward<S const &&>(format_str),
                                         std::forward<Args const &&>(args)...);
            if (constants::coloured_stderr)
                  fwrite(projstr, stderr_file);
            else
                  fwrite(MAIN_PROJECT_NAME " (warning):  ", stderr_file);

            fwrite(tmp, stderr_file);

            if (constants::coloured_stderr)
                  fwrite("\033[0m", stderr_file);

            fflush(stderr_file);
      } catch (...) {
            UNLOCK_FILE(stderr_file);
            throw;
      }

      UNLOCK_FILE(data::stderr_file);
#undef LOCK_FILE
#undef UNLOCK_FILE
}


/*======================================================================================*/


__forceinline constexpr void
timespec_add(timespec       *const __restrict lval,
             timespec const *const __restrict rval)
{
      lval->tv_nsec += rval->tv_nsec;
      lval->tv_sec  += rval->tv_sec;

      if (lval->tv_nsec >= INT64_C(1000000000)) {
            ++lval->tv_sec;
            lval->tv_nsec -= INT64_C(1000000000);
      }
}


template <typename Rep, typename Period>
__forceinline constexpr timespec
duration_to_timespec(std::chrono::duration<Rep, Period> const &dur)
{
      timespec dur_ts;  //NOLINT(cppcoreguidelines-pro-type-member-init)
      auto const secs  = std::chrono::duration_cast<std::chrono::seconds>(dur);
      auto const nsecs = std::chrono::duration_cast<std::chrono::nanoseconds>(dur) -
                         std::chrono::duration_cast<std::chrono::nanoseconds>(secs);
      dur_ts.tv_sec  = secs.count();
      dur_ts.tv_nsec = nsecs.count();
      return dur_ts;
}


__forceinline constexpr std::chrono::nanoseconds
timespec_to_duration(timespec const *dur_ts)
{
      return std::chrono::seconds(dur_ts->tv_sec) +
             std::chrono::nanoseconds(dur_ts->tv_nsec);
}


constexpr auto &operator+=(timespec &lval, timespec const &rval)
{
      timespec_add(&lval, &rval);
      return lval;
}

constexpr auto &operator+=(timespec &lval, timespec const *const rval)
{
      timespec_add(&lval, rval);
      return lval;
}

template <typename Rep, typename Period>
constexpr auto &operator+=(timespec &lval, std::chrono::duration<Rep, Period> const &rval)
{
      auto const ts = duration_to_timespec(rval);
      timespec_add(&lval, &ts);
      return lval;
}


/*======================================================================================*/


template <typename>
inline constexpr bool always_false = false;


namespace impl {


# if (defined __GNUC__ || defined __clang__ || defined __INTEL_COMPILER) || \
     (defined __has_builtin && __has_builtin(__builtin_bswap16) &&          \
      __has_builtin(__builtin_bswap32) && __has_builtin(__builtin_bswap64))
ND inline uint16_t bswap_native_16(uint16_t const val) { return __builtin_bswap16(val); }
ND inline uint32_t bswap_native_32(uint32_t const val) { return __builtin_bswap32(val); }
ND inline uint64_t bswap_native_64(uint64_t const val) { return __builtin_bswap64(val); }
# elif defined _MSC_VER
ND inline uint16_t bswap_native_16(uint16_t const val) { return _byteswap_ushort(val); }
ND inline uint32_t bswap_native_32(uint32_t const val) { return _byteswap_ulong(val); }
ND inline uint64_t bswap_native_64(uint64_t const val) { return _byteswap_uint64(val); }
# else
#  define NO_bswap_SUPPORT
# endif

ND constexpr uint16_t bswap_16(uint16_t const val) noexcept {
# ifndef NO_bswap_SUPPORT
      if (std::is_constant_evaluated())
# endif
            return static_cast<unsigned short>((val << 8) | (val >> 8));
# ifndef NO_bswap_SUPPORT
      else
            return bswap_native_16(val);
# endif
}

ND constexpr uint32_t bswap_32(uint32_t const val) noexcept
{
# ifndef NO_bswap_SUPPORT
      if (std::is_constant_evaluated())
# endif
            return (val << 24) | ((val << 8) & 0x00FF'0000) | ((val >> 8) & 0x0000'FF00) | (val >> 24);
# ifndef NO_bswap_SUPPORT
      else
            return bswap_native_32(val);
# endif
}

ND constexpr uint64_t bswap_64(uint64_t const val) noexcept {
# ifndef NO_bswap_SUPPORT
      if (std::is_constant_evaluated())
# endif
            return (val << 56) |
                   ((val << 40) & 0x00FF'0000'0000'0000) |
                   ((val << 24) & 0x0000'FF00'0000'0000) |
                   ((val << 8) & 0x0000'00FF'0000'0000) |
                   ((val >> 8) & 0x0000'0000'FF00'0000) |
                   ((val >> 24) & 0x0000'0000'00FF'0000) |
                   ((val >> 40) & 0x0000'0000'0000'FF00) |
                   (val >> 56);
# ifndef NO_bswap_SUPPORT
      else
            return bswap_native_64(val);
# endif
}


} // namespace impl


template <typename T>
    requires std::is_integral_v<T>
ND constexpr T bswap(T const val) noexcept
{
      if constexpr (sizeof(T) == 1)
            return val;
      else if constexpr (sizeof(T) == 2)
            return static_cast<T>(impl::bswap_16(static_cast<uint16_t>(val)));
      else if constexpr (sizeof(T) == 4)
            return static_cast<T>(impl::bswap_32(static_cast<uint32_t>(val)));
      else if constexpr (sizeof(T) == 8)
            return static_cast<T>(impl::bswap_64(static_cast<uint64_t>(val)));
      else 
            static_assert(always_false<T>, "Unexpected integer size");

      /* NOTREACHED */
      return -1;
}

template <typename T>
ND constexpr T hton(T const val) noexcept
{
      if constexpr (std::endian::native == std::endian::little)
            return bswap(val);
      else
            return val;
}

template <typename T>
ND constexpr T ntoh(T const val) noexcept
{
      if constexpr (std::endian::native == std::endian::little)
            return bswap(val);
      else
            return val;
}


/*======================================================================================*/


extern std::string_view const &get_signal_name(int signum);
extern std::string_view const &get_signal_explanation(int signum);


#ifdef _WIN32
namespace win32 {

void warning_box_explicit(_In_z_ wchar_t const *msg, _In_ DWORD errval);
void warning_box_message (_In_z_ wchar_t const *msg);
void warning_box_explicit(_In_z_ char const *msg, _In_ DWORD errval);
void warning_box_message (_In_z_ char const *msg);

NORETURN _Analysis_noreturn_ void error_exit_explicit(_In_z_ wchar_t const *msg, _In_ DWORD errval);
NORETURN _Analysis_noreturn_ void error_exit_message (_In_z_ wchar_t const *msg);
NORETURN _Analysis_noreturn_ void error_exit_explicit(_In_z_ char const *msg, _In_ DWORD errval);
NORETURN _Analysis_noreturn_ void error_exit_message (_In_z_ char const *msg);

NORETURN _Analysis_noreturn_ __forceinline void error_exit        (wchar_t const *msg)      { error_exit_explicit(msg,         GetLastError());    }
NORETURN _Analysis_noreturn_ __forceinline void error_exit_wsa    (wchar_t const *msg)      { error_exit_explicit(msg,         WSAGetLastError()); }
NORETURN _Analysis_noreturn_ __forceinline void error_exit_errno  (wchar_t const *msg)      { error_exit_explicit(msg,         errno);             }
NORETURN _Analysis_noreturn_ __forceinline void error_exit        (std::wstring const &msg) { error_exit_explicit(msg.c_str(), GetLastError());    }
NORETURN _Analysis_noreturn_ __forceinline void error_exit_wsa    (std::wstring const &msg) { error_exit_explicit(msg.c_str(), WSAGetLastError()); }
NORETURN _Analysis_noreturn_ __forceinline void error_exit_errno  (std::wstring const &msg) { error_exit_explicit(msg.c_str(), errno);             }
NORETURN _Analysis_noreturn_ __forceinline void error_exit_message(std::wstring const &msg) { error_exit_message (msg.c_str());                    }

NORETURN _Analysis_noreturn_ __forceinline void error_exit        (char const *msg)        { error_exit_explicit(msg,         GetLastError());    }
NORETURN _Analysis_noreturn_ __forceinline void error_exit_wsa    (char const *msg)        { error_exit_explicit(msg,         WSAGetLastError()); }
NORETURN _Analysis_noreturn_ __forceinline void error_exit_errno  (char const *msg)        { error_exit_explicit(msg,         errno);             }
NORETURN _Analysis_noreturn_ __forceinline void error_exit        (std::string const &msg) { error_exit_explicit(msg.c_str(), GetLastError());    }
NORETURN _Analysis_noreturn_ __forceinline void error_exit_wsa    (std::string const &msg) { error_exit_explicit(msg.c_str(), WSAGetLastError()); }
NORETURN _Analysis_noreturn_ __forceinline void error_exit_errno  (std::string const &msg) { error_exit_explicit(msg.c_str(), errno);             }
NORETURN _Analysis_noreturn_ __forceinline void error_exit_message(std::string const &msg) { error_exit_message (msg.c_str());                    }

extern BOOL WINAPI myCtrlHandler(DWORD type) noexcept;

template <typename Proc = ::FARPROC>
Proc get_proc_address_module(LPCWSTR const module_name, LPCSTR const proc_name)
{
      auto *mod_hand = ::GetModuleHandleW(module_name);
      if (!mod_hand || mod_hand == INVALID_HANDLE_VALUE)
            error_exit(L"GetModuleHandleW()");
      // NOLINTNEXTLINE(clang-diagnostic-cast-function-type)
      auto *routine = reinterpret_cast<Proc>(::GetProcAddress(mod_hand, proc_name));
      if (!routine)
            error_exit(L"GetProcAddress()");
      return routine;
}
} // namespace win32
#endif

/****************************************************************************************/
} // namespace util

using util::operator+=;

} // namespace MAIN_PACKAGE_NAMESPACE


#ifndef _WIN32
using MAIN_PACKAGE_NAMESPACE::util::putenv;  //NOLINT(google-global-names-in-headers)
#endif

#include "util/formatters.hh"


/****************************************************************************************/
#endif
// vim: ft=cpp
