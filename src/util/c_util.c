#include "Common.hh"
#include "util/c_util.h"

#ifndef HAVE_ASPRINTF
#  if !defined __MINGW32__ && !defined __MINGW64__ && defined vsnprintf
#    undef vsnprintf
#  endif

int
asprintf(_Notnull_ char **destp,
         _Notnull_ _Printf_format_string_ char const *__restrict fmt,
         ...)
{
      int     len;
      va_list ap1;
      va_start(ap1, fmt);

      {
            va_list ap2;
            va_copy(ap2, ap1);
            len = vsnprintf(NULL, 0, fmt, ap2);
            va_end(ap2);
      }

      *destp = malloc(len + 1LLU);
      if (!*destp)
            err(1, "malloc");
      vsprintf(*destp, fmt, ap1);
      va_end(ap1);

      return len;
}
#endif
