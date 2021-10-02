#pragma once
#ifndef HGUARD__CONTRIB__INITIALIZER_HACK_H_
#define HGUARD__CONTRIB__INITIALIZER_HACK_H_

// Initializer/finalizer sample for MSVC and GCC/Clang.
// 2010-2016 Joe Lowe. Released into the public domain.

/*
 * Edited somewhat by some idiot called RoflCopter4. Now uses the
 * common extension macro `__COUNTER__' and the `__LINE__' macro to
 * create a unique name.
 */

/*--------------------------------------------------------------------------------------*/
#if  defined(__GNUC__) || defined(__clang__)  
# define HELP_INITIALIZER_HACK_03_(FN, UNIQ) \
      __attribute__((__constructor__)) static void FN##_##UNIQ(void)

/*--------------------------------------------------------------------------------------*/
#elif defined __cplusplus
# define HELP_INITIALIZER_HACK_03_(FN, UNIQ) \
      HELP_INITIALIZER_HACK_04_(FN##_##UNIQ, FN##_cxx_run_at_program_init_hack_##UNIQ)

# define HELP_INITIALIZER_HACK_04_(FN, ST)      \
      static void FN(void);                     \
      struct ST##t_ { ST##t_(void) { FN(); } }; \
      static ST##t_ ST;                         \
      static void FN(void)

/*--------------------------------------------------------------------------------------*/
#elif defined(_MSC_VER)
# ifdef _WIN64
#  define HELP_INITIALIZER_HACK_03_(FN, UNIQ) \
      HELP_INITIALIZER_HACK_04_(FN##_##UNIQ, "")
# else
#  define HELP_INITIALIZER_HACK_03_(FN, UNIQ) \
      HELP_INITIALIZER_HACK_04_(FN##_##UNIQ, "_")
# endif

# pragma section(".CRT$XCU", read)
# define HELP_INITIALIZER_HACK_04_(FN, USCORE)                   \
      static void FN(void);                                      \
      __declspec(allocate(".CRT$XCU")) void (*FN##_)(void) = FN; \
      __pragma(comment(linker, "/include:" USCORE #FN "_"))      \
      static void FN(void)

/*--------------------------------------------------------------------------------------*/
#else
# error "Are you using tcc or something? Sorry, your oddball compiler isn't supported."
#endif

/*
 * Define a function which will be run at program init, though in an unspecified order.
 * The parameter is combined with __COUNTER__ and __LINE__ to make a unique identifier
 * that is used as the function's name. The name can therefore be anything you want.
 */
#define INITIALIZER_HACK(FN)                   HELP_INITIALIZER_HACK_01_(FN, __COUNTER__, __LINE__)
#define HELP_INITIALIZER_HACK_01_(FN, CNT, LN) HELP_INITIALIZER_HACK_02_(FN, CNT, LN)
#define HELP_INITIALIZER_HACK_02_(FN, CNT, LN) HELP_INITIALIZER_HACK_03_(FN, CNT ## _ ## LN)

#endif // initializer_hack.h
// vim: ft=cpp
