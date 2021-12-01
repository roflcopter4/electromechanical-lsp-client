#pragma once
#ifndef HGUARD_d_MACROS_H_
#define HGUARD_d_MACROS_H_
/****************************************************************************************/

#if !defined __GNUC__ && !defined __clang__
# define __attribute__(x)
#endif
#if !defined __declspec && !defined _MSC_VER
# define __declspec(...)
#endif
#ifndef __has_include
# define __has_include(x)
#endif

#ifdef _MSC_VER
# define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING 1
# define _CRT_SECURE_NO_WARNINGS 1
# define _USE_DECLSPECS_FOR_SAL 1
# ifdef __cplusplus
#  ifdef _MSVC_LANG
#   define CXX_LANG_VER _MSVC_LANG
#  endif
# else
#  define restrict __restrict
# endif
#else
# define _Notnull_ //NOLINT
#endif

#if defined _WIN32 || defined _WIN64
# define WIN32_LEAN_AND_MEAN 1
# define NOMINMAX 1
# define DOSISH 1 // One never escapes their roots.
#elif defined __linux__   || defined __bsd__  || defined bsd  || \
      defined __FreeBSD__ || defined __unix__ || defined unix
# define UNIXISH 1
#else
# error "What the hell kind of OS are you even using?"
#endif

/*--------------------------------------------------------------------------------------*/
#ifdef __cplusplus

# ifndef CXX_LANG_VER
#  define CXX_LANG_VER __cplusplus
# endif
# if CXX_LANG_VER >= 201700L || (defined __cplusplus && defined __TAG_HIGHLIGHT__)
#  define UNUSED      [[maybe_unused]]
#  define ND          [[nodiscard]]
#  define NORETURN    [[noreturn]]
#  define FALLTHROUGH [[fallthrough]]
# elif defined __GNUC__
#  define UNUSED      __attribute__((__unused__))
#  define ND          __attribute__((__warn_unused_result__))
#  define NORETURN    __attribute__((__noreturn__))
#  define FALLTHROUGH __attribute__((__fallthrough__))
# elif defined _MSC_VER
#  define UNUSED      __pragma(warning(suppress : 4100 4101))
#  define ND          _Check_return_
#  define NORETURN    __declspec(noreturn)
#  define FALLTHROUGH __fallthrough
# else
#  define UNUSED
#  define ND
#  define NORETURN
#  define FALLTHROUGH
# endif

# define DELETE_COPY_CTORS(t)               \
      t(t const &)                = delete; \
      t &operator=(t const &)     = delete

#define DELETE_MOVE_CTORS(t)                \
      t(t &&) noexcept            = delete; \
      t &operator=(t &&) noexcept = delete

# define DEFAULT_COPY_CTORS(t)               \
      t(t const &)                = default; \
      t &operator=(t const &)     = default

# define DEFAULT_MOVE_CTORS(t)               \
      t(t &&) noexcept            = default; \
      t &operator=(t &&) noexcept = default

#define DUMP_EXCEPTION(e)                                                            \
      do {                                                                           \
            std::cerr << fmt::format(                                                \
                             FMT_COMPILE("Caught exception:\n{}\n" R"(at line {}, in file '{}', in function '{}')"), \
                             (e).what(), __LINE__, __FILE__, __func__)               \
                      << std::endl;                                                  \
            std::cerr.flush();                                                       \
      } while (0)

#else // defined __cplusplus

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

# if defined __GNUC__
#  define NORETURN __attribute__((__noreturn__))
# elif defined _MSC_VER
#  define NORETURN __declspec(noreturn)
# else
#  define NORETURN
# endif

#endif // defined __cplusplus

#if defined __GNUC__ || defined __clang__
# define NOINLINE __attribute__((__noinline__))
#elif defined _MSC_VER
# define NOINLINE __declspec(noinline)
#else
# define NOINLINE
#endif

/*--------------------------------------------------------------------------------------*/

#ifndef __WORDSIZE
# if SIZE_MAX == UINT64_MAX
#  define __WORDSIZE 64
# elif SIZE_MAX == UINT32_MAX
#  define __WORDSIZE 32
# elif SIZE_MAX == UINT16_MAX
#  define __WORDSIZE 16
# else
#  error "No PDP-11s allowed. Sorry."
# endif
#endif

#define P00_HELPER_PASTE_2(a1, a2) a1 ## a2                          //NOLINT(cppcoreguidelines-macro-usage)
#define P99_PASTE_2(a1, a2)     P00_HELPER_PASTE_2(a1, a2)           //NOLINT(cppcoreguidelines-macro-usage)
#define P99_PASTE_3(a1, a2, a3) P99_PASTE_2(P99_PASTE_2(a1, a2), a3) //NOLINT(cppcoreguidelines-macro-usage)

#if !defined __BEGIN_DECLS || !defined __END_DECLS
# ifdef __cplusplus
#  define __BEGIN_DECLS extern "C" {
#  define __END_DECLS   }
# else
#  define __BEGIN_DECLS
#  define __END_DECLS
# endif
#endif

#define SIZE_C        P99_PASTE_3(UINT, __WORDSIZE, _C) //NOLINT(cppcoreguidelines-macro-usage)
#define SSIZE_C       P99_PASTE_3(INT,  __WORDSIZE, _C) //NOLINT(cppcoreguidelines-macro-usage)
#define SLS(s)        ("" s ""), (sizeof(s))            //NOLINT(cppcoreguidelines-macro-usage)
#define LSTRCPY(d, s) memcpy((d), SLS(s))               //NOLINT(cppcoreguidelines-macro-usage)
#define eprintf(...)  fprintf(stderr, __VA_ARGS__)      //NOLINT(cppcoreguidelines-macro-usage)

#if defined _WIN64 && !defined PATH_MAX
# if WDK_NTDDI_VERSION >= NTDDI_WIN10_RS1
#  define PATH_MAX 4096
# else
#  define PATH_MAX MAX_PATH
# endif
#endif

/*--------------------------------------------------------------------------------------*/

#define USEC2SECOND (1000000UL)    /*     1,000,000 - one million */
#define NSEC2SECOND (1000000000UL) /* 1,000,000,000 - one billion */

#define TIMESPEC_FROM_DOUBLE(FLT) \
        { (uintmax_t)(FLT), (uintmax_t)(((double)((FLT) - (double)((uintmax_t)(FLT)))) * (double)NSEC2SECOND) }

#define TIMESPEC_FROM_SECOND_FRACTION(i, d) \
        { (uintmax_t)(i), (uintmax_t)(NSEC2SECOND / ((uintmax_t)(d))) }

#if 0
#define TIMESPEC_FROM_DOUBLE(STS, FLT)              \
        ((void)( (STS)->tv_sec  = (uintmax_t)(FLT), \
                 (STS)->tv_nsec = (uintmax_t)(((double)((FLT) - (double)((uintmax_t)(FLT)))) * (double)NSEC2SECOND) ))

#define TIMESPEC_FROM_SECOND_FRACTION(STS, i, d) \
        ((void)( (STS)->tv_sec  = (uintmax_t)(i), \
                 (STS)->tv_nsec =  (uintmax_t)(NSEC2SECOND / ((uintmax_t)(d))) ))
#endif

/****************************************************************************************/
#endif // macros.h
// vim: ft=c
