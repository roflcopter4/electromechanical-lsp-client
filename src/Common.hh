#pragma once
#ifndef HGUARD__COMMON_HH_
#define HGUARD__COMMON_HH_ //NOLINT
/****************************************************************************************/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#ifdef USE_JEMALLOC
#  define JEMALLOC_NO_DEMANGLE 1
#  include <jemalloc/jemalloc.h>
#endif
#ifndef _MSC_VER
#  define _Notnull_
#  define _In_
#  define _In_z_
#  define _In_opt_
#  define _In_opt_z_
#  define _In_z_bytecount_(x)
#  define _Out_
#  define _Out_writes_(x)
#  define _Out_z_cap_(x)
#  define _Outptr_
#  define _Outptr_result_z_
#  define _Printf_format_string_
#  define _Post_z_
#endif
//#define __USE_MINGW_ANSI_STDIO 0

#ifdef __RESHARPER__
#  define _CRT_INTERNAL_NONSTDC_NAMES 1
#endif

/*--------------------------------------------------------------------------------------*/

#ifdef __cplusplus

#define MSGPACK_UNPACKER_INIT_BUFFER_SIZE SIZE_C(8'388'608)  /* 2^23 aka 8MiB */
// #define MSGPACK_UNPACKER_INIT_BUFFER_SIZE (8)

# include <assert.h> //NOLINT
# include "pch.hh"

inline namespace MAIN_PACKAGE_NAMESPACE {
using namespace std::literals;
#if FMT_USE_USER_DEFINED_LITERALS
using namespace fmt::literals;
#endif
} // namespace MAIN_PACKAGE_NAMESPACE

#else // not C++

# include <assert.h>
# include <ctype.h>
# include <errno.h>
# include <inttypes.h>
# include <limits.h>
# include <stdarg.h>
# include <stdbool.h>
# include <stddef.h>
# include <stdint.h>
# include <stdio.h>
# include <stdlib.h>
# include <string.h>

# include <fcntl.h>
# include <sys/stat.h>

# if defined _WIN32
#  define WIN32_LEAN_AND_MEAN 1
#  include <sal.h>
#  include <Windows.h>
#  include <WinSock2.h>

#  include <afunix.h>
#  include <direct.h>
#  include <io.h>
#  include <process.h>
# else
#  include <dirent.h>
#  include <spawn.h>
#  include <sys/socket.h>
#  include <sys/un.h>
#  include <sys/wait.h>
#  include <unistd.h>
# endif

#include "util/c/types.h"
#include "util/c/macros.h"

#endif // defined __cplusplus

/*--------------------------------------------------------------------------------------*/

#include "util/c/initializer.h"
#include "util/c/myerr.h"
#include "util/c/debug_trap.h"


/****************************************************************************************/
#endif // Common.hh
// vim: ft=cpp
