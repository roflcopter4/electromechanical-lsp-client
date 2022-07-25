#pragma once
#ifndef HGUARD__UTIL__RECODE__RECODE_PRIVATE_HH_
#define HGUARD__UTIL__RECODE__RECODE_PRIVATE_HH_ //NOLINT
/****************************************************************************************/


#include "Common.hh"
#include <unistr.h>

#if __has_include("cuchar")
#  include <cuchar>
#else
#  include <uchar.h>
#endif

/* "unistr.h" defines these extremely irritating macros. */
#undef true
#undef false
#undef uint8_t
#undef uint16_t
#undef uint32_t
#undef int8_t
#undef int16_t
#undef int32_t
#undef int64_t

//#define FORCE_USE_OF_LIBUNISTRING 1
//#define EMLSP_UNISTRING_NO_WCSLEN
//#define EMLSP_UNISTRING_NO_STRLEN
//#define EMLSP_USE_WIN32_STR_CONVERSION_FUNCS

#ifdef EMLSP_USE_WIN32_STR_CONVERSION_FUNCS
# errror "Not supported"
#endif

#if defined(_MSC_VER) || __has_builtin(__builtin_strlen)
#  define _STRLEN(str) __builtin_strlen(str)
#else
#  define _STRLEN(str) ::strlen(str)
#endif
#if defined(_MSC_VER) || __has_builtin(__builtin_wcslen)
#  define _WCSLEN(str) __builtin_wcslen(str)
#else
#  define _WCSLEN(str) ::wcslen(str)
#endif

#if WCHAR_MAX == INT32_MAX || WCHAR_MAX == UINT32_MAX
#  define WCHAR_IS_U32
#elif WCHAR_MAX == INT16_MAX || WCHAR_MAX == UINT16_MAX
#  define WCHAR_IS_U16
#else
# error "Impossible?!"
#endif


/****************************************************************************************/
#endif
