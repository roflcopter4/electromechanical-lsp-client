#pragma once
#ifndef HGUARD__MACROS_H_
#define HGUARD__MACROS_H_
/****************************************************************************************/

#include <limits.h>

#if !defined __GNUC__ && !defined __clang__ && !defined __attribute__
# define __attribute__(x)
#endif
#ifndef __attr_access
# define __attr_access(x)
#endif
#if !defined __declspec && !defined _MSC_VER
# define __declspec(...)
#endif
#ifndef __has_include
# define __has_include(x)
#endif

#ifdef __cplusplus
# if defined _MSC_VER && defined _MSVC_LANG
#  define CXX_LANG_VER _MSVC_LANG
# endif
# ifndef CXX_LANG_VER
#  define CXX_LANG_VER __cplusplus
# endif
#endif

#if (defined __cplusplus && (CXX_LANG_VER >= 201700L)) || (defined __STDC_VERSION__ && __STDC_VERSION__ > 201710L)
# define UNUSED      [[maybe_unused]]
# define ND          [[nodiscard]]
# define NORETURN    [[noreturn]]
# define FALLTHROUGH [[fallthrough]]
#elif defined __GNUC__
# define UNUSED      __attribute__((__unused__))
# define ND          __attribute__((__warn_unused_result__))
# define NORETURN    __attribute__((__noreturn__))
# define FALLTHROUGH __attribute__((__fallthrough__))
#elif defined _MSC_VER
# define UNUSED      __pragma(warning(suppress : 4100 4101))
# define ND          _Check_return_
# define NORETURN    __declspec(noreturn)
# define FALLTHROUGH __fallthrough
#else
# define UNUSED
# define ND
# define NORETURN
# define FALLTHROUGH
#endif

#ifdef __cplusplus

# define DELETE_COPY_CTORS(t)               \
      t(t const &)                = delete; \
      t &operator=(t const &)     = delete

# define DELETE_MOVE_CTORS(t)               \
      t(t &&) noexcept            = delete; \
      t &operator=(t &&) noexcept = delete

# define DEFAULT_COPY_CTORS(t)               \
      t(t const &)                = default; \
      t &operator=(t const &)     = default

# define DEFAULT_MOVE_CTORS(t)               \
      t(t &&) noexcept            = default; \
      t &operator=(t &&) noexcept = default

# define DUMP_EXCEPTION(e)                                                             \
      do {                                                                             \
            fmt::print(stderr,                                                         \
                       FMT_COMPILE("Caught exception:\n{}\nat line {}, in file '{}', " \
                                   "in function '{}'\n"),                              \
                       (e).what(), __LINE__, __FILE__, FUNCTION_NAME);                 \
            fflush(stderr);                                                            \
      } while (0)

# ifdef __TAG_HIGHLIGHT__
#  define REQUIRES(...)
# else
#  define REQUIRES(...) requires __VA_ARGS__
# endif

# ifndef NO_OBNOXIOUS_TWO_LETTER_CONVENIENCE_MACROS_PLEASE
#  define FC(str) FMT_COMPILE(str)
# endif

#else // not defined __cplusplus

# ifndef thread_local
#  if defined(_MSC_VER)
#   define thread_local __declspec(thread)
#  elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
#   define thread_local _Thread_local
#  elif defined(__GNUC__)
#   define thread_local __thread
#  else
#   define thread_local
#  endif
# endif

# ifndef static_assert
#  if (defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L) || defined _MSC_VER
#   define static_assert(...) _Static_assert(__VA_ARGS__)
#  else
#   define static_assert(COND ((char [(COND) ? 1 : -1]){})
#  endif
# endif

# if defined __GNUC__ || defined __clang__
#  define NORETURN __attribute__((__noreturn__))
# elif defined _MSC_VER
#  define NORETURN __declspec(noreturn)
# elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
#  define NORETURN _Noreturn
# else
#  define NORETURN
# endif

#endif // defined __cplusplus

#if defined __GNUC__ || defined __clang__
# define NOINLINE __attribute__((__noinline__))
# ifndef __always_inline
#  define __always_inline __attribute__((__always_inline__)) __inline__
# endif
# ifndef __forceinline
#  define __forceinline __always_inline
# endif
#elif defined _MSC_VER
# define NOINLINE __declspec(noinline)
#else
# define NOINLINE
# define __forceinline __inline
#endif

/*--------------------------------------------------------------------------------------*/

#ifndef __WORDSIZE
# if SIZE_MAX == ULLONG_MAX
#  define __WORDSIZE 64
# elif SIZE_MAX == UINT_MAX
#  define __WORDSIZE 32
# elif SIZE_MAX == SHRT_MAX
#  define __WORDSIZE 16
# else
#  error "I have no useful warning message to give here. You know what you did."
# endif
#endif

#define P00_HELPER_PASTE_2(a1, a2) a1 ## a2
#define P00_PASTE_2(a1, a2)        P00_HELPER_PASTE_2(a1, a2)

#define P99_PASTE_0(...)          __VA_ARGS__
#define P99_PASTE_1(...)          __VA_ARGS__
#define P99_PASTE_2(...)          P00_PASTE_2(__VA_ARGS__)
#define P99_PASTE_3(a1, a2, ...)  P99_PASTE_2(P99_PASTE_2(a1, a2), __VA_ARGS__)
#define P99_PASTE_4(a1, a2, ...)  P99_PASTE_3(P99_PASTE_2(a1, a2), __VA_ARGS__)
#define P99_PASTE_5(a1, a2, ...)  P99_PASTE_4(P99_PASTE_2(a1, a2), __VA_ARGS__)
#define P99_PASTE_6(a1, a2, ...)  P99_PASTE_5(P99_PASTE_2(a1, a2), __VA_ARGS__)
#define P99_PASTE_7(a1, a2, ...)  P99_PASTE_6(P99_PASTE_2(a1, a2), __VA_ARGS__)
#define P99_PASTE_8(a1, a2, ...)  P99_PASTE_7(P99_PASTE_2(a1, a2), __VA_ARGS__)
#define P99_PASTE_9(a1, a2, ...)  P99_PASTE_8(P99_PASTE_2(a1, a2), __VA_ARGS__)
#define P99_PASTE_10(a1, a2, ...) P99_PASTE_9(P99_PASTE_2(a1, a2), __VA_ARGS__)

#define P00_NUM_ARGS_b(_0a, _0b, _2, _3, _4, _5, _6, _7, _8, _9, _10, ...) _10
#define P00_NUM_ARGS_a(...) P00_NUM_ARGS_b(__VA_ARGS__, 10, 9, 8, 7, 6, 5, 4, 3, 2, 0, 0)
#define P00_NUM_ARGS(...)   P00_NUM_ARGS_a(__VA_ARGS__)
#define P00_PASTE_a(n)      P00_PASTE_2(P99_PASTE_, n)
#define P00_PASTE(...)      P00_PASTE_a(P00_NUM_ARGS(__VA_ARGS__))

#define P99_PASTE(...) P00_PASTE(__VA_ARGS__)(__VA_ARGS__)

#define P00_PASTE_TEST_1(a, b, c, ...) P99_PASTE(a, b, c, __VA_ARGS__)
#if P99_PASTE(0, x, A, F, 5, 1, 0, U, L, L) != 0xAF510ULL || P00_PASTE_TEST_1(0, x, F, F, 0, 0, U) != 0xFF00U
# error "Your C pre-processor is broken or non-conformant. Cannot continue."
#endif
#undef P00_PASTE_TEST_1

#if !defined __BEGIN_DECLS || !defined __END_DECLS
# ifdef __cplusplus
#  define __BEGIN_DECLS extern "C" {
#  define __END_DECLS   }
# else
#  define __BEGIN_DECLS
#  define __END_DECLS
# endif
#endif

#define SIZE_C        P99_PASTE_3(UINT, __WORDSIZE, _C)
#define SSIZE_C       P99_PASTE_3(INT,  __WORDSIZE, _C)
#define SLS(s)        ("" s ""), (sizeof(s))
#define LSTRCPY(d, s) memcpy((d), SLS(s))
#define eprintf(...)  fprintf(stderr, __VA_ARGS__)

#if defined _WIN64 && !defined PATH_MAX
# if WDK_NTDDI_VERSION >= NTDDI_WIN10_RS1
#  define PATH_MAX 4096
# else
#  define PATH_MAX MAX_PATH
# endif
#endif

#if defined __GNUC__ || defined __clang__
# define FUNCTION_NAME __PRETTY_FUNCTION__
#elif defined _MSC_VER
# define FUNCTION_NAME __FUNCTION__
#else
# define FUNCTION_NAME __func__
#endif


/*--------------------------------------------------------------------------------------*/

#ifdef __cplusplus
# define USEC2SECOND UINTMAX_C(1'000'000)     /*     1,000,000 - one million */
# define NSEC2SECOND UINTMAX_C(1'000'000'000) /* 1,000,000,000 - one billion */
#else
# define USEC2SECOND UINTMAX_C(1000000)    /*     1,000,000 - one million */
# define NSEC2SECOND UINTMAX_C(1000000000) /* 1,000,000,000 - one billion */
#endif

#define BIGFLT double

#define TIMESPEC_FROM_DOUBLE(FLT)                                          \
      { (uintmax_t)((BIGFLT)(FLT)),                                        \
            (uintmax_t)(((BIGFLT)((BIGFLT)(FLT) -                          \
                                  (BIGFLT)((uintmax_t)((BIGFLT)(FLT))))) * \
                        (BIGFLT)NSEC2SECOND) }

#undef BIGFLT

#define TIMESPEC_FROM_SECOND_FRACTION(seconds, numerator, denominator)               \
      {                                                                              \
            (uintmax_t)(seconds),                                                    \
            (uintmax_t)((denominator) == 0 ? UINTMAX_C(0)                            \
                                           : (((uintmax_t)(numerator)*NSEC2SECOND) / \
                                              (uintmax_t)(denominator)))             \
      }

/****************************************************************************************/
#endif // macros.h
// vim: ft=c
