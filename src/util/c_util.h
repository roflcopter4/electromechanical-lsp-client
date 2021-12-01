#pragma once
#ifndef HGUARD_d_C_UTIL_HH_
#define HGUARD_d_C_UTIL_HH_
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
# define PFSAL _Printf_format_string_
#else
# define PFSAL
# define _Maybenull_ //NOLINT
#endif


__BEGIN_DECLS


/**
 * \brief
 * Creates a pseudorandom filename. The buffer should be empty; no template is required.
 * Instead, provide the desired directory for the file and it a name will be created
 * there. Optionally, a prefix and/or suffix may be provided for the filename.
 *
 * NOTE: NOT FOR USE IN GENERATING FILENAMES TO BE USED DIRECTLY IN A WORLD WRITABLE
 * TEMPORARY DIRECTORY SUCH AS /tmp! This function is totally insecure and employs rand()
 * for simplicity.
 *
 * It is only safe if the directory you provide has been created securely by a function
 * like mkdtemp, or otherwise guarantee that the file you're creating does not already
 * exist.
 *
 * \param buf An empty char buffer which should be of size PATH_MAX + 1.
 * \param dir The base directory for the file. May not be NULL. May be an empty string.
 * \param prefix Optional prefix for the filename. May be NULL.
 * \param suffix Optional suffix for the filename. May be NULL.
 * \return Pointer to the provided buffer with the generated filename.
 */
extern char *braindead_tempname(_Notnull_   char       *restrict buf,
                                _Notnull_   char const *restrict dir,
                                _Maybenull_ char const *restrict prefix,
                                _Maybenull_ char const *restrict suffix)
    __attribute__((__nonnull__(1, 2)));

#if !defined HAVE_ASPRINTF
extern int asprintf(_Notnull_       char      **destp,
                    _Notnull_ PFSAL char const *restrict fmt,
                    ...)
    __attribute__((__nonnull__)) ATTRIBUTE_PRINTF(2, 3);
#endif


extern unsigned cxx_random_device_get_random_val(void);
extern uint32_t cxx_random_engine_get_random_val(void);


__END_DECLS


#undef PFSAL
#undef ATTRIBUTE_PRINTF

/****************************************************************************************/
#endif // Common.hh
// vim: ft=c
