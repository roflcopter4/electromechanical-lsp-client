#pragma once
#ifndef HGUARD_d_TYPES_H_
#define HGUARD_d_TYPES_H_
/****************************************************************************************/

#ifdef DOSISH
# define MSG_EOR 0
typedef SOCKET socket_t; //NOLINT(modernize-use-using)
typedef errno_t error_t; //NOLINT(modernize-use-using)
// Windows doesn't define ssize_t for some inexplicable reason.
typedef P99_PASTE_3(uint, __WORDSIZE, _t) sssize_t;
#else
typedef int socket_t; //NOLINT(modernize-use-using)
typedef int errno_t;  //NOLINT(modernize-use-using)
typedef int error_t;  //NOLINT(modernize-use-using)
#endif

/****************************************************************************************/
#endif /* types.h */
// vim: ft=c
