#pragma once
#include "Common.hh"
#include <codecvt>
#include <unistr.h>

#if __has_include("cuchar")
#  include <cuchar>
#else
#  include <uchar.h>
#endif
#undef true
#undef false
#undef uint8_t
#undef uint16_t
#undef uint32_t
#undef int8_t
#undef int16_t
#undef int32_t
#undef int64_t

#define FORCE_USE_OF_LIBUNISTRING 1
#define EMLSP_UNISTRING_NO_WCSLEN
#define EMLSP_UNISTRING_NO_STRLEN

#if WCHAR_MAX == INT32_MAX || WCHAR_MAX == UINT32_MAX
#  define WCHAR_IS_U32
#  define SIZEOF_WCHAR (32)
#elif WCHAR_MAX == INT16_MAX || WCHAR_MAX == UINT16_MAX
#  define WCHAR_IS_U16
#  define SIZEOF_WCHAR (16)
#else
# error "Impossible?!"
#endif

inline namespace emlsp {
namespace util::unistring {
/****************************************************************************************/


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
charconv(From const *orig, size_t length) noexcept(false);


template <typename To, class Container>
      REQUIRES(concepts::IsIntegral<typename Container::value_type>;)
ND std::basic_string<To>
charconv(Container const &orig) noexcept(false)
{
      return charconv<To, typename Container::value_type>(orig.data(), orig.size());
}


/*--------------------------------------------------------------------------------------*/


#if defined EMLSP_UNISTRING_NO_WCSLEN
inline size_t strlen (char    const *str) { return ::u8_strlen(reinterpret_cast<uint8_t const *>(str)); }
inline size_t strlen (char8_t const *str) { return ::u8_strlen(reinterpret_cast<uint8_t const *>(str)); }
inline size_t strlen (uint8_t const *str) { return ::u8_strlen(str); }
#else
inline size_t strlen (char    const *str) { return __builtin_strlen(str); }
inline size_t strlen (char8_t const *str) { return __builtin_strlen(reinterpret_cast<char const *>(str)); }
inline size_t strlen (uint8_t const *str) { return __builtin_strlen(reinterpret_cast<char const *>(str)); }
#endif

#if defined EMLSP_UNISTRING_NO_WCSLEN
# if defined WCHAR_IS_U16
inline size_t strlen(wchar_t const *str) { return ::u16_strlen(reinterpret_cast<uint16_t const *>(str)); }
# elif defined WCHAR_IS_U32
inline size_t strlen(wchar_t const *str) { return ::u32_strlen(reinterpret_cast<uint32_t const *>(str)); }
# endif
#else
inline size_t strlen(wchar_t const *str) { return wcslen(str); }
#endif

#if defined WCHAR_IS_U16 && !defined EMLSP_UNISTRING_NO_WCSLEN
inline size_t strlen(uint16_t const *str) { return wcslen(str); }
inline size_t strlen(char16_t const *str) { return wcslen(reinterpret_cast<wchar_t const *>(str)); }
inline size_t strlen(uint32_t const *str) { return ::u32_strlen(str); }
inline size_t strlen(char32_t const *str) { return ::u32_strlen(reinterpret_cast<uint32_t const *>(str)); }
#elif defined WCHAR_IS_U32 && !defined EMLSP_UNISTRING_NO_WCSLEN
inline size_t strlen(uint16_t const *str) { return ::u16_strlen(str); }
inline size_t strlen(char16_t const *str) { return ::u16_strlen(reinterpret_cast<uint16_t const *>(str)); }
inline size_t strlen(uint32_t const *str) { return wcslen(reinterpret_cast<wchar_t const *>(str)); }
inline size_t strlen(char32_t const *str) { return wcslen(reinterpret_cast<wchar_t const *>(str)); }
#else
inline size_t strlen(uint16_t const *str) { return ::u16_strlen(str); }
inline size_t strlen(char16_t const *str) { return ::u16_strlen(reinterpret_cast<uint16_t const *>(str)); }
inline size_t strlen(uint32_t const *str) { return ::u32_strlen(str); }
inline size_t strlen(char32_t const *str) { return ::u32_strlen(reinterpret_cast<uint32_t const *>(str)); }
#endif


/****************************************************************************************/
} // namespace util::unistring
} // namespace emlsp
