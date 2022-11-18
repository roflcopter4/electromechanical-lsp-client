// ReSharper disable CppTooWideScopeInitStatement
// ReSharper disable CppClangTidyReadabilitySimplifySubscriptExpr
#include "Common.hh"
#include "recode.hh"

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

static int64_t
UTF16_to_UTF8_size(_In_z_ WCHAR const *unicode_string, size_t const len)
/*++
Routine Description:
    This routine will return the length needed to represent the unicode
    string as ANSI
Arguments:
    UnicodeString is the unicode string whose ansi length is returned
    *AnsiSizeInBytes is number of bytes needed to represent unicode
        string as ANSI
Return Value:
    ERROR_SUCCESS or error code
--*/
{
      auto const size = ::WideCharToMultiByte(CP_UTF8, 0, unicode_string,
                                              static_cast<int>(len + 1),
                                              nullptr, 0, nullptr, nullptr);

      return size > 0 ? size : -static_cast<int64_t>(GetLastError());
}

static int64_t
UTF16_to_UTF8_size(_In_z_ char16_t const *unicode_string, size_t const len)
{
      return UTF16_to_UTF8_size(reinterpret_cast<WCHAR const *>(unicode_string), len);
}

} // namespace detail


namespace impl {


NORETURN static void
conversion_error(unsigned const e, int const from, int const to, char const *sig, int const line)
{
      char errbuf[128];
      auto const *eptr = my_strerror(e, errbuf, sizeof errbuf);

#ifndef NDEBUG
      //PSNIP_TRAP();
#endif

      /* This ought to be evaluated to a constant string at compile time. */
      auto const foo = fmt::format(("Failed to convert UTF{} string to UTF{} ({} -> {}) at ({} in {})"), from, to, e, eptr, line, sig);
      std::cerr << foo << '\n';
      std::cerr.flush();
      throw std::runtime_error(foo);
}


#if defined _WIN32 && defined EMLSP_USE_WIN32_STR_CONVERSION_FUNCS
struct use_win32_errcode_tag {};

NORETURN static void
conversion_error(use_win32_errcode_tag, int const from, int const to, char const *sig, int const line)
{
      auto const e = GetLastError();
      auto const ecode = std::error_code{static_cast<int>(e),
                                         std::system_category()};

#ifndef NDEBUG
      //PSNIP_TRAP();
#endif
      /* This ought to be evaluated to a constant string at compile time. */
      throw std::runtime_error(
          fmt::format(FC("Failed to convert UTF{} string to UTF{} ({} -> {}) at ({} in {})"),
                      from, to, e, ecode.message(), line, sig));
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

#define conversion_error(...) (conversion_error)(__VA_ARGS__, __FUNCTION__, __LINE__)

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

      for (bool again = true; again;) {
            str.reserve(resultlen + SIZE_C(1));
            size_t true_resultlen = resultlen;
            errno = 0;
            AUTOC *result = u16_to_u8(reinterpret_cast<uint16_t const *>(ws), len,
                                      reinterpret_cast<uint8_t *>(str.data()), &true_resultlen);
            if (!result || errno)
                  conversion_error(errno, 16, 8);
            if (resultlen >= true_resultlen)
                  again = false;
            resultlen = true_resultlen;
      }

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

      for (bool again = true; again;) {
            str.reserve(resultlen + SIZE_C(1));
            size_t true_resultlen = resultlen;
            errno = 0;
            AUTOC *result = u32_to_u8(reinterpret_cast<uint32_t const *>(ws), len,
                                      reinterpret_cast<uint8_t *>(str.data()), &true_resultlen);
            if (!result || errno)
                  conversion_error(errno, 32, 8);
            if (resultlen >= true_resultlen)
                  again = false;
            resultlen = true_resultlen;
      }

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
      size_t      resultlen;

#if defined _WIN32 && defined EMLSP_USE_WIN32_STR_CONVERSION_FUNCS
      {
            resultlen = detail::UTF16_to_UTF8_size(ws, len) - 1UL;
            if (int64_t(resultlen) < 0)
                  win32::error_exit_explicit("WideCharToMultiByte", DWORD(-int64_t(resultlen)));
            str.reserve(resultlen + SIZE_C(1));

            int result = ::WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
                                               reinterpret_cast<LPCWSTR>(ws),
                                               static_cast<int>(len + 1),
                                               str.data(),
                                               static_cast<int>(resultlen + 1),
                                               nullptr, nullptr);
            if (result == 0)
                  conversion_error(use_win32_errcode_tag{}, 8, 16);
      }
#else
      resultlen = (len + SIZE_C(1)) * SIZE_C(2);
      for (bool again = true; again;) {
            str.reserve(resultlen + SIZE_C(1));
            size_t true_resultlen = resultlen;
            errno = 0;
            AUTOC *result = u16_to_u8(reinterpret_cast<uint16_t const *>(ws), len,
                                      reinterpret_cast<uint8_t *>(str.data()),
                                      &true_resultlen);
            if (!result || errno)
                  conversion_error(errno, 16, 8);
            if (resultlen >= true_resultlen)
                  again = false;
            resultlen = true_resultlen;
      }

      str.data()[resultlen] = '\0';
#endif

      resize_string_hack(str, resultlen);
      return str;
}

std::string
char32_to_char(char32_t const *ws, size_t const len)
{
      std::string str;
      size_t      resultlen = (len + SIZE_C(1)) * SIZE_C(4);

      for (bool again = true; again;) {
            str.reserve(resultlen + SIZE_C(1));
            size_t true_resultlen = resultlen;
            errno = 0;
            AUTOC *result = u32_to_u8(reinterpret_cast<uint32_t const *>(ws), len,
                                      reinterpret_cast<uint8_t *>(str.data()),
                                      &true_resultlen);
            if (!result || errno)
                  conversion_error(errno, 32, 8);
            if (resultlen >= true_resultlen)
                  again = false;
            resultlen = true_resultlen;
      }

      str.data()[resultlen] = 0;
      resize_string_hack(str, resultlen);
      return str;
}

std::wstring
char8_to_wide(char8_t const *ws, size_t const len)
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
      size_t      resultlen = len;
      str.reserve(resultlen + 1);

#if defined _WIN32 && defined EMLSP_USE_WIN32_STR_CONVERSION_FUNCS
      resultlen = ::MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
                                        reinterpret_cast<LPCSTR>(ws),
                                        static_cast<int>(len + 1),
                                        str.data(),
                                        static_cast<int>(resultlen + 1));
      if (resultlen == 0)
            conversion_error(use_win32_errcode_tag{}, 8, 16);
#else
      AUTOC *result = func(reinterpret_cast<uint8_t const *>(ws), len,
                           reinterpret_cast<uint_type *>(str.data()), &resultlen);
      if (!result)
            conversion_error(errno, 8, numbits);
      str.data()[resultlen] = '\0';
#endif

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

} // namespace impl


/****************************************************************************************/
/* recode is the wrapper for all this junk */
/****************************************************************************************/


/*--------------------------------------------------------------------------------------*/
/* The fundamental types: char and wchar_t. */

template <> std::string
recode<char, wchar_t>(wchar_t const *orig, size_t const length)
{
#ifdef WCHAR_IS_U16
      return impl::char16_to_char(reinterpret_cast<char16_t const *>(orig), length);
#else
      return impl::char32_to_char(reinterpret_cast<char32_t const *>(orig), length);
#endif
}

template <> std::wstring
recode<wchar_t, char>(char const *orig, size_t const length)
{
      return impl::char8_to_wide(reinterpret_cast<char8_t const *>(orig), length);
}

/*--------------------------------------------------------------------------------------*/
/* Conversions to and from char (excluding wchar_t). */

template <> std::string
recode<char, char8_t>(char8_t const *orig, size_t const length)
{
      return std::string{reinterpret_cast<char const *>(orig), length};
}

template <> std::string
recode<char, char16_t>(char16_t const *orig, size_t const length)
{
      return impl::char16_to_char(orig, length);
}

template <> std::string
recode<char, char32_t>(char32_t const *orig, size_t const length)
{
      return impl::char32_to_char(orig, length);
}

template <> std::u8string
recode<char8_t, char>(char const *orig, size_t const length)
{
      return std::u8string{reinterpret_cast<char8_t const *>(orig), length};
}

template <> std::u16string
recode<char16_t, char>(char const *orig, size_t const length)
{
      return impl::char8_to_char16(reinterpret_cast<char8_t const *>(orig), length);
}

template <> std::u32string
recode<char32_t, char>(char const *orig, size_t const length)
{
      return impl::char8_to_char32(reinterpret_cast<char8_t const *>(orig), length);
}

/*--------------------------------------------------------------------------------------*/
/* Conversions to and from wchar_t (excluding char). */

template <> std::wstring
recode<wchar_t, char8_t>(char8_t const *orig, size_t const length)
{
      return impl::char8_to_wide(orig, length);
}

template <> std::wstring
recode<wchar_t, char16_t>(char16_t const *orig, size_t const length)
{
      return impl::char16_to_wide(orig, length);
}

template <> std::wstring
recode<wchar_t, char32_t>(char32_t const *orig, size_t const length)
{
      return impl::char32_to_wide(orig, length);
}

template <> std::u8string
recode<char8_t, wchar_t>(wchar_t const *orig, size_t const length)
{
#ifdef WCHAR_IS_U16
      return impl::char16_to_char8(reinterpret_cast<char16_t const *>(orig), length);
#else
      return impl::char32_to_char8(reinterpret_cast<char32_t const *>(orig), length);
#endif
}

template <> std::u16string
recode<char16_t, wchar_t>(wchar_t const *orig, size_t const length)
{
#ifdef WCHAR_IS_U16
      return std::u16string{reinterpret_cast<char16_t const *>(orig), length};
#else
      return impl::char32_to_char16(reinterpret_cast<char32_t const *>(orig), length);
#endif
}

template <> std::u32string
recode<char32_t, wchar_t>(wchar_t const *orig, size_t const length)
{
#ifdef WCHAR_IS_U16
      return impl::char16_to_char32(reinterpret_cast<char16_t const *>(orig), length);
#else
      return std::u32string{reinterpret_cast<char32_t const *>(orig), length};
#endif
}

/*--------------------------------------------------------------------------------------*/
/* The rest. */

template <> std::u16string
recode<char16_t, char8_t>(char8_t const *orig, size_t const length)
{
      return impl::char8_to_char16(orig, length);
}

template <> std::u32string
recode<char32_t, char8_t>(char8_t const *orig, size_t const length)
{
      return impl::char8_to_char32(orig, length);
}

template <> std::u8string
recode<char8_t, char16_t>(char16_t const *orig, size_t const length)
{
      return impl::char16_to_char8(orig, length);
}

template <> std::u32string
recode<char32_t, char16_t>(char16_t const *orig, size_t const length)
{
      return impl::char16_to_char32(orig, length);
}

template <> std::u16string
recode<char16_t, char32_t>(char32_t const *orig, size_t const length)
{
      return impl::char32_to_char16(orig, length);
}

template <> std::u8string
recode<char8_t, char32_t>(char32_t const *orig, size_t const length)
{
      return impl::char32_to_char8(orig, length);
}

/*--------------------------------------------------------------------------------------*/
/* What the hell; why not. */

template <> std::string
recode<char, char>(char const *orig, size_t const length)
{
      return std::string{orig, length};
}

template <> std::wstring
recode<wchar_t, wchar_t>(wchar_t const *orig, size_t const length)
{
      return std::wstring{orig, length};
}

template <> std::u8string
recode<char8_t, char8_t>(char8_t const *orig, size_t const length)
{
      return std::u8string{orig, length};
}

template <> std::u16string
recode<char16_t, char16_t>(char16_t const *orig, size_t const length)
{
      return std::u16string{orig, length};
}

template <> std::u32string
recode<char32_t, char32_t>(char32_t const *orig, size_t const length)
{
      return std::u32string{orig, length};
}


/****************************************************************************************/
} // namespace util::unistring
} // namespace emlsp
