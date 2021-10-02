#ifndef __USE_ISOC99
# define __USE_ISOC99 1
#endif
#ifndef __USE_ISOC11
# define __USE_ISOC11 1
#endif

#include "Common.hh"
#include "util/c_util.h"

#if HAVE_GETRANDNUM
#  include <sys/random.h>
#  define RAND(dst) getrandom(&dest, sizeof(dst), 0)
#elif HAVE_ARC4RANDOM
#  define RAND(dst) arc4random_buf(&dst, sizeof(dst))
#endif

#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifdef _MSC_VER
# define restrict __restrict
#endif

#ifdef DUMB_TEMPNAME_NUM_CHARS
# define NUM_RANDOM_CHARS DUMB_TEMPNAME_NUM_CHARS
#else
# define NUM_RANDOM_CHARS 9
#endif

#ifdef _WIN32
# define FILESEP_CHAR '\\'
# define FILESEP_STR  "\\"
# define CHAR_IS_FILESEP(ch) ((ch) == '\\' || (ch) == '/')
# ifndef PATH_MAX
#  if WDK_NTDDI_VERSION >= NTDDI_WIN10_RS1
#   define PATH_MAX 4096
#  else
#   define PATH_MAX MAX_PATH
#  endif
# endif
#else
# define FILESEP_CHAR '/'
# define FILESEP_STR  "/"
# define CHAR_IS_FILESEP(ch) ((ch) == '/')
#endif

static char *get_random_chars(char *buf);

/*======================================================================================*/

char *
even_dumber_tempname(char       *restrict       buf,
                     char const *restrict const dir,
                     char const *restrict const prefix,
                     char const *restrict const suffix)
{
      size_t len = strlen(dir);
      memcpy(buf, dir, len + 1);

      if (!CHAR_IS_FILESEP(buf[len - 1]))
            buf[len++] = FILESEP_CHAR;

      char *ptr = buf + len;

      if (prefix) {
            len = strlen(prefix);
            memcpy(ptr, prefix, len);
            ptr += len;
      }

      ptr = get_random_chars(ptr);

      if (suffix) {
            len = strlen(suffix);
            memcpy(ptr, suffix, len);
            ptr += len;
      }

      *ptr = '\0';
      return buf;
}

static char *
get_random_chars(char *buf)
{
      char *ptr = buf;


      for (int i = 0; i < NUM_RANDOM_CHARS; ++i) {
            unsigned const tmp = rand();

            if ((tmp & 0x0F) < 2)
                  *ptr++ = (char)((rand() % 10) + '0');
            else
                  *ptr++ = (char)((rand() % 26) + ((tmp & 1) ? 'A' : 'a'));
      }

      return ptr;
}

/*--------------------------------------------------------------------------------------*/

INITIALIZER_HACK(init)
{
      srand(cxx_random_device_get_random_val());
}
