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


/**
 * \brief Convert a valid unicode string from some character type to any other.
 * \note - The semi-confusing ordering of template parameters as (To, From) is done to
 * allow the From type to be deduced. In use one need only specify the type to which
 * one wants the string to be converted.
 * -# There is no generic implementation of this template. All instantiations are
 * explicit.
 * -# 'char` is assumed to be valid UTF-8, and neither 'signed char` nor 'unsigned char`
 * are supported. Those types should be considered 8-bit integers.
 * \tparam To Desired output char type. Must be given explicitly.
 * \tparam From Input char type (typically deduced).
 * \param orig The NUL terminated string to convert.
 * \param length Number of unicode points in the string.
 * \return The appropriate type of std::basic_string.
 */
template <typename To, typename From>
ND std::basic_string<To>
recode(_In_z_bytecount_(length) From const *orig, _In_ size_t length)
    __attribute__((__nonnull__));


template <typename To, typename From, size_t N>
ND std::basic_string<To>
recode(_In_z_bytecount_(N) From (&orig)[N])
{
      return recode<To, std::remove_cvref_t<From>>(orig, N - SIZE_C(1));
}


template <typename To, class Container>
      REQUIRES (concepts::Integral<typename Container::value_type>)
ND std::basic_string<To>
recode(Container const &orig)
{
      return recode<To, typename Container::value_type>(orig.data(), orig.size());
}


template <typename To, typename From>
      REQUIRES (concepts::Pointer<From> && !concepts::Reference<From>)
ND std::basic_string<To>
recode(From const orig)
{
      return recode<To>(orig, strlen(orig));
}


/*--------------------------------------------------------------------------------------*/
/* Instantiation prototypes. */


#define RECODE(TO, FROM) \
    recode<TO, FROM>(_In_z_bytecount_(length) FROM const *orig, _In_ size_t length) \
         __attribute__((__nonnull__))

extern template ND std::string    RECODE(char,     wchar_t );
extern template ND std::string    RECODE(char,     char8_t );
extern template ND std::string    RECODE(char,     char16_t);
extern template ND std::string    RECODE(char,     char32_t);
extern template ND std::string    RECODE(char,     char    );
extern template ND std::wstring   RECODE(wchar_t,  char    );
extern template ND std::wstring   RECODE(wchar_t,  char8_t );
extern template ND std::wstring   RECODE(wchar_t,  char16_t);
extern template ND std::wstring   RECODE(wchar_t,  char32_t);
extern template ND std::wstring   RECODE(wchar_t,  wchar_t );
extern template ND std::u8string  RECODE(char8_t,  char    );
extern template ND std::u8string  RECODE(char8_t,  wchar_t );
extern template ND std::u8string  RECODE(char8_t,  char16_t);
extern template ND std::u8string  RECODE(char8_t,  char32_t);
extern template ND std::u8string  RECODE(char8_t,  char8_t );
extern template ND std::u16string RECODE(char16_t, char    );
extern template ND std::u16string RECODE(char16_t, wchar_t );
extern template ND std::u16string RECODE(char16_t, char8_t );
extern template ND std::u16string RECODE(char16_t, char32_t);
extern template ND std::u16string RECODE(char16_t, char16_t);
extern template ND std::u32string RECODE(char32_t, char    );
extern template ND std::u32string RECODE(char32_t, wchar_t );
extern template ND std::u32string RECODE(char32_t, char8_t );
extern template ND std::u32string RECODE(char32_t, char16_t);
extern template ND std::u32string RECODE(char32_t, char32_t);

#undef RECODE


/*--------------------------------------------------------------------------------------*/


namespace impl {

using csize_t = size_t const;

ND inline size_t mbsnlen (char     const *str, csize_t size) noexcept { return ::u8_mbsnlen (reinterpret_cast<uint8_t  const *>(str), size); }
#if defined WCHAR_IS_U16
ND inline size_t mbsnlen (wchar_t  const *str, csize_t size) noexcept { return ::u16_mbsnlen(reinterpret_cast<uint16_t const *>(str), size); }
#elif defined WCHAR_IS_U32
ND inline size_t mbsnlen (wchar_t  const *str, csize_t size) noexcept { return ::u32_mbsnlen(reinterpret_cast<uint32_t const *>(str), size); }
#endif

ND inline size_t mbsnlen (uint8_t  const *str, csize_t size) noexcept { return ::u8_mbsnlen (str, size); }
ND inline size_t mbsnlen (uint16_t const *str, csize_t size) noexcept { return ::u16_mbsnlen(str, size); }
ND inline size_t mbsnlen (uint32_t const *str, csize_t size) noexcept { return ::u32_mbsnlen(str, size); }
ND inline size_t mbsnlen (char8_t  const *str, csize_t size) noexcept { return ::u8_mbsnlen (reinterpret_cast<uint8_t  const *>(str), size); }
ND inline size_t mbsnlen (char16_t const *str, csize_t size) noexcept { return ::u16_mbsnlen(reinterpret_cast<uint16_t const *>(str), size); }
ND inline size_t mbsnlen (char32_t const *str, csize_t size) noexcept { return ::u32_mbsnlen(reinterpret_cast<uint32_t const *>(str), size); }

} // namespace impl


template <typename To, class Container>
      REQUIRES (concepts::Integral<typename Container::value_type>)
ND size_t
mbsnlen(Container const &orig) noexcept
{
      return impl::mbsnlen(orig.data(), orig.size());
}

template <typename T, size_t N>
      REQUIRES (!util::concepts::Pointer<T>)
ND size_t mbsnlen(T const (&str)[N]) noexcept
{
      return impl::mbsnlen(str, N - SIZE_C(1));
}

template <typename T>
      REQUIRES (util::concepts::Pointer<T>)
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
