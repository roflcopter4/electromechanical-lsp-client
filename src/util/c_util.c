#include "Common.hh"
#include "util/c_util.h"

#define MINOF(a, b) (((a) < (b)) ? (a) : (b))
#define MAXOF(a, b) (((a) > (b)) ? (a) : (b))

#ifndef HAVE_ASPRINTF
# if !defined __MINGW32__ && !defined __MINGW64__ && defined vsnprintf
#  undef vsnprintf
# endif

/*
 * Laziest possible asprintf implementation. But it works, assuming (v)snprintf
 * conforms to C99. Microsoft's current one is supposed to so I won't bother checking.
 */
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

      *destp = malloc(len + SIZE_C(1));
      if (!*destp)
            err(1, "malloc");
      vsprintf(*destp, fmt, ap1);
      va_end(ap1);

      return len;
}
#endif


#if defined __GNUC__
  /* This also works with clang. */
# pragma GCC diagnostic push
# pragma GCC diagnostic error "-Wint-conversion"
# define DIAG_POP() _Pragma("GCC diagnostic pop")
#else
# define DIAG_POP()
#endif

_Check_return_ char const *
my_strerror(errno_t const errval, _Notnull_ char *buf, size_t const buflen)
{
      char const *estr;

#if defined HAVE_STRERROR_S || defined _MSC_VER
      strerror_s(buf, buflen, errval);
      estr = buf;
#elif defined HAVE_STRERROR_R
# if defined __GLIBC__ && defined _GNU_SOURCE && defined __USE_GNU
      estr = strerror_r(errval, buf, buflen);
# else
      int const r = strerror_r(errval, buf, buflen);
      (void)r;
      estr = buf;
# endif
#else
      estr = strerror(errval);
#endif

      return estr;
}

DIAG_POP()


#ifndef HAVE_STRLCPY
extern size_t emlsp_strlcpy(_Notnull_ char       *restrict dst,
                            _Notnull_ char const *restrict src,
                                      size_t const         size)
{
      /* Do it the stupid way. It's, frankly, probably faster anyway. */
      size_t const slen = strlen(src);

      if (size) {
            size_t const len = MINOF(slen, size - SIZE_C(1));
            memcpy(dst, src, len);
            dst[len] = '\0';
      }

      return slen;  // Does not include NUL.
}
#endif
