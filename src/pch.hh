#pragma once
#ifndef HGUARD__PCH_HH_
#define HGUARD__PCH_HH_

#include "util/macros.h"

#if 0
#include <algorithm>
#include <filesystem>
#include <functional>
#include <ios>
#include <iosfwd>
#include <iterator>
#include <list>
#include <locale>
#include <map>
#include <new>
#include <queue>
#include <regex>
#include <scoped_allocator>
#include <stack>
#include <string_view>
#include <typeinfo>
#include <utility>
#include <vector>
#endif

#include <atomic>
#include <fstream>
#include <iostream>
#include <istream>
#include <limits>
#include <memory>
#include <ostream>
#include <sstream>
#include <string>

#include <exception>
#include <stdexcept>

#include <cassert>
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

/*--------------------------------------------------------------------------------------*/

#define FMT_HEADER_ONLY 1
#include <fmt/core.h>

#include <fmt/compile.h>
#include <fmt/format-inl.h>
#include <fmt/format.h>
#include <fmt/ranges.h>

/****************************************************************************************/
#endif
