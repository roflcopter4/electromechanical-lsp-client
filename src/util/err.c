#include "myerr.h"

#include <pthread.h>

#define DEBUG true

#define SHUTUPGCC __attribute__((__unused__)) ssize_t n =

#ifdef HAVE_EXECINFO_H
#  include <execinfo.h>
#define SHOW_STACKTRACE()                              \
      __extension__({                                  \
            void  *arr[128];                           \
            size_t num = backtrace(arr, 128);          \
            (void)write(2, SLS("<<< FATAL ERROR >>>\n" \
                               "    STACKTRACE:\n"));  \
            backtrace_symbols_fd(arr, num, 2);         \
            (void)write(2, "\n", 1);                   \
            fsync(2);                                  \
      })
#else
#  define SHOW_STACKTRACE(...)
#endif

extern mtx_t util_c_error_write_mutex;
mtx_t util_c_error_write_mutex;

INITIALIZER_HACK(mutex_init)
{
      mtx_init(&util_c_error_write_mutex, mtx_plain);
}

void
my_err_(int  const           status,
        bool const           print_err,
        char const *restrict file,
        int  const           line,
        char const *restrict func,
        char const *restrict format,
        ...)
{
      va_list   ap;
      int const e = errno;
      mtx_lock(&util_c_error_write_mutex);

      fprintf(stderr, "%s: (%s %d - %s): ", MAIN_PROJECT_NAME, file, line, func);
      va_start(ap, format);
      vfprintf(stderr, format, ap);
      va_end(ap);

      if (print_err)
            fprintf(stderr, ": %s\n", strerror(e));
      else
            putc('\n', stderr);

      fflush(stderr);
      SHOW_STACKTRACE();

      mtx_unlock(&util_c_error_write_mutex);
      exit(status);
}

void
my_warn_(bool const           print_err,
         bool const           force,
         char const *restrict file,
         int  const           line,
         char const *restrict func,
         char const *restrict format,
         ...)
{
      if (!DEBUG && !force)
            return;

      va_list   ap;
      int const e = errno;
      mtx_lock(&util_c_error_write_mutex);

      fprintf(stderr, "%s: (%s %d - %s): ", MAIN_PROJECT_NAME, file, line, func);
      va_start(ap, format);
      vfprintf(stderr, format, ap);
      va_end(ap);

      if (print_err)
            fprintf(stderr, ": %s\n", strerror(e));
      else
            fputc('\n', stderr);

      fflush(stderr);
      mtx_unlock(&util_c_error_write_mutex);
}
