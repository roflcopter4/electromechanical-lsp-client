// ReSharper disable CppInconsistentNaming

// clang-tidy off
#include "util/c/myerr.h"
#include "util/c/c_util.h"
// clang-tidy on

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

#ifdef _WIN32
//# define LOCK_FILE(x) _lock_file(x)
//# define UNLOCK_FILE(x) _unlock_file(x)
# define LOCK_FILE(x)
# define UNLOCK_FILE(x)
#else
# define LOCK_FILE(x) flockfile(x)
# define UNLOCK_FILE(x) funlockfile(x)
#endif

#define CSTR_RESTRICT _In_z_ char const *const __restrict
#define FORMAT_STR    _In_z_ _Printf_format_string_ char const *const __restrict

/*--------------------------------------------------------------------------------------*/

#if defined _WIN32

static CRITICAL_SECTION error_write_mtx;

INITIALIZER_HACK(err_c_mutex_init)
{
      InitializeCriticalSection(&error_write_mtx);
}

_Acquires_exclusive_lock_(*mtx) static __inline void
dumb_wrapper_mutex_lock(_In_ CRITICAL_SECTION *mtx)
{
      EnterCriticalSection(mtx);
}

_Releases_exclusive_lock_(*mtx) static __inline void
dumb_wrapper_mutex_unlock(_In_ CRITICAL_SECTION *mtx)
{
      LeaveCriticalSection(mtx);
}

#else

# include <pthread.h>
static pthread_mutex_t error_write_mtx;

INITIALIZER_HACK(mutex_init)
{
      pthread_mutex_init(&error_write_mtx, NULL);
}

static __inline void
dumb_wrapper_mutex_lock(pthread_mutex_t *mtx)
{
      pthread_mutex_lock(mtx);
}

static __inline void
dumb_wrapper_mutex_unlock(pthread_mutex_t *mtx)
{
      pthread_mutex_unlock(mtx);
}

#endif


/*--------------------------------------------------------------------------------------*/


ND extern size_t emlsp_util_cxxdemangle(_In_z_ char const *raw_name, _Out_ char *buffer)
    __attribute__((__nonnull__));


static __inline void
dump_error(int const errval)
{
      char        buf[128];
      char const *estr = my_strerror(errval, buf, 128);
      fprintf(stderr, ": %s\n", estr);
}


/****************************************************************************************/


NORETURN void
emlsp_my_err_(_In_ bool const print_err,
              CSTR_RESTRICT   file,
              _In_ int const  line,
              CSTR_RESTRICT   func,
              FORMAT_STR      format,
              ...)
{
      va_list   ap;
      int const e = errno;
      dumb_wrapper_mutex_lock(&error_write_mtx);
      LOCK_FILE(stderr);

      fprintf(stderr, "\n" MAIN_PROJECT_NAME ": (%s %d - %s): ", file, line, func);
      va_start(ap, format);
      vfprintf(stderr, format, ap);
      va_end(ap);

      if (print_err)
            dump_error(e);
      else
            putc('\n', stderr);

#if 0
#ifdef _MSC_VER
      __try {
#endif
      g_on_error_stack_trace("");
#ifdef _MSC_VER
      } __except (EXCEPTION_EXECUTE_HANDLER) {}
#endif
#endif
      if (e)
            emlsp_win32_dump_backtrace(stderr);

      fflush(stderr);
      UNLOCK_FILE(stderr);
      dumb_wrapper_mutex_unlock(&error_write_mtx);

      if (e)
            abort();
      exit(0);
}


void
emlsp_my_warn_(_In_ bool const print_err,
            UU _In_ bool const force,
               CSTR_RESTRICT   file,
               _In_ int const  line,
               CSTR_RESTRICT   func,
               FORMAT_STR      format,
               ...)
{
#ifdef NDEBUG
      if (!force)
            return;
#endif

      va_list   ap;
      int const e = errno;
      dumb_wrapper_mutex_lock(&error_write_mtx);
      LOCK_FILE(stderr);

      fprintf(stderr, "\n" MAIN_PROJECT_NAME ": (%s %d - %s): ", file, line, func);
      va_start(ap, format);
      vfprintf(stderr, format, ap);
      va_end(ap);

      if (print_err)
            dump_error(e);
      else
            fputc('\n', stderr);

      fflush(stderr);
      UNLOCK_FILE(stderr);
      dumb_wrapper_mutex_unlock(&error_write_mtx);
}
