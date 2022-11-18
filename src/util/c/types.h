#pragma once
#ifndef HGUARD__TYPES_H_
#define HGUARD__TYPES_H_
/****************************************************************************************/

#include "util/c/macros.h"

__BEGIN_DECLS

#ifdef _WIN32
#  define MSG_EOR 0
typedef SOCKET socket_t;
typedef errno_t error_t;
// Windows doesn't define ssize_t for some inexplicable reason.
typedef P99_PASTE_3(int, __WORDSIZE, _t) ssize_t;
#else
typedef int socket_t;
typedef int errno_t;
typedef int error_t;
/* It makes life just a little easier to have this type defined. */
typedef uint32_t DWORD;
#endif

typedef float  float32_t;
typedef double float64_t;
#if defined __DBL_MANT_DIG__ && \
     __DBL_MANT_DIG__  != __LDBL_MANT_DIG__ && \
     __LDBL_MANT_DIG__ != 113
typedef long double float80_t;
#endif
#ifdef __GNUC__
typedef __float128 float128_t;
#endif

typedef int const intc;


#ifndef SOCK_CLOEXEC
#  define SOCK_CLOEXEC 0
#endif

__END_DECLS

/****************************************************************************************/
#endif /* types.h */
// vim: ft=c
