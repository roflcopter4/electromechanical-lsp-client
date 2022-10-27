#ifndef HGUARD_d_C_UTIL_HH_
#define HGUARD_d_C_UTIL_HH_
#pragma once
/****************************************************************************************/

#include "Common.hh"

#if defined __GNUC__ || defined __clang__
# ifdef __clang__
#  define ATTRIBUTE_PRINTF(...) __attribute__((__format__(__printf__, __VA_ARGS__)))
# else
#  define ATTRIBUTE_PRINTF(...) __attribute__((__format__(__gnu_printf__, __VA_ARGS__)))
# endif
#else
# define ATTRIBUTE_PRINTF(...)
#endif
#ifdef _MSC_VER
#else
# define _Maybenull_ //NOLINT
# ifndef _Check_return_
#  define _Check_return_ __attribute__((__warn_unused_result__)) //NOLINT
# endif
# ifndef _Printf_format_string_
#  define _Printf_format_string_
# endif
#endif
#define PFSAL _Printf_format_string_

#if !defined restrict && (defined __cplusplus || defined __RESHARPER__)
# define restrict __restrict
#endif


__BEGIN_DECLS
/*--------------------------------------------------------------------------------------*/


_Check_return_ char const *
my_strerror(_In_                 errno_t errval,
            _Out_writes_(buflen) char   *buf,
            _In_                 size_t  buflen)
    __attribute__((__nonnull__(2), __warn_unused_result__));


/**
 * \brief
 * Creates a pseudorandom filename. The buffer should be empty; no template is required.
 * Instead, provide the desired directory for the file and it a name will be created
 * there. Optionally, a prefix and/or suffix may be provided for the filename.
 *
 * NOT FOR USE IN GENERATING FILENAMES TO BE USED DIRECTLY IN A WORLD WRITABLE
 * TEMPORARY DIRECTORY SUCH AS '/tmp`!
 *
 * It is safe only if the directory you provide has been created securely by a function
 * like mkdtemp, or otherwise guarantee that the file you're creating can not be
 * created * by something else before you do it.
 *
 * \param buf An empty char buffer which should be of size <code>PATH_MAX + 1</code>.
 * \param dir The base directory for the file. May <b>not</b> be <code>NULL</code>.
 *            May be an empty string.
 * \param prefix Optional prefix for the filename. May be <code>NULL</code>.
 * \param suffix Optional suffix for the filename. May be <code>NULL</code>.
 * \return Length of the generated filename.
 */
extern size_t braindead_tempname(_Out_ _Post_z_ char       *__restrict buf,
                                 _In_z_         char const *__restrict dir,
                                 _In_opt_z_     char const *__restrict prefix,
                                 _In_opt_z_     char const *__restrict suffix)
    __attribute__((__nonnull__(1, 2)))
    __attr_access((__write_only__, 1)) __attr_access((__read_only__, 2))
    __attr_access((__read_only__,  3)) __attr_access((__read_only__, 4));


#ifndef HAVE_ASPRINTF
/**
 * \brief Laziest possible asprintf implementation. But it works, assuming (v)snprintf
 * conforms to C99. Microsoft finally provides one, so this should be portable.
 * \param destp Address of an uninitialized C string. Will be initialized with malloc.
 * \param fmt The format string.
 * \param ... Arguments
 * \return Number of characters written.
 */
extern int asprintf(_Outptr_result_z_ char       **__restrict destp,
                    _In_z_ PFSAL      char const  *__restrict fmt,
                    ...)
    __attribute__((__nonnull__)) ATTRIBUTE_PRINTF(2, 3);
#endif

#ifdef HAVE_STRLCPY
_Check_return_ __always_inline
size_t emlsp_strlcpy(_Notnull_ char       *__restrict dst,
                     _Notnull_ char const *__restrict src,
                     size_t                           size)
    __attribute__((__nonnull__, __artificial__, __warn_unused_result__))
    __attr_access((__write_only__, 1))
    __attr_access((__read_only__, 2))
{
      return strlcpy(dst, src, size);
}
#else
extern size_t emlsp_strlcpy(_Out_writes_z_(size) char       *__restrict dst,
                            _In_z_               char const *__restrict src,
                            _In_                 size_t                 size)
    __attribute__((__nonnull__(1, 2)))
    __attr_access((__write_only__, 1))
    __attr_access((__read_only__,  2));

# ifndef __cplusplus
#  define strlcpy(dst, src, size) emlsp_strlcpy((dst), (src), (size))
# endif
#endif

#ifndef HAVE_DPRINTF
extern int vdprintf(_In_ int fd,
                    _In_z_ _Printf_format_string_
                    char const *__restrict format,
                    _In_ va_list ap);
extern int dprintf(_In_ int fd,
                   _In_z_ _Printf_format_string_
                   char const *__restrict format,
                   ...)
      ATTRIBUTE_PRINTF(2, 3);
#endif


extern _Check_return_ uint32_t emlsp_cxx_random_device_get_random_val(void)
      __attribute__((__visibility__("hidden"), __warn_unused_result__));
// Get random 32 bit value. Duh.
extern _Check_return_ uint32_t emlsp_cxx_random_engine_get_random_val_32(void)
      __attribute__((__visibility__("hidden"), __warn_unused_result__));
// Get random 64 bit value. Duh.
extern _Check_return_ uint64_t emlsp_cxx_random_engine_get_random_val_64(void)
      __attribute__((__visibility__("hidden"), __warn_unused_result__));


#ifdef _WIN32
extern _Check_return_ DWORD getppid(void)
      __attribute__((__warn_unused_result__));

extern _Success_(return == 0) int fsync(_In_ int descriptor);

# ifndef __cplusplus
extern NORETURN void emlsp_win32_error_exit_message_w(_In_z_ wchar_t const *msg);
extern          void emlsp_win32_dump_backtrace(_In_ FILE *fp);
# endif
#endif


/*--------------------------------------------------------------------------------------*/
__END_DECLS


#undef PFSAL
#undef ATTRIBUTE_PRINTF

/****************************************************************************************/
#endif // c_util.h
// vim: ft=c
