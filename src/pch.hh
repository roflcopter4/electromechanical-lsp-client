#pragma once
#ifndef HGUARD_d_PCH_HH_
#define HGUARD_d_PCH_HH_

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif
#include "util/macros.h"

#include <algorithm>
#include <any>
#include <array>
#include <atomic>
#include <bit>
#include <bitset>
#include <charconv>
#include <chrono>
#include <codecvt>
#include <compare>
#include <concepts>
#include <condition_variable>
#include <exception>
#include <execution>
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

#if 0
#include <atomic>
#include <iostream>
#include <limits>
#include <memory>
#include <sstream>
#include <string>
#endif


#include <cassert>
#include <cerrno>
#include <cinttypes>
#include <csignal>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

/*--------------------------------------------------------------------------------------*/

#if defined DOSISH
#  define WIN32_LEAN_AND_MEAN 1
#  include <Windows.h>
#  include <winsock2.h>

#  include <afunix.h>
#  include <direct.h>
#  include <io.h>
#  include <process.h>
#else
#  include <spawn.h>
#  include <sys/socket.h>
#  include <sys/un.h>
#  include <sys/wait.h>
#  include <unistd.h>
#endif

#include <fcntl.h>
#include <sys/stat.h>

/*--------------------------------------------------------------------------------------*/

#include <fmt/core.h>

#include <fmt/compile.h>
#include <fmt/format-inl.h>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <fmt/printf.h>


#ifdef NDEBUG
#  define RAPIDJSON_ASSERT(x)
#  define RAPIDJSON_NOEXCEPT_ASSERT(x)
#else
#  define RAPIDJSON_ASSERT_THROWS
#  define RAPIDJSON_NOEXCEPT_ASSERT(x) assert(x)
#  define RAPIDJSON_ASSERT(x)                                    \
      do {                                                       \
            if (!(x)) [[unlikely]] {                             \
                  errx(1, "rapidjson assertion failed: %s", #x); \
            }                                                    \
      } while (0)
#endif

//#include "rapid.hh"
#include <rapidjson/rapidjson.h>

/****************************************************************************************/
#endif
