#include "Common.hh"
#include "util/c/c_util.h"

#define MINOF(a, b) (((a) < (b)) ? (a) : (b))
#define MAXOF(a, b) (((a) > (b)) ? (a) : (b))

/****************************************************************************************/

#ifndef HAVE_ASPRINTF
# if defined vsnprintf && !defined __MINGW32__ && !defined __MINGW64__
#  undef vsnprintf
# endif

int
asprintf(_Outptr_result_z_             char      **restrict destp,
         _In_z_ _Printf_format_string_ char const *restrict fmt,
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
            err("malloc()");
      vsprintf(*destp, fmt, ap1);
      va_end(ap1);

      return len;
}
#endif

/****************************************************************************************/

#if defined __GNUC__
  /* This also works with clang. */
# pragma GCC diagnostic push
# pragma GCC diagnostic error "-Wint-conversion"
# define DIAG_POP() _Pragma("GCC diagnostic pop")
#else
# define DIAG_POP()
#endif
PRAGMA_GCC("GCC diagnostic push")
PRAGMA_GCC("GCC diagnostic error \"-Wint-conversion\"")

_Check_return_ char const *
my_strerror(_In_                 errno_t errval,
            _Out_writes_(buflen) char   *buf,
            _In_                 size_t  buflen)
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

PRAGMA_GCC("GCC diagnostic pop")
DIAG_POP()

/****************************************************************************************/

#ifndef HAVE_STRLCPY
size_t
emlsp_strlcpy(_Out_writes_z_(size) char       *restrict dst,
              _In_z_               char const *restrict src,
              _In_                 size_t const         size)
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

/****************************************************************************************/

#ifndef HAVE_DPRINTF

int
vdprintf(_In_ int const fd,
         _In_z_ _Printf_format_string_
         char const *const __restrict format,
         _In_ va_list ap)
{
      FILE     *fp  = fdopen(fd, "wb");
      int const ret = vfprintf(fp, format, ap);
      fclose(fp);
      return ret;
}

int
dprintf(_In_ int const fd,
        _In_z_ _Printf_format_string_
        char const *const __restrict format,
        ...)
{
      va_list ap;
      va_start(ap, format);
      int const ret = vdprintf(fd, format, ap);
      va_end(ap);
      return ret;
}

#endif


/****************************************************************************************/

#ifdef _WIN32
/*
 * Credit: https://gist.github.com/mattn/253013/d47b90159cf8ffa4d92448614b748aa1d235ebe4
 */

# include <Windows.h>
# include <TlHelp32.h>

_Check_return_
DWORD getppid(void)
{
      bool           found     = false;
      DWORD          ppid      = 0;
      void *const    hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

      if (hSnapshot && hSnapshot == INVALID_HANDLE_VALUE)
      {
            PROCESSENTRY32 pe32 = {.dwSize = sizeof pe32};
            DWORD const    pid  = GetCurrentProcessId();
            BOOL           ok   = Process32First(hSnapshot, &pe32);

            while (ok) {
                  if (pid == pe32.th32ProcessID) {
                        ppid  = pe32.th32ParentProcessID;
                        found = true;
                        break;
                  }
                  ok = Process32Next(hSnapshot, &pe32);
            }

            CloseHandle(hSnapshot);

            if (found)
                  return ppid;
      }

      emlsp_win32_error_exit_message_w(
          L"getppid() failed to find parent ID. Because getppid() *must* always "
          L"succeed, this error is fatal.");
}

/*--------------------------------------------------------------------------------------*/

_Success_(return == 0)
int fsync(_In_ int const descriptor)
{
      return (FlushFileBuffers((HANDLE)(uintptr_t)_get_osfhandle(descriptor)) == 0)
                 ? -1
                 :  0;
}
#endif
