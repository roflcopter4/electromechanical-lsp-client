#pragma once
#ifndef HGUARD__UTIL__RECODE__STRLEN_HH_
#define HGUARD__UTIL__RECODE__STRLEN_HH_ //NOLINT
/****************************************************************************************/

#include "recode_private.hh"

inline namespace emlsp {
namespace util::unistring {
/****************************************************************************************/


namespace impl {

#define _UNISTR_STRLEN(TYPE)  \
  NODISCARD inline size_t strlen(_In_z_ TYPE const *const str) noexcept


#if defined EMLSP_UNISTRING_NO_STRLEN
_UNISTR_STRLEN (char)    { return ::u8_strlen(reinterpret_cast<uint8_t const *>(str)); }
_UNISTR_STRLEN (char8_t) { return ::u8_strlen(reinterpret_cast<uint8_t const *>(str)); }
_UNISTR_STRLEN (uint8_t) { return ::u8_strlen(str); }
#else
_UNISTR_STRLEN (char)    { return _STRLEN(str); }
_UNISTR_STRLEN (char8_t) { return _STRLEN(reinterpret_cast<char const *>(str)); }
_UNISTR_STRLEN (uint8_t) { return _STRLEN(reinterpret_cast<char const *>(str)); }
#endif


#if defined EMLSP_UNISTRING_NO_WCSLEN
# if defined WCHAR_IS_U16
_UNISTR_STRLEN (wchar_t) { return ::u16_strlen(reinterpret_cast<uint16_t const *>(str)); }
# elif defined WCHAR_IS_U32
_UNISTR_STRLEN (wchar_t) { return ::u32_strlen(reinterpret_cast<uint32_t const *>(str)); }
# endif
#else
_UNISTR_STRLEN (wchar_t) { return _WCSLEN(str); }
#endif


#if defined WCHAR_IS_U16 && !defined EMLSP_UNISTRING_NO_WCSLEN
_UNISTR_STRLEN (uint16_t) { return _WCSLEN(reinterpret_cast<wchar_t const *>(str)); }
_UNISTR_STRLEN (char16_t) { return _WCSLEN(reinterpret_cast<wchar_t const *>(str)); }
_UNISTR_STRLEN (uint32_t) { return ::u32_strlen(str); }
_UNISTR_STRLEN (char32_t) { return ::u32_strlen(reinterpret_cast<uint32_t const *>(str)); }
#elif defined WCHAR_IS_U32 && !defined EMLSP_UNISTRING_NO_WCSLEN
_UNISTR_STRLEN (uint16_t) { return ::u16_strlen(str); }
_UNISTR_STRLEN (char16_t) { return ::u16_strlen(reinterpret_cast<uint16_t const *>(str)); }
_UNISTR_STRLEN (uint32_t) { return _WCSLEN(reinterpret_cast<wchar_t const *>(str)); }
_UNISTR_STRLEN (char32_t) { return _WCSLEN(reinterpret_cast<wchar_t const *>(str)); }
#else
_UNISTR_STRLEN (uint16_t) { return ::u16_strlen(str); }
_UNISTR_STRLEN (char16_t) { return ::u16_strlen(reinterpret_cast<uint16_t const *>(str)); }
_UNISTR_STRLEN (uint32_t) { return ::u32_strlen(str); }
_UNISTR_STRLEN (char32_t) { return ::u32_strlen(reinterpret_cast<uint32_t const *>(str)); }
#endif

#undef _UNISTR_STRLEN
#undef _WCSLEN
#undef _STRLEN

} // namespace impl

/*--------------------------------------------------------------------------------------*/


template <typename T, size_t N>
ND constexpr size_t strlen(T const (&str)[N]) noexcept
{
      return std::size(str) - SIZE_C(1);
}

template <typename T>
    REQUIRES (
          concepts::Pointer<T> &&
          !concepts::Reference<T> &&
          !concepts::Array<T> &&
          requires (T x) { {*x} -> concepts::Integral; }
    )
ND size_t strlen(T str) noexcept
{
      return impl::strlen(str);
}


/****************************************************************************************/
} // namespace util::unistring
} // namespace emlsp
#endif
