#pragma once
#ifndef HGUARD__TYPES_H_
#define HGUARD__TYPES_H_
/****************************************************************************************/

#include "util/macros.h"

#ifdef _WIN32
#  define MSG_EOR 0
typedef SOCKET socket_t; //NOLINT(modernize-use-using)
typedef errno_t error_t; //NOLINT(modernize-use-using)
// Windows doesn't define ssize_t for some inexplicable reason.
typedef P99_PASTE_3(int, __WORDSIZE, _t) ssize_t;
#else
typedef int socket_t;   //NOLINT(modernize-use-using)
typedef int errno_t;    //NOLINT(modernize-use-using)
typedef int error_t;    //NOLINT(modernize-use-using)
/* It makes life just a little easier to have this type defined. */
typedef uint32_t DWORD; //NOLINT(modernize-use-using)
#endif

typedef float  float32_t;
typedef double float64_t;
#if __DBL_MANT_DIG__ != __LDBL_MANT_DIG__ && __LDBL_MANT_DIG__ != 113
typedef long double float80_t;
#endif
#ifdef __GNUC__
typedef __float128 float128_t;
#endif


#ifndef SOCK_CLOEXEC
#  define SOCK_CLOEXEC 0
#endif

/****************************************************************************************/
#endif /* types.h */
// vim: ft=c
