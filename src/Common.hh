#pragma once
#ifndef HGUARD_d_COMMON_HH_
#define HGUARD_d_COMMON_HH_
/****************************************************************************************/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "util/macros.h"

/*--------------------------------------------------------------------------------------*/

#ifdef __cplusplus

# include "pch.hh"
# include "util/types.h"
# include "util/util.hh"

#else // not C++

# include <assert.h>
# include <ctype.h>
# include <errno.h>
# include <fcntl.h>
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

# if defined DOSISH
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

# include "util/types.h"
# include "util/myerr.h"

#endif // defined __cplusplus

/*--------------------------------------------------------------------------------------*/

#include "util/initializer_hack.h"

/****************************************************************************************/
#endif // Common.hh
// vim: ft=cpp