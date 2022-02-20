#pragma once
#ifndef HGUARD__PCH_HH_
#define HGUARD__PCH_HH_ //NOLINT

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#if 0
#include <any>
#include <charconv>
#include <codecvt>
#include <compare>
#include <forward_list>
#include <functional>
#include <iosfwd>
#include <list>
#include <numbers>
#include <numeric>
#include <optional>
#include <ratio>
#include <scoped_allocator>
#include <set>
#include <span>
#include <tuple>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <valarray>
#endif


#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <bitset>
#include <chrono>
#include <concepts>
#include <condition_variable>
#include <exception>
#include <execution>
#include <filesystem>
#include <fstream>
#include <future>
#include <iomanip>
#include <ios>
#include <iostream>
#include <istream>
#include <iterator>
#include <limits>
#include <locale>
#include <map>
#include <memory>
#include <mutex>
#include <new>
#include <ostream>
#include <queue>
#include <random>
#include <regex>
#include <sstream>
#include <stack>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <variant>
#include <vector>


#include <cassert>
#include <cerrno>
#include <cinttypes>
#include <climits>
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
#  include <Windows.h>
#  include <WinSock2.h>
#  include <afunix.h>
#  include <direct.h>
#  include <io.h>
#  include <process.h>
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

#include "rapid.hh"
#include <msgpack.hpp>

#include <event2/event.h>
#include <uv.h>

#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/dns.h>
#include <event2/http.h>
#include <event2/listener.h>
#include <event2/thread.h>

#include <boost/asio.hpp>
#include <boost/system.hpp>


#include <glib.h> //NOLINT
#include <gio/gio.h>
// clang-format on

// #include <uvw.hpp>

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

// #include <rapidjson/rapidjson.h>

#include "util/formatters.hh"

/****************************************************************************************/
#endif
