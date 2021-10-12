#pragma once
#ifndef HGUARD_d_UTIL_H_
#define HGUARD_d_UTIL_H_

#include "Common.hh"

#if defined __GNUC__ || defined __clang__
# define FUNCTION_ __PRETTY_FUNCTION__
# define PRINTF_SAL
# ifdef __clang__
#  define ATTRIBUTE_PRINTF(...) __attribute__((__format__(printf, __VA_ARGS__)))
# else
#  define ATTRIBUTE_PRINTF(...) __attribute__((__format__(gnu_printf, __VA_ARGS__)))
# endif
#else
# define ATTRIBUTE_PRINTF(...)
# if defined _MSC_VER
#  define FUNCTION_ __FUNCTION__
#  define PRINTF_SAL _Printf_format_string_
# else
#  define FUNCTION_ __func__
#  define PRINTF_SAL
# endif
#endif

#if defined __cplusplus

namespace emlsp::util {

extern "C" { extern mtx_t util_c_error_write_mutex; }

extern void
my_err_throw(int  const           status,
             bool const           print_err,
             char const *restrict file,
             int  const           line,
             char const *restrict func,
             char const *restrict format,
             ...);

} // namespace emlsp::util

#  define err(EVAL, ...)  emlsp::util::my_err_throw((EVAL), true,  __FILE__, __LINE__, FUNCTION_, ##__VA_ARGS__)
#  define errx(EVAL, ...) emlsp::util::my_err_throw((EVAL), false, __FILE__, __LINE__, FUNCTION_, ##__VA_ARGS__)

#else // defined __cplusplus

#  define err(EVAL, ...)  my_err_ ((EVAL), true,  __FILE__, __LINE__, FUNCTION_, __VA_ARGS__)
#  define errx(EVAL, ...) my_err_ ((EVAL), false, __FILE__, __LINE__, FUNCTION_, __VA_ARGS__)

#endif

__BEGIN_DECLS

#define warn(...)       my_warn_(true,  false, __FILE__, __LINE__, FUNCTION_, ##__VA_ARGS__)
#define warnx(...)      my_warn_(false, false, __FILE__, __LINE__, FUNCTION_, ##__VA_ARGS__)
#define shout(...)      my_warn_(true,  true,  __FILE__, __LINE__, FUNCTION_, ##__VA_ARGS__)
#define shoutx(...)     my_warn_(false, true,  __FILE__, __LINE__, FUNCTION_, ##__VA_ARGS__)

NORETURN extern void my_err_(int  status,
                             bool print_err,
                             char const *__restrict file,
                             int line,
                             char const *__restrict func,
                             PRINTF_SAL char const *__restrict format,
                             ...) ATTRIBUTE_PRINTF(6, 7);

extern void          my_warn_(bool print_err,
                              bool force,
                              char const *__restrict file,
                              int line,
                              char const *__restrict func,
                              PRINTF_SAL char const *__restrict format,
                              ...) ATTRIBUTE_PRINTF(6, 7);

__END_DECLS

#undef ATTRIBUTE_PRINTF
#undef PRINTF_SAL
#endif
// vim: ft=cpp
