#pragma once
#ifndef HGUARD_d_UTIL_HH_
#define HGUARD_d_UTIL_HH_
/****************************************************************************************/

// No idea why this happens sometimes.
#if __cplusplus < 202002L
#  undef __cplusplus
#  define __cplusplus 202002L
#endif
#include "Common.hh"
#include <filesystem>

namespace emlsp::util {

using std::filesystem::path;

path get_temporary_directory(char const *prefix = nullptr);
path get_temporary_filename(char const *prefix = nullptr, char const *suffix = nullptr);

namespace rpc {

extern socket_t open_new_socket(char const *path);
extern socket_t connect_to_socket(char const *path);

} // namespace rpc

namespace win32 {

void error_exit(wchar_t const *lpsz_function);

} // namespace win32

} // namespace emlsp::util

#ifndef restrict
#  define restrict __restrict
#endif

#include "util/c_util.h"

/****************************************************************************************/
#endif // Common.hh
// vim: ft=cpp
