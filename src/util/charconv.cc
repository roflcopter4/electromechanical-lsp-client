// ReSharper disable CppTooWideScopeInitStatement
#include "Common.hh"
#include "charconv.hh"

#include <unistr.h>
#undef uint8_t
#undef uint16_t
#undef uint32_t

#define AUTOC auto const

inline namespace emlsp {
namespace util::unistring {
/****************************************************************************************/

namespace detail {


NORETURN static void
conversion_error(errno_t const e, int const from, int const to)
{
      char errbuf[128];
      auto eptr = my_strerror(e, errbuf, 128);

      /* This ought to be evaluated to a constant string at compile time. */
      throw std::runtime_error(
          fmt::format(FC("Failed to convert UTF{} string to UTF{} ({} -> {})"),
                      from, to, e, eptr));
}


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
char8_to_char16(char8_t const *ws, size_t const len)
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
char8_to_char32(char8_t const *ws, size_t const len)
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
char16_to_char8(char16_t const *ws, size_t const len)
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
char16_to_char32(char16_t const *ws, size_t const len)
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
char32_to_char8(char32_t const *ws, size_t const len)
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
char32_to_char16(char32_t const *ws, size_t const len)
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
char16_to_char(char16_t const *ws, size_t const len)
{
      std::string str;
      size_t      resultlen = (len + SIZE_C(1)) * SIZE_C(2);

      str.reserve(resultlen);
      AUTOC *result = u16_to_u8(reinterpret_cast<uint16_t const *>(ws), len,
                                reinterpret_cast<uint8_t *>(str.data()), &resultlen);
      if (!result)
            conversion_error(errno, 16, 8);

      str.data()[resultlen] = 0;
      resize_string_hack(str, resultlen);
      return str;
}

std::string
char32_to_char(char32_t const *ws, size_t const len)
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
char8_to_wide(char8_t const *ws, size_t const len)
{
#if defined WCHAR_IS_U16
      using uint_type = uint16_t;
      static constexpr int numbits = 16;
#elif defined WCHAR_IS_U32
      using uint_type = uint32_t;
      static constexpr int numbits = 32:
#endif

      std::wstring str;
      size_t      resultlen = len + SIZE_C(1);
      str.reserve(resultlen);

      AUTOC *result = u8_to_u16(reinterpret_cast<uint8_t const *>(ws), len,
                                reinterpret_cast<uint_type *>(str.data()), &resultlen);
      if (!result)
            conversion_error(errno, 8, numbits);

      str.data()[resultlen] = 0;
      resize_string_hack(str, resultlen);
      return str;
}

std::wstring
char16_to_wide(char16_t const *ws, size_t const len)
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
char32_to_wide(char32_t const *ws, size_t const len)
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


/*--------------------------------------------------------------------------------------*/
/* The fundemental types: char and wchar_t. */

template <> std::string
charconv<char, wchar_t>(wchar_t const *orig, size_t const codepoints)
{
#ifdef WCHAR_IS_U16
      return detail::char16_to_char(reinterpret_cast<char16_t const *>(orig), codepoints);
#else
      return detail::char32_to_char(reinterpret_cast<char32_t const *>(orig), codepoints);
#endif
}

template <> std::wstring
charconv<wchar_t, char>(char const *orig, size_t const codepoints)
{
      return detail::char8_to_wide(reinterpret_cast<char8_t const *>(orig), codepoints);
}

/*--------------------------------------------------------------------------------------*/
/* Conversions to and from char (excluding wchar_t). */

template <> std::string
charconv<char, char8_t>(char8_t const *orig, size_t const codepoints)
{
      return std::string{reinterpret_cast<char const *>(orig), codepoints};
}

template <> std::string
charconv<char, char16_t>(char16_t const *orig, size_t const codepoints)
{
      return detail::char16_to_char(orig, codepoints);
}

template <> std::string
charconv<char, char32_t>(char32_t const *orig, size_t const codepoints)
{
      return detail::char32_to_char(orig, codepoints);
}

template <> std::u8string
charconv<char8_t, char>(char const *orig, size_t const codepoints)
{
      return std::u8string{reinterpret_cast<char8_t const *>(orig), codepoints};
}

template <> std::u16string
charconv<char16_t, char>(char const *orig, size_t const codepoints)
{
      return detail::char8_to_char16(reinterpret_cast<char8_t const *>(orig), codepoints);
}

template <> std::u32string
charconv<char32_t, char>(char const *orig, size_t const codepoints)
{
      return detail::char8_to_char32(reinterpret_cast<char8_t const *>(orig), codepoints);
}

/*--------------------------------------------------------------------------------------*/
/* Conversions to and from wchar_t (excluding char). */

template <> std::wstring
charconv<wchar_t, char8_t>(char8_t const *orig, size_t const codepoints)
{
      return detail::char8_to_wide(orig, codepoints);
}

template <> std::wstring
charconv<wchar_t, char16_t>(char16_t const *orig, size_t const codepoints)
{
      return detail::char16_to_wide(orig, codepoints);
}

template <> std::wstring
charconv<wchar_t, char32_t>(char32_t const *orig, size_t const codepoints)
{
      return detail::char32_to_wide(orig, codepoints);
}

template <> std::u8string
charconv<char8_t, wchar_t>(wchar_t const *orig, size_t const codepoints)
{
#ifdef WCHAR_IS_U16
      return detail::char16_to_char8(reinterpret_cast<char16_t const *>(orig), codepoints);
#else
      return detail::char32_to_char8(reinterpret_cast<char32_t const *>(orig), codepoints);
#endif
}

template <> std::u16string
charconv<char16_t, wchar_t>(wchar_t const *orig, size_t const codepoints)
{
#ifdef WCHAR_IS_U16
      return std::u16string{reinterpret_cast<char16_t const *>(orig), codepoints};
#else
      return detail::char32_to_char16(reinterpret_cast<char32_t const *>(orig), codepoints);
#endif
}

template <> std::u32string
charconv<char32_t, wchar_t>(wchar_t const *orig, size_t const codepoints)
{
#ifdef WCHAR_IS_U16
      return detail::char16_to_char32(reinterpret_cast<char16_t const *>(orig), codepoints);
#else
      return std::u32string{reinterpret_cast<char32_t const *>(orig), codepoints};
#endif
}

/*--------------------------------------------------------------------------------------*/
/* The rest. */

template <> std::u16string
charconv<char16_t, char8_t>(char8_t const *orig, size_t const codepoints)
{
      return detail::char8_to_char16(orig, codepoints);
}

template <> std::u32string
charconv<char32_t, char8_t>(char8_t const *orig, size_t const codepoints)
{
      return detail::char8_to_char32(orig, codepoints);
}

template <> std::u8string
charconv<char8_t, char16_t>(char16_t const *orig, size_t const codepoints)
{
      return detail::char16_to_char8(orig, codepoints);
}

template <> std::u32string
charconv<char32_t, char16_t>(char16_t const *orig, size_t const codepoints)
{
      return detail::char16_to_char32(orig, codepoints);
}

template <> std::u16string
charconv<char16_t, char32_t>(char32_t const *orig, size_t const codepoints)
{
      return detail::char32_to_char16(orig, codepoints);
}

template <> std::u8string
charconv<char8_t, char32_t>(char32_t const *orig, size_t const codepoints)
{
      return detail::char32_to_char8(orig, codepoints);
}

/*--------------------------------------------------------------------------------------*/
/* What the hell; why not. */

template <> std::string
charconv<char, char>(char const *orig, size_t const codepoints)
{
      return std::string{orig, codepoints};
}

template <> std::wstring
charconv<wchar_t, wchar_t>(wchar_t const *orig, size_t const codepoints)
{
      return std::wstring{orig, codepoints};
}

template <> std::u8string
charconv<char8_t, char8_t>(char8_t const *orig, size_t const codepoints)
{
      return std::u8string{orig, codepoints};
}

template <> std::u16string
charconv<char16_t, char16_t>(char16_t const *orig, size_t const codepoints)
{
      return std::u16string{orig, codepoints};
}

template <> std::u32string
charconv<char32_t, char32_t>(char32_t const *orig, size_t const codepoints)
{
      return std::u32string{orig, codepoints};
}


/****************************************************************************************/
} // namespace util::unistring
} // namespace emlsp
