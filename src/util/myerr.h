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
#  define PRINTF_SAL _Printf_format_string_
# endif
#endif

#if defined __cplusplus

# include <fmt/printf.h>
namespace emlsp::util {

extern "C" { extern mtx_t util_c_error_write_mutex; }

template<typename... T>
void
my_err_throw(UNUSED int const     status,
             bool const           print_err,
             char const *restrict file,
             int  const           line,
             char const *restrict func,
             char const *restrict format,
             T   const &... args)
{
      int const e = errno;
      std::stringstream buf;
      mtx_lock(&util_c_error_write_mutex);

      buf << fmt::format(FMT_COMPILE("{}: ({} {} - {}): "), MAIN_PROJECT_NAME, file, line, func)
          << fmt::sprintf(format, args...);

      if (print_err)
            buf << fmt::format(FMT_COMPILE("\n\terrno {}: \"{}\""), e, strerror(e));

      mtx_unlock(&util_c_error_write_mutex);
      throw std::runtime_error(buf.str());
}

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
