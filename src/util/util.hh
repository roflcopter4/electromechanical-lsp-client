#ifndef HGUARD_d_UTIL_HH_
#define HGUARD_d_UTIL_HH_
#pragma once
/****************************************************************************************/

// No idea why this happens sometimes.
#include "Common.hh"

inline namespace emlsp {
namespace util {

using std::filesystem::path;

extern path get_temporary_directory(char const *prefix = nullptr);
extern path get_temporary_filename(char const *prefix = nullptr, char const *suffix = nullptr);

extern std::vector<char> slurp_file(char const *fname);
inline std::vector<char> slurp_file(std::string const &fname) { return slurp_file(fname.c_str()); }
inline std::vector<char> slurp_file(path const &fname)        { return slurp_file(fname.string().c_str()); }

std::string char_repr(char const ch);

template <typename V,
          std::enable_if_t<!std::is_pointer<V>::value, bool> = true>
inline void resize_vector_hack(V &vec, size_t const new_size)
{
      struct dummy {
            typename V::value_type v;
            /* The constructor must be an empty function for this to work. Setting to
             * `default' results in allocated memory being initialized anyway. */
            dummy() {} // NOLINT
      };
      static_assert(sizeof(dummy[10]) == sizeof(typename V::value_type[10]),
                    "alignment error");

      using V2 = std::vector<dummy, typename std::allocator_traits<
                                        typename V::allocator_type>::template rebind_alloc<dummy>>;

      reinterpret_cast<V2 &>(vec).resize(new_size);
}

template<typename T>
concept Stringable = std::convertible_to<T, std::string> || std::same_as<T, std::string>;

template<typename T>
concept NonStringable = !std::convertible_to<T, std::string> && !std::same_as<T, std::string>;

template<typename T>
concept StringLiteral = std::convertible_to<T, char const (&)[]> || std::convertible_to<T, char (&)[]>;

template<typename T>
concept NonStringLiteral = !std::convertible_to<T, char const (&)[]> && !std::convertible_to<T, char (&)[]>;

namespace ipc {

extern socket_t open_new_socket(char const *path);
extern socket_t connect_to_socket(char const *path);
extern socket_t connect_to_socket(sockaddr_un const *addr);

} // namespace ipc

namespace win32 {

void error_exit(wchar_t const *lpsz_function);

} // namespace win32
} // namespace util
} // namespace emlsp

#if defined __cplusplus && !defined restrict
#  define restrict __restrict
#endif

#include "util/c_util.h"

/****************************************************************************************/
#endif // Common.hh
// vim: ft=cpp
