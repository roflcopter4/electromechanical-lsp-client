// ReSharper disable CppInconsistentNaming
#ifndef HGUARD_d_UTIL_H_
#define HGUARD_d_UTIL_H_
#pragma once

#include "Common.hh"

#if defined __GNUC__ || defined __clang__
# define PRINTF_SAL
# ifdef __clang__
#  define ATTRIBUTE_PRINTF(...) __attribute__((__format__(__printf__, __VA_ARGS__)))
# else
#  define ATTRIBUTE_PRINTF(...) __attribute__((__format__(__gnu_printf__, __VA_ARGS__)))
# endif
# define ATTRIBUTE_NONNULL(...) __attribute__((__nonnull__(__VA_ARGS__)))
#else
# define ATTRIBUTE_PRINTF(...)
# define ATTRIBUTE_NONNULL(...)
#endif
#define CSTR_RESTRICT _In_z_ char const *__restrict
#define FORMAT_STR    _In_z_ _Printf_format_string_ char const *__restrict


/****************************************************************************************/
#if defined __cplusplus

inline namespace MAIN_PACKAGE_NAMESPACE {
namespace util {

NORETURN extern void
my_err_throw(
      _In_ bool     print_err,
      CSTR_RESTRICT file,
      _In_ int      line,
      CSTR_RESTRICT func,
      FORMAT_STR    format,
      ...
) ATTRIBUTE_PRINTF(5, 6) __attribute__((__nonnull__(2, 4, 5)));


} // namespace util
} // namespace MAIN_PACKAGE_NAMESPACE

#define err(...)  ::MAIN_PACKAGE_NAMESPACE::util::my_err_throw(true,  __FILE__, __LINE__, FUNCTION_NAME, ##__VA_ARGS__)
#define errx(...) ::MAIN_PACKAGE_NAMESPACE::util::my_err_throw(false, __FILE__, __LINE__, FUNCTION_NAME, ##__VA_ARGS__)

#define err_nothrow(...)  ::emlsp_my_err_(true,  __FILE__, __LINE__, FUNCTION_NAME, ##__VA_ARGS__)
#define errx_nothrow(...) ::emlsp_my_err_(false, __FILE__, __LINE__, FUNCTION_NAME, ##__VA_ARGS__)

#else // defined __cplusplus

#  define err(...)  emlsp_my_err_(true,  __FILE__, __LINE__, FUNCTION_NAME, ##__VA_ARGS__)
#  define errx(...) emlsp_my_err_(false, __FILE__, __LINE__, FUNCTION_NAME, ##__VA_ARGS__)

#endif
/****************************************************************************************/

__BEGIN_DECLS

#define warn(...)   emlsp_my_warn_(true,  false, __FILE__, __LINE__, FUNCTION_NAME, ##__VA_ARGS__)
#define warnx(...)  emlsp_my_warn_(false, false, __FILE__, __LINE__, FUNCTION_NAME, ##__VA_ARGS__)
#define shout(...)  emlsp_my_warn_(true,  true,  __FILE__, __LINE__, FUNCTION_NAME, ##__VA_ARGS__)
#define shoutx(...) emlsp_my_warn_(false, true,  __FILE__, __LINE__, FUNCTION_NAME, ##__VA_ARGS__)

NORETURN extern void
emlsp_my_err_(
      _In_ bool     print_err,
      CSTR_RESTRICT file,
      _In_ int      line,
      CSTR_RESTRICT func,
      FORMAT_STR    format,
      ...
) ATTRIBUTE_PRINTF(5, 6) ATTRIBUTE_NONNULL(2, 4, 5);

extern void
emlsp_my_warn_(
      _In_ bool     print_err,
      _In_ bool     force,
      CSTR_RESTRICT file,
      _In_ int      line,
      CSTR_RESTRICT func,
      FORMAT_STR    format,
      ...
) ATTRIBUTE_PRINTF(6, 7) ATTRIBUTE_NONNULL(3, 5, 6);

__END_DECLS

/****************************************************************************************/
#undef ATTRIBUTE_PRINTF
#undef ATTRIBUTE_NONNULL
#undef PRINTF_SAL
#undef CSTR_RESTRICT
#undef FORMAT_STR
#endif
// vim: ft=cpp
