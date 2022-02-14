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
#else
# if defined _MSC_VER
#  define PRINTF_SAL _Notnull_ _Printf_format_string_
# else
#  define PRINTF_SAL
# endif
# define ATTRIBUTE_PRINTF(...)
#endif
#define CSTR_RESTRICT char const *restrict
#define FORMAT_STR    PRINTF_SAL CSTR_RESTRICT

#if defined __cplusplus

inline namespace emlsp {
namespace util {

/* The C++ function 'my_err_throw` must use the same mutex as the C function my_err_,
 * even if it means awkwardly mixing APIs. */
extern "C" { extern mtx_t util_c_error_write_mutex; }

NORETURN
extern void
my_err_throw(int           status,
             bool          print_err,
             CSTR_RESTRICT file,
             int           line,
             CSTR_RESTRICT func,
             FORMAT_STR    format,
             ...)
      ATTRIBUTE_PRINTF(6, 7) __attribute__((__nonnull__(6)));


} // namespace util
} // namespace emlsp

#define err(EVAL, ...)  ::util::my_err_throw((EVAL), true,  __FILE__, __LINE__, FUNCTION_NAME, ##__VA_ARGS__)
#define errx(EVAL, ...) ::util::my_err_throw((EVAL), false, __FILE__, __LINE__, FUNCTION_NAME, ##__VA_ARGS__)

#define err_nothrow(EVAL, ...)  my_err_((EVAL), true,  __FILE__, __LINE__, FUNCTION_NAME, ##__VA_ARGS__)
#define errx_nothrow(EVAL, ...) my_err_((EVAL), false, __FILE__, __LINE__, FUNCTION_NAME, ##__VA_ARGS__)

#else // defined __cplusplus

#  define err(EVAL, ...)  my_err_((EVAL), true,  __FILE__, __LINE__, FUNCTION_NAME, ##__VA_ARGS__)
#  define errx(EVAL, ...) my_err_((EVAL), false, __FILE__, __LINE__, FUNCTION_NAME, ##__VA_ARGS__)

#endif

__BEGIN_DECLS

#define warn(...)       my_warn_(true,  false, __FILE__, __LINE__, FUNCTION_NAME, ##__VA_ARGS__)
#define warnx(...)      my_warn_(false, false, __FILE__, __LINE__, FUNCTION_NAME, ##__VA_ARGS__)
#define shout(...)      my_warn_(true,  true,  __FILE__, __LINE__, FUNCTION_NAME, ##__VA_ARGS__)
#define shoutx(...)     my_warn_(false, true,  __FILE__, __LINE__, FUNCTION_NAME, ##__VA_ARGS__)

NORETURN extern void
my_err_(int           status,
        bool          print_err,
        CSTR_RESTRICT file,
        int           line,
        CSTR_RESTRICT func,
        FORMAT_STR    format,
        ...)
     ATTRIBUTE_PRINTF(6, 7) __attribute__((__nonnull__(6)));

extern void
my_warn_(bool          print_err,
         bool          force,
         CSTR_RESTRICT file,
         int           line,
         CSTR_RESTRICT func,
         FORMAT_STR    format,
         ...)
     ATTRIBUTE_PRINTF(6, 7) __attribute__((__nonnull__(6)));

__END_DECLS

#undef ATTRIBUTE_PRINTF
#undef PRINTF_SAL
#undef CSTR_RESTRICT
#undef FORMAT_STR
#endif
// vim: ft=cpp
