#pragma once
#ifndef HGUARD_d_C_UTIL_HH_
#define HGUARD_d_C_UTIL_HH_
/****************************************************************************************/

#include "Common.hh"

__BEGIN_DECLS


/**
 * Creates a pseudorandom filename. The buffer should be empty; no template is required.
 * Instead, provide the desired directory for the file and it a name will be created
 * there. Optionally, a preffix and/or suffix may be provided for the filename.
 *
 * NOTE: NOT FOR USE IN GENERATING FILENAMES TO BE USED DIRECTLY IN A WORLD WRITABLE
 * TEMPORARY DIRECTORY SUCH AS /tmp! This function is totally insecure and employs rand()
 * for simplicity.
 * It is only safe if the directory you provide has been created securely by a function
 * like mkdtemp.
 */
extern char *even_dumber_tempname(_Notnull_ char       *restrict buf,
                                  _Notnull_ char const *restrict dir,
                                            char const *restrict prefix,
                                            char const *restrict suffix)
    __attribute__((__nonnull__(1, 2)));


extern unsigned cxx_random_device_get_random_val(void);


__END_DECLS

/****************************************************************************************/
#endif // Common.hh
// vim: ft=c
