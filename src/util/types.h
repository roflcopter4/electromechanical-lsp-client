#pragma once
#ifndef HGUARD__TYPES_H_
#define HGUARD__TYPES_H_
/****************************************************************************************/

#ifdef _WIN32
#  define MSG_EOR 0
typedef SOCKET socket_t; //NOLINT(modernize-use-using)
typedef errno_t error_t; //NOLINT(modernize-use-using)
// Windows doesn't define ssize_t for some inexplicable reason.
typedef P99_PASTE_3(int, __WORDSIZE, _t) ssize_t;
#else
typedef int socket_t; //NOLINT(modernize-use-using)
typedef int errno_t;  //NOLINT(modernize-use-using)
typedef int error_t;  //NOLINT(modernize-use-using)
#endif

#ifndef SOCK_CLOEXEC
#  define SOCK_CLOEXEC 0
#endif

/****************************************************************************************/
#endif /* types.h */
// vim: ft=c
