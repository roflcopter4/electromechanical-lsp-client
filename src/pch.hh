#pragma once
#ifndef HGUARD__PCH_HH_
#define HGUARD__PCH_HH_ //NOLINT

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#ifdef DEBUG
# define _LIBCPP_DEBUG_LEVEL 2
# define _GLIBCXX_ASSERTIONS 1
// # define _GLIBCXX_PARALLEL 1
// # define _GLIBCXX_PARALLEL_ASSERTIONS 1
# undef NDEBUG
#endif

#define __PTW32_CLEANUP_C 1
//#define __PTW32_CLEANUP_CXX 1

// #include "all_the_clang.hh"

#include <algorithm>
#include <any>
#include <array>
#include <atomic>
#include <bit>
#include <bitset>
#include <chrono>
#include <codecvt>
#include <compare>
#include <concepts>
#include <condition_variable>
#include <exception>
#include <filesystem>
#include <forward_list>
#include <fstream>
#include <functional>
#include <future>
#include <iomanip>
#include <ios>
#include <iosfwd>
#include <iostream>
#include <istream>
#include <iterator>
#include <limits>
#include <list>
#include <locale>
#include <map>
#include <memory>
#include <mutex>
#include <new>
#include <numbers>
#include <numeric>
#include <optional>
#include <ostream>
#include <queue>
#include <random>
#include <ratio>
#include <regex>
#include <scoped_allocator>
#include <set>
#include <span>
#include <sstream>
#include <stack>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <tuple>
#include <type_traits>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <valarray>
#include <variant>
#include <vector>

#include <cassert>
#include <cerrno>
#include <cinttypes>
#include <climits>
#include <csetjmp>
#include <csignal>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

/*--------------------------------------------------------------------------------------*/

#if defined _WIN32
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN 1
#  endif
#  include <sal.h>
#  include <Windows.h>
#  include <WinSock2.h>
#  include <WS2tcpip.h>
#  include <WSPiApi.h>
#  include <afunix.h>
#  include <direct.h>
#  include <io.h>
#  include <process.h>
#  include <processthreadsapi.h>
#  include <detours/detours.h>
#else
#  include <netdb.h>
#  include <netinet/in.h>
#  include <netinet/ip.h>
#  include <netinet/ip6.h>
#  include <spawn.h>
#  include <sys/ioctl.h>
#  include <sys/resource.h>
#  include <sys/socket.h>
#  include <sys/un.h>
#  include <sys/wait.h>
#  include <unistd.h>
#endif

#include <fcntl.h>
#include <sys/stat.h>

#ifndef _MSC_VER
# include <pthread.h>
#endif

/*--------------------------------------------------------------------------------------*/

// clang-format off
#include <fmt/core.h>
#include <fmt/format-inl.h>
#include <fmt/format.h>

#include <fmt/chrono.h>
#include <fmt/color.h>
#include <fmt/compile.h>
#include <fmt/os.h>
#include <fmt/ostream.h>
#include <fmt/printf.h>
#include <fmt/ranges.h>
#include <fmt/xchar.h>

#include "util/macros.h"
#include "util/types.h"
#include "util/util.hh"

#include "rapid.hh"

#include <msgpack.hpp>
#include <uv.h>

#include <glib.h> //NOLINT
#include <gio/gio.h>
// clang-format on

#include "util/my_glib.hh"

#if 0
#ifdef NDEBUG
#  define RAPIDJSON_ASSERT(x)
#  define RAPIDJSON_NOEXCEPT_ASSERT(x)
#else
#  define RAPIDJSON_ASSERT_THROWS 1
#  define RAPIDJSON_NOEXCEPT_ASSERT(x) assert(x)
#  define RAPIDJSON_ASSERT(x)                                    \
      do {                                                       \
            if (!(x)) [[unlikely]] {                             \
                  errx(1, "rapidjson assertion failed: %s", #x); \
            }                                                    \
      } while (0)
#endif
#endif

#ifdef __MINGW64__
extern "C" __declspec(dllimport) HRESULT __stdcall
SetThreadDescription (
    _In_ HANDLE hThread,
    _In_ PCWSTR lpThreadDescription
);
#endif

// #include <rapidjson/rapidjson.h>

/****************************************************************************************/
#endif
