#ifndef HGUARD__COMMON_HH_
#define HGUARD__COMMON_HH_ //NOLINT
#pragma once
/****************************************************************************************/

#define _GLIBCXX_ASSERTIONS 1
#define _GLIBCXX_PARALLEL 1
#define _GLIBCXX_PARALLEL_ASSERTIONS 1
#undef NDEBUG

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#ifdef USE_JEMALLOC
#  define JEMALLOC_NO_DEMANGLE 1
#  include <jemalloc/jemalloc.h>
#endif

/*--------------------------------------------------------------------------------------*/

#ifdef __cplusplus

#define MSGPACK_UNPACKER_INIT_BUFFER_SIZE (1<<23)
// #define MSGPACK_UNPACKER_INIT_BUFFER_SIZE (8)

# include <assert.h>
# include "pch.hh"

inline namespace emlsp {
using namespace std::literals;
#if FMT_USE_USER_DEFINED_LITERALS
using namespace fmt::literals;
#endif
} // namespace emlsp

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
#  include <Windows.h>
#  include <winsock2.h>

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

#endif // defined __cplusplus

/*--------------------------------------------------------------------------------------*/

#include "util/macros.h"

#ifdef HAVE_THREADS_H
# include <threads.h>
#else
# include <tinycthread.h>
#endif

#include "util/types.h"

#ifdef __cplusplus
# include "util/util.hh"
#else
/* nothing */
#endif

#include "util/initializer_hack.h"
#include "util/myerr.h"

/****************************************************************************************/
#endif // Common.hh
// vim: ft=cpp
