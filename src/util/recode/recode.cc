// ReSharper disable CppTooWideScopeInitStatement
#include "Common.hh"
#include "recode.hh"

#include "util/debug_trap.h"

#include <unistr.h>

#undef uint8_t
#undef uint16_t
#undef uint32_t

#define AUTOC auto const

#if WCHAR_MAX == INT32_MAX || WCHAR_MAX == UINT32_MAX
#  define WCHAR_IS_U32
#elif WCHAR_MAX == INT16_MAX || WCHAR_MAX == UINT16_MAX
#  define WCHAR_IS_U16
#else
# error "Impossible?!"
#endif

inline namespace emlsp {
namespace util::unistring {
/****************************************************************************************/

namespace detail {


NORETURN static void
conversion_error(errno_t const e, int const from, int const to) noexcept(false)
{
      char errbuf[128];
      auto const *eptr = my_strerror(e, errbuf, sizeof errbuf);

#ifndef NDEBUG
      PSNIP_TRAP();
#endif
      /* This ought to be evaluated to a constant string at compile time. */
      throw std::runtime_error(
          fmt::format(FC("Failed to convert UTF{} string to UTF{} ({} -> {})"),
                      from, to, e, eptr));
}


#if defined _WIN32 && defined EMLSP_USE_WIN32_STR_CONVERSION_FUNCS
struct use_win32_errcode_tag {};

NORETURN static void
conversion_error(use_win32_errcode_tag, int const from, int const to) noexcept(false)
{
      auto const e = GetLastError();
      auto const ecode = std::error_code{static_cast<int>(e),
                                         std::system_category()};

#ifndef NDEBUG
      PSNIP_TRAP();
#endif
      /* This ought to be evaluated to a constant string at compile time. */
      throw std::runtime_error(
          fmt::format(FC("Failed to convert UTF{} string to UTF{} ({} -> {})"),
                      from, to, e, ecode.message()));
}
#endif


#define ABSURDLY_EVIL_MACRO_OF_DOOM(FROM, TO)                                              \
std::u##TO##string char##FROM##_to_char##TO(char##FROM##_t const *ws, size_t const len)    \
{                                                                                          \
      static constexpr size_t size_ratio = std::max(SIZE_C(FROM) / SIZE_C(TO), SIZE_C(1)); \
      std::u##TO##string str;                                                              \
      size_t        resultlen = (len + SIZE_C(1)) * size_ratio;                            \
      str.reserve(resultlen);                                                              \
                                                                                           \
      AUTOC *result = u##FROM##_to_u##TO(                                                  \
            reinterpret_cast<uint##FROM##_t const *>(ws), len,                             \
            reinterpret_cast<uint##TO##_t *>(str.data()), &resultlen);                     \
      if (!result)                                                                         \
            conversion_error(errno, FROM, TO);                                             \
                                                                                           \
      str.data()[resultlen] = 0;                                                           \
      resize_string_hack(str, resultlen);                                                  \
      return str;                                                                          \
}

/*--------------------------------------------------------------------------------------*/

std::u16string
char8_to_char16(char8_t const *ws, size_t const len) noexcept(false)
{
      std::u16string str;
      size_t         resultlen = len + SIZE_C(1);
      str.reserve(resultlen);

      AUTOC *result = u8_to_u16(reinterpret_cast<uint8_t const *>(ws), len,
                                reinterpret_cast<uint16_t *>(str.data()), &resultlen);
      if (!result)
            conversion_error(errno, 8, 16);

      str.data()[resultlen] = 0;
      resize_string_hack(str, resultlen);
      return str;
}

std::u32string
char8_to_char32(char8_t const *ws, size_t const len) noexcept(false)
{
      std::u32string str;
      size_t         resultlen = len + SIZE_C(1);
      str.reserve(resultlen);

      AUTOC *result = u8_to_u32(reinterpret_cast<uint8_t const *>(ws), len,
                                reinterpret_cast<uint32_t *>(str.data()), &resultlen);
      if (!result)
            conversion_error(errno, 8, 32);

      str.data()[resultlen] = 0;
      resize_string_hack(str, resultlen);
      return str;
}

std::u8string
char16_to_char8(char16_t const *ws, size_t const len) noexcept(false)
{
      std::u8string str;
      size_t        resultlen = (len + SIZE_C(1)) * SIZE_C(2);
      str.reserve(resultlen);

      AUTOC *result = u16_to_u8(reinterpret_cast<uint16_t const *>(ws), len,
                                reinterpret_cast<uint8_t *>(str.data()), &resultlen);
      if (!result)
            conversion_error(errno, 16, 8);

      str.data()[resultlen] = 0;
      resize_string_hack(str, resultlen);
      return str;
}

std::u32string
char16_to_char32(char16_t const *ws, size_t const len) noexcept(false)
{
      std::u32string str;
      size_t         resultlen = (len + SIZE_C(1));
      str.reserve(resultlen);

      AUTOC *result = u16_to_u32(reinterpret_cast<uint16_t const *>(ws), len,
                                 reinterpret_cast<uint32_t *>(str.data()), &resultlen);
      if (!result)
            conversion_error(errno, 16, 32);

      str.data()[resultlen] = 0;
      resize_string_hack(str, resultlen);
      return str;
}

std::u8string
char32_to_char8(char32_t const *ws, size_t const len) noexcept(false)
{
      std::u8string str;
      size_t        resultlen = (len + SIZE_C(1)) * SIZE_C(4);
      str.reserve(resultlen);

      AUTOC *result = u32_to_u8(reinterpret_cast<uint32_t const *>(ws), len,
                                reinterpret_cast<uint8_t *>(str.data()), &resultlen);
      if (!result)
            conversion_error(errno, 32, 8);

      str.data()[resultlen] = 0;
      resize_string_hack(str, resultlen);
      return str;

}

std::u16string
char32_to_char16(char32_t const *ws, size_t const len) noexcept(false)
{
      std::u16string str;
      size_t         resultlen = (len + SIZE_C(1)) * SIZE_C(2);
      str.reserve(resultlen);

      AUTOC *result = u32_to_u16(reinterpret_cast<uint32_t const *>(ws), len,
                                 reinterpret_cast<uint16_t *>(str.data()), &resultlen);
      if (!result)
            conversion_error(errno, 32, 16);

      str.data()[resultlen] = 0;
      resize_string_hack(str, resultlen);
      return str;
}


/*--------------------------------------------------------------------------------------*/

std::string
char16_to_char(char16_t const *ws, size_t const len) noexcept(false)
{
      std::string str;
      size_t      resultlen = (len + SIZE_C(1)) * SIZE_C(2);
      str.reserve(resultlen);

#if defined _WIN32 && defined EMLSP_USE_WIN32_STR_CONVERSION_FUNCS
      resultlen = WideCharToMultiByte(CP_UTF8, 0, reinterpret_cast<LPCWSTR>(ws),
                                      static_cast<int>(len), str.data(),
                                      static_cast<int>(resultlen), nullptr, nullptr);
      if (resultlen == 0)
            conversion_error(use_win32_errcode_tag{}, 8, 16);
#else
      AUTOC *result = u16_to_u8(reinterpret_cast<uint16_t const *>(ws), len,
                                reinterpret_cast<uint8_t *>(str.data()), &resultlen);
      if (!result)
            conversion_error(errno, 16, 8);
#endif

      str.data()[resultlen] = 0;
      resize_string_hack(str, resultlen);
      return str;
}

std::string
char32_to_char(char32_t const *ws, size_t const len) noexcept(false)
{
      std::string str;
      size_t      resultlen = (len + SIZE_C(1)) * SIZE_C(4);
      str.reserve(resultlen);

      AUTOC *result = u32_to_u8(reinterpret_cast<uint32_t const *>(ws), len,
                                reinterpret_cast<uint8_t *>(str.data()), &resultlen);
      if (!result)
            conversion_error(errno, 32, 8);

      str.data()[resultlen] = 0;
      resize_string_hack(str, resultlen);
      return str;
}

std::wstring
char8_to_wide(char8_t const *ws, size_t const len) noexcept(false)
{
#if defined WCHAR_IS_U16
# ifndef EMLSP_USE_WIN32_STR_CONVERSION_FUNCS
      using uint_type = uint16_t;
      constexpr auto       func    = &::u8_to_u16;
      static constexpr int numbits = 16;
# endif
#elif defined WCHAR_IS_U32
      using uint_type = uint32_t;
      static constexpr int numbits = 32;
      auto const func = &::u8_to_u32;
#endif

      std::wstring str;
      size_t      resultlen = len + SIZE_C(1);
      str.reserve(resultlen);

#if defined _WIN32 && defined EMLSP_USE_WIN32_STR_CONVERSION_FUNCS
      resultlen = MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<LPCSTR>(ws),
                                      static_cast<int>(len), str.data(),
                                      static_cast<int>(resultlen));
      if (resultlen == 0)
            conversion_error(use_win32_errcode_tag{}, 8, 16);
#else
      AUTOC *result = func(reinterpret_cast<uint8_t const *>(ws), len,
                           reinterpret_cast<uint_type *>(str.data()), &resultlen);
      if (!result)
            conversion_error(errno, 8, numbits);
#endif

      str.data()[resultlen] = 0;
      resize_string_hack(str, resultlen);
      return str;
}

std::wstring
char16_to_wide(char16_t const *ws, size_t const len) noexcept(false)
{
#if defined WCHAR_IS_U16
      return {reinterpret_cast<wchar_t const *>(ws), len};
#elif defined WCHAR_IS_U32
      std::wstring str;
      size_t      resultlen = len + SIZE_C(1);
      str.reserve(resultlen);

      AUTOC *result = u16_to_u32(reinterpret_cast<uint16_t const *>(ws), len,
                                 reinterpret_cast<uint32_t *>(str.data()), &resultlen);
      if (!result)
            conversion_error(errno, 16, 32);

      str.data()[resultlen] = 0;
      resize_string_hack(str, resultlen);
      return str;
#else
# error "Impossible"
#endif
}

std::wstring
char32_to_wide(char32_t const *ws, size_t const len) noexcept(false)
{
#if defined WCHAR_IS_U32
      return {reinterpret_cast<wchar_t const *>(ws), len};
#elif defined WCHAR_IS_U16
      std::wstring str;
      size_t       resultlen = (len + SIZE_C(1)) * SIZE_C(2);
      str.reserve(resultlen);

      AUTOC *result = u32_to_u16(reinterpret_cast<uint32_t const *>(ws), len,
                                 reinterpret_cast<uint16_t *>(str.data()), &resultlen);
      if (!result)
            conversion_error(errno, 32, 16);

      str.data()[resultlen] = 0;
      resize_string_hack(str, resultlen);
      return str;
#else
# error "Impossible"
#endif
}

} // namespace detail


/****************************************************************************************/
/* recode is the wrapper for all this junk */
/****************************************************************************************/


/*--------------------------------------------------------------------------------------*/
/* The fundamental types: char and wchar_t. */

template <> std::string
recode<char, wchar_t>(wchar_t const *orig, size_t const length) noexcept(false)
{
#ifdef WCHAR_IS_U16
      return detail::char16_to_char(reinterpret_cast<char16_t const *>(orig), length);
#else
      return detail::char32_to_char(reinterpret_cast<char32_t const *>(orig), length);
#endif
}

template <> std::wstring
recode<wchar_t, char>(char const *orig, size_t const length) noexcept(false)
{
      return detail::char8_to_wide(reinterpret_cast<char8_t const *>(orig), length);
}

/*--------------------------------------------------------------------------------------*/
/* Conversions to and from char (excluding wchar_t). */

template <> std::string
recode<char, char8_t>(char8_t const *orig, size_t const length) noexcept(false)
{
      return std::string{reinterpret_cast<char const *>(orig), length};
}

template <> std::string
recode<char, char16_t>(char16_t const *orig, size_t const length) noexcept(false)
{
      return detail::char16_to_char(orig, length);
}

template <> std::string
recode<char, char32_t>(char32_t const *orig, size_t const length) noexcept(false)
{
      return detail::char32_to_char(orig, length);
}

template <> std::u8string
recode<char8_t, char>(char const *orig, size_t const length) noexcept(false)
{
      return std::u8string{reinterpret_cast<char8_t const *>(orig), length};
}

template <> std::u16string
recode<char16_t, char>(char const *orig, size_t const length) noexcept(false)
{
      return detail::char8_to_char16(reinterpret_cast<char8_t const *>(orig), length);
}

template <> std::u32string
recode<char32_t, char>(char const *orig, size_t const length) noexcept(false)
{
      return detail::char8_to_char32(reinterpret_cast<char8_t const *>(orig), length);
}

/*--------------------------------------------------------------------------------------*/
/* Conversions to and from wchar_t (excluding char). */

template <> std::wstring
recode<wchar_t, char8_t>(char8_t const *orig, size_t const length) noexcept(false)
{
      return detail::char8_to_wide(orig, length);
}

template <> std::wstring
recode<wchar_t, char16_t>(char16_t const *orig, size_t const length) noexcept(false)
{
      return detail::char16_to_wide(orig, length);
}

template <> std::wstring
recode<wchar_t, char32_t>(char32_t const *orig, size_t const length) noexcept(false)
{
      return detail::char32_to_wide(orig, length);
}

template <> std::u8string
recode<char8_t, wchar_t>(wchar_t const *orig, size_t const length) noexcept(false)
{
#ifdef WCHAR_IS_U16
      return detail::char16_to_char8(reinterpret_cast<char16_t const *>(orig), length);
#else
      return detail::char32_to_char8(reinterpret_cast<char32_t const *>(orig), length);
#endif
}

template <> std::u16string
recode<char16_t, wchar_t>(wchar_t const *orig, size_t const length) noexcept(false)
{
#ifdef WCHAR_IS_U16
      return std::u16string{reinterpret_cast<char16_t const *>(orig), length};
#else
      return detail::char32_to_char16(reinterpret_cast<char32_t const *>(orig), length);
#endif
}

template <> std::u32string
recode<char32_t, wchar_t>(wchar_t const *orig, size_t const length) noexcept(false)
{
#ifdef WCHAR_IS_U16
      return detail::char16_to_char32(reinterpret_cast<char16_t const *>(orig), length);
#else
      return std::u32string{reinterpret_cast<char32_t const *>(orig), length};
#endif
}

/*--------------------------------------------------------------------------------------*/
/* The rest. */

template <> std::u16string
recode<char16_t, char8_t>(char8_t const *orig, size_t const length) noexcept(false)
{
      return detail::char8_to_char16(orig, length);
}

template <> std::u32string
recode<char32_t, char8_t>(char8_t const *orig, size_t const length) noexcept(false)
{
      return detail::char8_to_char32(orig, length);
}

template <> std::u8string
recode<char8_t, char16_t>(char16_t const *orig, size_t const length) noexcept(false)
{
      return detail::char16_to_char8(orig, length);
}

template <> std::u32string
recode<char32_t, char16_t>(char16_t const *orig, size_t const length) noexcept(false)
{
      return detail::char16_to_char32(orig, length);
}

template <> std::u16string
recode<char16_t, char32_t>(char32_t const *orig, size_t const length) noexcept(false)
{
      return detail::char32_to_char16(orig, length);
}

template <> std::u8string
recode<char8_t, char32_t>(char32_t const *orig, size_t const length) noexcept(false)
{
      return detail::char32_to_char8(orig, length);
}

/*--------------------------------------------------------------------------------------*/
/* What the hell; why not. */

template <> std::string
recode<char, char>(char const *orig, size_t const length) noexcept(false)
{
      return std::string{orig, length};
}

template <> std::wstring
recode<wchar_t, wchar_t>(wchar_t const *orig, size_t const length) noexcept(false)
{
      return std::wstring{orig, length};
}

template <> std::u8string
recode<char8_t, char8_t>(char8_t const *orig, size_t const length) noexcept(false)
{
      return std::u8string{orig, length};
}

template <> std::u16string
recode<char16_t, char16_t>(char16_t const *orig, size_t const length) noexcept(false)
{
      return std::u16string{orig, length};
}

template <> std::u32string
recode<char32_t, char32_t>(char32_t const *orig, size_t const length) noexcept(false)
{
      return std::u32string{orig, length};
}


/****************************************************************************************/
} // namespace util::unistring
} // namespace emlsp
