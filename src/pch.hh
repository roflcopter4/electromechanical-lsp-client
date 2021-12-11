#pragma once
#ifndef HGUARD__PCH_HH_
#define HGUARD__PCH_HH_

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif
#include "util/macros.h"

#if 0
#include <any>
#include <execution>
#include <bit>
#include <bitset>
#include <charconv>
#include <chrono>
#include <codecvt>
#include <compare>
#include <forward_list>
#include <functional>
#include <future>
#include <ios>
#include <iosfwd>
#include <limits>
#include <list>
#include <locale>
#include <numbers>
#include <numeric>
#include <optional>
#include <ratio>
#include <regex>
#include <scoped_allocator>
#include <set>
#include <span>
#include <tuple>
#include <type_traits>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <valarray>
#endif

#include <algorithm>
#include <array>
#include <atomic>
#include <concepts>
#include <condition_variable>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <istream>
#include <iterator>
#include <map>
#include <memory>
#include <mutex>
#include <new>
#include <ostream>
#include <queue>
#include <random>
#include <sstream>
#include <stack>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
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
