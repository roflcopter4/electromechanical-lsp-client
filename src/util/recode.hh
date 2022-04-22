#pragma once
#include "Common.hh"
#include "util/util.hh"
#include <unistr.h>

#if __has_include("cuchar")
#  include <cuchar>
#else
#  include <uchar.h>
#endif

/* "unistr.h" defines these extremely irritating macros. */
#undef true
#undef false
#undef uint8_t
#undef uint16_t
#undef uint32_t
#undef int8_t
#undef int16_t
#undef int32_t
#undef int64_t

//#define FORCE_USE_OF_LIBUNISTRING 1
//#define EMLSP_UNISTRING_NO_WCSLEN
//#define EMLSP_UNISTRING_NO_STRLEN

#if defined(_MSC_VER) || __has_builtin(__builtin_strlen)
#  define _STRLEN(str) __builtin_strlen(str)
#else
#  define _STRLEN(str) ::strlen(str)
#endif
#if defined(_MSC_VER) || __has_builtin(__builtin_wcslen)
#  define _WCSLEN(str) __builtin_wcslen(str)
#else
#  define _WCSLEN(str) ::wcslen(str)
#endif

#if WCHAR_MAX == INT32_MAX || WCHAR_MAX == UINT32_MAX
#  define WCHAR_IS_U32
#elif WCHAR_MAX == INT16_MAX || WCHAR_MAX == UINT16_MAX
#  define WCHAR_IS_U16
#else
# error "Impossible?!"
#endif

inline namespace emlsp {
namespace util {
namespace unistring {
/****************************************************************************************/


#if defined EMLSP_UNISTRING_NO_WCSLEN
ND inline size_t strlen (char    const *str) { return ::u8_strlen(reinterpret_cast<uint8_t const *>(str)); }
ND inline size_t strlen (char8_t const *str) { return ::u8_strlen(reinterpret_cast<uint8_t const *>(str)); }
ND inline size_t strlen (uint8_t const *str) { return ::u8_strlen(str); }
#else
ND inline size_t strlen (char    const *str) { return _STRLEN(str); }
ND inline size_t strlen (char8_t const *str) { return _STRLEN(reinterpret_cast<char const *>(str)); }
ND inline size_t strlen (uint8_t const *str) { return _STRLEN(reinterpret_cast<char const *>(str)); }
#endif

#if defined EMLSP_UNISTRING_NO_WCSLEN
# if defined WCHAR_IS_U16
ND inline size_t strlen(wchar_t const *str) { return ::u16_strlen(reinterpret_cast<uint16_t const *>(str)); }
# elif defined WCHAR_IS_U32
ND inline size_t strlen(wchar_t const *str) { return ::u32_strlen(reinterpret_cast<uint32_t const *>(str)); }
# endif
#else
ND inline size_t strlen(wchar_t const *str) { return _WCSLEN(str); }
#endif

#if defined WCHAR_IS_U16 && !defined EMLSP_UNISTRING_NO_WCSLEN
ND inline size_t strlen(uint16_t const *str) { return _WCSLEN(reinterpret_cast<wchar_t const *>(str)); }
ND inline size_t strlen(char16_t const *str) { return _WCSLEN(reinterpret_cast<wchar_t const *>(str)); }
ND inline size_t strlen(uint32_t const *str) { return ::u32_strlen(str); }
ND inline size_t strlen(char32_t const *str) { return ::u32_strlen(reinterpret_cast<uint32_t const *>(str)); }
#elif defined WCHAR_IS_U32 && !defined EMLSP_UNISTRING_NO_WCSLEN
ND inline size_t strlen(uint16_t const *str) { return ::u16_strlen(str); }
ND inline size_t strlen(char16_t const *str) { return ::u16_strlen(reinterpret_cast<uint16_t const *>(str)); }
ND inline size_t strlen(uint32_t const *str) { return _WCSLEN(reinterpret_cast<wchar_t const *>(str)); }
ND inline size_t strlen(char32_t const *str) { return _WCSLEN(reinterpret_cast<wchar_t const *>(str)); }
#else
ND inline size_t strlen(uint16_t const *str) { return ::u16_strlen(str); }
ND inline size_t strlen(char16_t const *str) { return ::u16_strlen(reinterpret_cast<uint16_t const *>(str)); }
ND inline size_t strlen(uint32_t const *str) { return ::u32_strlen(str); }
ND inline size_t strlen(char32_t const *str) { return ::u32_strlen(reinterpret_cast<uint32_t const *>(str)); }
#endif


/*--------------------------------------------------------------------------------------*/


/**
 * \brief Convert a valid unicode string from any char type to any other.
 * \note - The semi-confusing ordering of template parameters as (To, From) is done to
 * allow the From type to be deduced. In use, one need only specify to which type they
 * want the string to be converted.
 * -# There is no generic implementation of this template. All instantiations are
 * explicit.
 * \tparam To Desired output char type.
 * \tparam From Input char type (typically deduced).
 * \param orig The NUL terminated string to convert.
 * \param length Number of unicode points in the string.
 * \return The appropriate type of std::basic_string.
 */
template <typename To, typename From>
ND std::basic_string<To>
recode(From const *orig, size_t length)  noexcept(false);


template <typename To, typename From, size_t N>
      REQUIRES (concepts::IsIntegral<From>)
ND std::basic_string<To>
recode(From const (&orig)[N])  noexcept(false)
{
      return recode<To, From>(orig, N - SIZE_C(1));
}


template <typename To, class Container>
      REQUIRES (concepts::IsIntegral<typename Container::value_type>)
ND std::basic_string<To>
recode(Container const &orig)  noexcept(false)
{
      return recode<To, typename Container::value_type>(orig.data(), orig.size());
}


template <typename To, typename From>
      REQUIRES (concepts::IsTrivial<From> && concepts::IsPointer<From>)
ND std::basic_string<To>
recode(From const orig)  noexcept(false)
{
      return recode<To, From>(orig, strlen(orig));
}


/*--------------------------------------------------------------------------------------*/


template <> ND std::string    recode<char,     wchar_t>  (wchar_t  const *, size_t) noexcept(false);
template <> ND std::wstring   recode<wchar_t,  char>     (char     const *, size_t) noexcept(false);
template <> ND std::string    recode<char,     char8_t>  (char8_t  const *, size_t) noexcept(false);
template <> ND std::string    recode<char,     char16_t> (char16_t const *, size_t) noexcept(false);
template <> ND std::string    recode<char,     char32_t> (char32_t const *, size_t) noexcept(false);
template <> ND std::u8string  recode<char8_t,  char>     (char     const *, size_t) noexcept(false);
template <> ND std::u16string recode<char16_t, char>     (char     const *, size_t) noexcept(false);
template <> ND std::u32string recode<char32_t, char>     (char     const *, size_t) noexcept(false);
template <> ND std::wstring   recode<wchar_t,  char8_t>  (char8_t  const *, size_t) noexcept(false);
template <> ND std::wstring   recode<wchar_t,  char16_t> (char16_t const *, size_t) noexcept(false);
template <> ND std::wstring   recode<wchar_t,  char32_t> (char32_t const *, size_t) noexcept(false);
template <> ND std::u8string  recode<char8_t,  wchar_t>  (wchar_t  const *, size_t) noexcept(false);
template <> ND std::u16string recode<char16_t, wchar_t>  (wchar_t  const *, size_t) noexcept(false);
template <> ND std::u32string recode<char32_t, wchar_t>  (wchar_t  const *, size_t) noexcept(false);
template <> ND std::u16string recode<char16_t, char8_t>  (char8_t  const *, size_t) noexcept(false);
template <> ND std::u32string recode<char32_t, char8_t>  (char8_t  const *, size_t) noexcept(false);
template <> ND std::u8string  recode<char8_t,  char16_t> (char16_t const *, size_t) noexcept(false);
template <> ND std::u32string recode<char32_t, char16_t> (char16_t const *, size_t) noexcept(false);
template <> ND std::u16string recode<char16_t, char32_t> (char32_t const *, size_t) noexcept(false);
template <> ND std::u8string  recode<char8_t,  char32_t> (char32_t const *, size_t) noexcept(false);
template <> ND std::string    recode<char,     char>     (char     const *, size_t) noexcept(false);
template <> ND std::wstring   recode<wchar_t,  wchar_t>  (wchar_t  const *, size_t) noexcept(false);
template <> ND std::u8string  recode<char8_t,  char8_t>  (char8_t  const *, size_t) noexcept(false);
template <> ND std::u16string recode<char16_t, char16_t> (char16_t const *, size_t) noexcept(false);
template <> ND std::u32string recode<char32_t, char32_t> (char32_t const *, size_t) noexcept(false);


/*--------------------------------------------------------------------------------------*/


namespace impl {

ND inline size_t mbsnlen (char     const *str, size_t size) { return ::u8_mbsnlen(reinterpret_cast<uint8_t const *>(str), size); }
#if defined WCHAR_IS_U16
ND inline size_t mbsnlen (wchar_t  const *str, size_t size) { return ::u16_mbsnlen(reinterpret_cast<uint16_t const *>(str), size); }
#elif defined WCHAR_IS_U32
ND inline size_t mbsnlen (wchar_t  const *str, size_t size) { return ::u32_mbsnlen(reinterpret_cast<uint32_t const *>(str), size); }
#endif

ND inline size_t mbsnlen (uint8_t  const *str, size_t size) { return ::u8_mbsnlen(str, size); }
ND inline size_t mbsnlen (char8_t  const *str, size_t size) { return ::u8_mbsnlen(reinterpret_cast<uint8_t const *>(str), size); }
ND inline size_t mbsnlen (uint16_t const *str, size_t size) { return ::u16_mbsnlen(str, size); }
ND inline size_t mbsnlen (char16_t const *str, size_t size) { return ::u16_mbsnlen(reinterpret_cast<uint16_t const *>(str), size); }
ND inline size_t mbsnlen (uint32_t const *str, size_t size) { return ::u32_mbsnlen(str, size); }
ND inline size_t mbsnlen (char32_t const *str, size_t size) { return ::u32_mbsnlen(reinterpret_cast<uint32_t const *>(str), size); }

} // namespace impl


template <typename To, class Container>
      REQUIRES (concepts::IsIntegral<typename Container::value_type>)
ND size_t
mbsnlen(Container const &orig) noexcept(false)
{
      return impl::mbsnlen(orig.data(), orig.size());
}

template <typename T, size_t N>
ND size_t mbsnlen(T const (&str)[N])
{
      return impl::mbsnlen(str, N - SIZE_C(1));
}

template <typename T>
      REQUIRES (util::concepts::IsPointer<T>)
ND size_t mbsnlen(T const str)
{
      return impl::mbsnlen(str, strlen(str));
}

template <typename T>
ND size_t mbsnlen(T const *str, size_t const size)
{
      return impl::mbsnlen(str, size);
}


} // namespace unistring
/*--------------------------------------------------------------------------------------*/


using unistring::recode;
using unistring::strlen;
using unistring::mbsnlen;

#undef _STRLEN
#undef _WCSLEN
#if defined WCHAR_IS_U16
#  undef WCHAR_IS_U16
#elif defined WCHAR_IS_U32
#  undef WCHAR_IS_U32
#endif


/****************************************************************************************/
} // namespace util
} // namespace emlsp
