// ReSharper disable CppInconsistentNaming
#include "myerr.h"

#include "c_util.h"

#include <pthread.h>

#define SHUTUPGCC __attribute__((__unused__)) \
      ssize_t const P99_PASTE(show_stacktrace_macro_variable_, __COUNTER__, _, __LINE__) =

#ifdef HAVE_EXECINFO_H
#  include <execinfo.h>
#define SHOW_STACKTRACE()                                                      \
      __extension__({                                                          \
            void     *arr[128];                                                \
            int const num = backtrace(arr, 128);                               \
            SHUTUPGCC write(2, SLS("\n<<< FATAL ERROR: STACKTRACE... >>>\n")); \
            backtrace_symbols_fd(arr, num, 2);                                 \
            SHUTUPGCC write(2, "\n", 1);                                       \
            fsync(2);                                                          \
      })
#else
#  define SHOW_STACKTRACE(...)
#endif

/*--------------------------------------------------------------------------------------*/

#if defined _WIN32
static CRITICAL_SECTION error_write_mtx;
INITIALIZER_HACK(mutex_init) { InitializeCriticalSection(&error_write_mtx); }
_Acquires_exclusive_lock_(*mtx)
static void dumb_wrapper_mutex_lock(_In_ CRITICAL_SECTION *mtx)   { EnterCriticalSection(mtx); }
_Releases_exclusive_lock_(*mtx)
static void dumb_wrapper_mutex_unlock(_In_ CRITICAL_SECTION *mtx) { LeaveCriticalSection(mtx); }
#else
static pthread_mutex_t error_write_mtx;
INITIALIZER_HACK(mutex_init) { pthread_mutex_init(&error_write_mtx, NULL); }
static void dumb_wrapper_mutex_lock(pthread_mutex_t *mtx)   { pthread_mutex_lock(mtx); }
static void dumb_wrapper_mutex_unlock(pthread_mutex_t *mtx) { pthread_mutex_unlock(mtx); }
#endif

static __inline void dump_error(int const errval)
{
      char        buf[128];
      char const *estr = my_strerror(errval, buf, 128);

      fprintf(stderr, ": %s\n", estr);
}

/*--------------------------------------------------------------------------------------*/

void
my_err_(_In_   bool const           print_err,
        _In_z_ char const *restrict file,
        _In_   int  const           line,
        _In_z_ char const *restrict func,
        _Printf_format_string_ _In_z_
        char const *restrict format,
        ...)
{
      va_list   ap;
      int const e = errno;
      dumb_wrapper_mutex_lock(&error_write_mtx);

      fprintf(stderr, "%s: (%s %d - %s): ", MAIN_PROJECT_NAME, file, line, func);
      va_start(ap, format);
      vfprintf(stderr, format, ap);
      va_end(ap);

      if (print_err)
            dump_error(e);
      else
            fputc('\n', stderr);

      fflush(stderr);
      SHOW_STACKTRACE();

      dumb_wrapper_mutex_unlock(&error_write_mtx);
      abort();
}

void
my_warn_(   _In_   bool const           print_err,
         UU _In_   bool const           force,
            _In_z_ char const *restrict file,
            _In_   int  const           line,
            _In_z_ char const *restrict func,
            _Printf_format_string_ _In_z_
            char const *restrict format,
            ...)
{
#ifdef NDEBUG
      if (!force)
            return;
#endif

      va_list   ap;
      int const e = errno;
      dumb_wrapper_mutex_lock(&error_write_mtx);

      fprintf(stderr, "%s: (%s %d - %s): ", MAIN_PROJECT_NAME, file, line, func);
      va_start(ap, format);
      vfprintf(stderr, format, ap);
      va_end(ap);

      if (print_err)
            dump_error(e);
      else
            fputc('\n', stderr);

      fflush(stderr);
      dumb_wrapper_mutex_unlock(&error_write_mtx);
}
