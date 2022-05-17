#pragma once
#ifndef HGUARD__UTIL__RECODE__RECODE_HH_
#define HGUARD__UTIL__RECODE__RECODE_HH_ //NOLINT
/****************************************************************************************/

#include "recode_private.hh"
#include "strlen.hh"

inline namespace emlsp {
namespace util {
namespace unistring {
/****************************************************************************************/


#if 0
template <typename T, size_t N>
      requires ( requires {
            requires concepts::NotPointer<T>;
            requires concepts::Reference<T>;
            requires concepts::Array<T>;
      })
ND constexpr size_t strlen(T const (&str)[N]) noexcept
{
      return std::size(str) - SIZE_C(1);
}

template <typename T>
      requires (concepts::Pointer<T>)
ND size_t strlen(std::remove_pointer_t<std::remove_cv_t<T>> const *str) noexcept;


#define _UNISTR_STRLEN(Type)  \
  template <>                 \
  ND __forceinline size_t strlen<std::remove_cvref_t<Type> *>(std::remove_cvref_t<Type> const *str) noexcept

#if defined EMLSP_UNISTRING_NO_WCSLEN
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
_UNISTR_STRLEN (wchar_t> { return ::u16_strlen(reinterpret_cast<uint16_t const *>(str)); }
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
#endif


/*--------------------------------------------------------------------------------------*/


/**
 * \brief Convert a valid unicode string from any char type to any other.
 * \note - The semi-confusing ordering of template parameters as (To, From) is done to
 * allow the From type to be deduced. In use, one need only specify to which type they
 * want the string to be converted.
 * -# There is no generic implementation of this template. All instantiations are
 * explicit.
 * \tparam To Desired output char type. Must be given explicitly.
 * \tparam From Input char type (typically deduced).
 * \param orig The NUL terminated string to convert.
 * \param length Number of unicode points in the string.
 * \return The appropriate type of std::basic_string.
 */
template <typename To, typename From>
ND std::basic_string<To>
recode(From const *orig, size_t length) noexcept(false);


template <typename To, typename From, size_t N>
ND std::basic_string<To>
recode(_In_z_bytecount_(N) From (&orig)[N]) noexcept(false)
{
      return recode<To, std::remove_cvref_t<From>>(orig, N - SIZE_C(1));
}


template <typename To, class Container>
      requires (concepts::Integral<typename Container::value_type>)
ND std::basic_string<To>
recode(Container const &orig)  noexcept(false)
{
      return recode<To, typename Container::value_type>(orig.data(), orig.size());
}


template <typename To, typename From>
      requires (concepts::Pointer<From> && concepts::NotReference<From>)
ND std::basic_string<To>
recode(From const orig)  noexcept(false)
{
      return recode<To>(orig, strlen(orig));
}


/*--------------------------------------------------------------------------------------*/
/* Instantiation prototypes. */


#define RECODE(TO, FROM) \
    recode<TO, FROM>(_In_z_bytecount_(len_) FROM const *str_, _In_ size_t len_) \
        noexcept(false) __attribute__((__nonnull__, __pure__))

template<> ND std::string    RECODE(char,     wchar_t );
template<> ND std::string    RECODE(char,     char8_t );
template<> ND std::string    RECODE(char,     char16_t);
template<> ND std::string    RECODE(char,     char32_t);
template<> ND std::string    RECODE(char,     char    );
template<> ND std::wstring   RECODE(wchar_t,  char    );
template<> ND std::wstring   RECODE(wchar_t,  char8_t );
template<> ND std::wstring   RECODE(wchar_t,  char16_t);
template<> ND std::wstring   RECODE(wchar_t,  char32_t);
template<> ND std::wstring   RECODE(wchar_t,  wchar_t );
template<> ND std::u8string  RECODE(char8_t,  char    );
template<> ND std::u8string  RECODE(char8_t,  wchar_t );
template<> ND std::u8string  RECODE(char8_t,  char16_t);
template<> ND std::u8string  RECODE(char8_t,  char32_t);
template<> ND std::u8string  RECODE(char8_t,  char8_t );
template<> ND std::u16string RECODE(char16_t, char    );
template<> ND std::u16string RECODE(char16_t, wchar_t );
template<> ND std::u16string RECODE(char16_t, char8_t );
template<> ND std::u16string RECODE(char16_t, char32_t);
template<> ND std::u16string RECODE(char16_t, char16_t);
template<> ND std::u32string RECODE(char32_t, char    );
template<> ND std::u32string RECODE(char32_t, wchar_t );
template<> ND std::u32string RECODE(char32_t, char8_t );
template<> ND std::u32string RECODE(char32_t, char16_t);
template<> ND std::u32string RECODE(char32_t, char32_t);

#if 0
#define PREFIX template <> NODISCARD
#define SUFFIX noexcept(false) __attribute__((__nonnull__, __pure__))
PREFIX std::string    recode<char,     wchar_t>  (_In_z_ wchar_t  const *, size_t) SUFFIX;
PREFIX std::string    recode<char,     char8_t>  (_In_z_ char8_t  const *, size_t) SUFFIX;
PREFIX std::string    recode<char,     char16_t> (_In_z_ char16_t const *, size_t) SUFFIX;
PREFIX std::string    recode<char,     char32_t> (_In_z_ char32_t const *, size_t) SUFFIX;
PREFIX std::string    recode<char,     char>     (_In_z_ char     const *, size_t) SUFFIX;
PREFIX std::wstring   recode<wchar_t,  char>     (_In_z_ char     const *, size_t) SUFFIX;
PREFIX std::wstring   recode<wchar_t,  char8_t>  (_In_z_ char8_t  const *, size_t) SUFFIX;
PREFIX std::wstring   recode<wchar_t,  char16_t> (_In_z_ char16_t const *, size_t) SUFFIX;
PREFIX std::wstring   recode<wchar_t,  char32_t> (_In_z_ char32_t const *, size_t) SUFFIX;
PREFIX std::wstring   recode<wchar_t,  wchar_t>  (_In_z_ wchar_t  const *, size_t) SUFFIX;
PREFIX std::u8string  recode<char8_t,  char>     (_In_z_ char     const *, size_t) SUFFIX;
PREFIX std::u8string  recode<char8_t,  wchar_t>  (_In_z_ wchar_t  const *, size_t) SUFFIX;
PREFIX std::u8string  recode<char8_t,  char16_t> (_In_z_ char16_t const *, size_t) SUFFIX;
PREFIX std::u8string  recode<char8_t,  char32_t> (_In_z_ char32_t const *, size_t) SUFFIX;
PREFIX std::u8string  recode<char8_t,  char8_t>  (_In_z_ char8_t  const *, size_t) SUFFIX;
PREFIX std::u16string recode<char16_t, char>     (_In_z_ char     const *, size_t) SUFFIX;
PREFIX std::u16string recode<char16_t, wchar_t>  (_In_z_ wchar_t  const *, size_t) SUFFIX;
PREFIX std::u16string recode<char16_t, char8_t>  (_In_z_ char8_t  const *, size_t) SUFFIX;
PREFIX std::u16string recode<char16_t, char32_t> (_In_z_ char32_t const *, size_t) SUFFIX;
PREFIX std::u16string recode<char16_t, char16_t> (_In_z_ char16_t const *, size_t) SUFFIX;
PREFIX std::u32string recode<char32_t, char>     (_In_z_ char     const *, size_t) SUFFIX;
PREFIX std::u32string recode<char32_t, wchar_t>  (_In_z_ wchar_t  const *, size_t) SUFFIX;
PREFIX std::u32string recode<char32_t, char8_t>  (_In_z_ char8_t  const *, size_t) SUFFIX;
PREFIX std::u32string recode<char32_t, char16_t> (_In_z_ char16_t const *, size_t) SUFFIX;
PREFIX std::u32string recode<char32_t, char32_t> (_In_z_ char32_t const *, size_t) SUFFIX;
#undef PREFIX
#undef SUFFIX
#endif

#undef RECODE


/*--------------------------------------------------------------------------------------*/


namespace impl {

ND inline size_t mbsnlen (char     const *str, size_t size) noexcept { return ::u8_mbsnlen(reinterpret_cast<uint8_t const *>(str), size); }
#if defined WCHAR_IS_U16
ND inline size_t mbsnlen (wchar_t  const *str, size_t size) noexcept { return ::u16_mbsnlen(reinterpret_cast<uint16_t const *>(str), size); }
#elif defined WCHAR_IS_U32
ND inline size_t mbsnlen (wchar_t  const *str, size_t size) noexcept { return ::u32_mbsnlen(reinterpret_cast<uint32_t const *>(str), size); }
#endif

ND inline size_t mbsnlen (uint8_t  const *str, size_t size) noexcept { return ::u8_mbsnlen(str, size); }
ND inline size_t mbsnlen (char8_t  const *str, size_t size) noexcept { return ::u8_mbsnlen(reinterpret_cast<uint8_t const *>(str), size); }
ND inline size_t mbsnlen (uint16_t const *str, size_t size) noexcept { return ::u16_mbsnlen(str, size); }
ND inline size_t mbsnlen (char16_t const *str, size_t size) noexcept { return ::u16_mbsnlen(reinterpret_cast<uint16_t const *>(str), size); }
ND inline size_t mbsnlen (uint32_t const *str, size_t size) noexcept { return ::u32_mbsnlen(str, size); }
ND inline size_t mbsnlen (char32_t const *str, size_t size) noexcept { return ::u32_mbsnlen(reinterpret_cast<uint32_t const *>(str), size); }

} // namespace impl


template <typename To, class Container>
      requires (concepts::Integral<typename Container::value_type>)
ND size_t
mbsnlen(Container const &orig) noexcept
{
      return impl::mbsnlen(orig.data(), orig.size());
}

template <typename T, size_t N>
      requires (util::concepts::NotPointer<T>)
ND size_t mbsnlen(T const (&str)[N]) noexcept
{
      return impl::mbsnlen(str, N - SIZE_C(1));
}

template <typename T>
      requires (util::concepts::Pointer<T>)
ND size_t mbsnlen(std::remove_pointer_t<std::remove_cv_t<T>> const *str) noexcept
{
      return impl::mbsnlen(str, strlen(str));
}

template <typename T>
ND size_t mbsnlen(T const *str, size_t const size) noexcept
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
#endif
