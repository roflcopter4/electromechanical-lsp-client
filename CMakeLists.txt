cmake_minimum_required (VERSION 3.20.0)

# set (MAIN_PROJECT_VERSION_MAJOR      0)
# set (MAIN_PROJECT_VERSION_MINOR      0)
# set (MAIN_PROJECT_VERSION_PATCHLEVEL 0)
# string (JOIN "." MAIN_PROJECT_VERSION MAIN
#     PROJECT_VERSION_MAJOR
#     PROJECT_VERSION_MINOR
#    PROJECT_VERSION_PATCH)

project (electromechanical-lsp LANGUAGES C CXX VERSION 0.0.1)
include (CheckFunctionExists)
include (CheckSymbolExists)
include (CheckIncludeFile)


###############################################################################
# Options

option(USE_JEMALLOC "Use jemalloc" OFF)

macro(FIX_WINDOWS_PATHS _pathvar)
    string(REPLACE "\\" "/" ${_pathvar} "${${_pathvar}}")
endmacro()

if (NOT CMAKE_BUILD_TYPE)
    message("Setting build type to \"Release\"")
    set (CMAKE_BUILD_TYPE "Release")
endif()

if ((CMAKE_BUILD_TYPE STREQUAL "Debug") OR
    (CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo"))
    set (DEBUG 1)
endif()

###############################################################################
# Includes and defines

if (MINGW)
    add_definitions(-D__MINGW__)
    add_definitions(-D__USE_MINGW_ANSI_STDIO=1)
endif()
if (MSVC)
    set (CMAKE_REQUIRED_DEFINITIONS DHAVE_CONFIG_H)
    add_definitions(-DHAVE_CONFIG_H)
else()
    set (CMAKE_REQUIRED_DEFINITIONS -D__USE_ISOC99 -D__USE_ISOC11
         -D__USE_ISOCXX11 -D_GNU_SOURCE -DHAVE_CONFIG_H)
endif()

add_definitions(-DHAVE_CONFIG_H)
include_directories("${CMAKE_CURRENT_BINARY_DIR}")
include_directories("${PROJECT_SOURCE_DIR}/src")

set (CMAKE_CXX_STANDARD 20)
set (CMAKE_CXX_STANDARD_REQUIRED TRUE)

###############################################################################

list(APPEND CMAKE_MODULE_PATH
     "${CMAKE_SOURCE_DIR}/cmake"
     "${CMAKE_SOURCE_DIR}/cmake/gnome-cmake/modules")

if (WIN32)
    if (NOT VCPKG_ROOT)
        message(FATAL_ERROR "Must provide a definition for VCPKG_ROOT (ie the directory with vcpkg.exe")
    endif()
    message( "INSTALLED DIR IS \"${VCPKG_ROOT}\"" )
    list(APPEND CMAKE_MODULE_PATH "${VCPKG_ROOT}/scripts/buildsystems")
    include (vcpkg)
endif()

set(THREADS_PREFER_PTHREAD_FLAG TRUE)
set(CMAKE_THREAD_PREFER_PTHREAD TRUE)

find_package (Threads REQUIRED)
find_package (fmt REQUIRED)
find_package (RapidJSON REQUIRED)
find_package (nlohmann_json REQUIRED)
find_package (GLib REQUIRED)

include_directories(fmt_INCLUDE_DIRECTORIES)

if (USE_JEMALLOC)
    find_package (Jemalloc REQUIRED)
endif()

###############################################################################
# Symbol and header checks

CHECK_SYMBOL_EXISTS (arc4random     "stdlib.h"   HAVE_ARC4RANDOM)
CHECK_SYMBOL_EXISTS (mkstemp        "stdlib.h"   HAVE_MKSTEMP)
CHECK_SYMBOL_EXISTS (mkdtemp        "stdlib.h"   HAVE_MKDTEMP)
CHECK_SYMBOL_EXISTS (mkostemps      "stdlib.h"   HAVE_MKOSTEMPS)
CHECK_SYMBOL_EXISTS (asprintf       "stdio.h"    HAVE_ASPRINTF)
CHECK_SYMBOL_EXISTS (dprintf        "stdio.h"    HAVE_DPRINTF)
CHECK_SYMBOL_EXISTS (stricmp        "string.h"   HAVE_STRICMP)
CHECK_SYMBOL_EXISTS (strlcat        "string.h"   HAVE_STRLCAT)
CHECK_SYMBOL_EXISTS (strlcpy        "string.h"   HAVE_STRLCPY)
CHECK_SYMBOL_EXISTS (strsep         "string.h"   HAVE_STRSEP)
CHECK_SYMBOL_EXISTS (fork           "unistd.h"   HAVE_FORK)
CHECK_SYMBOL_EXISTS (pause          "unistd.h"   HAVE_PAUSE)
CHECK_SYMBOL_EXISTS (pipe2          "unistd.h"   HAVE_PIPE2)
CHECK_SYMBOL_EXISTS (posix_spawnp   "spawn.h"    HAVE_POSIX_SPAWNP)
CHECK_SYMBOL_EXISTS (strcasecmp     "strings.h"  HAVE_STRCASECMP)
CHECK_SYMBOL_EXISTS (err            "err.h"      HAVE_ERR)

CHECK_SYMBOL_EXISTS (clock_gettime  "time.h"     HAVE_CLOCK_GETTIME)
CHECK_SYMBOL_EXISTS (timespec_get   "time.h"     HAVE_TIMESPEC_GET)
CHECK_SYMBOL_EXISTS (nanosleep      "time.h"     HAVE_NANOSLEEP)

CHECK_INCLUDE_FILE ("execinfo.h"    HAVE_EXECINFO_H)
CHECK_INCLUDE_FILE ("intrin.h"      HAVE_INTRIN_H)
CHECK_INCLUDE_FILE ("stdatomic.h"   HAVE_STDATOMIC_H)
CHECK_INCLUDE_FILE ("stdnoreturn.h" HAVE_STDNORETURN_H)
CHECK_INCLUDE_FILE ("sys/auxv.h"    HAVE_SYS_AUXV_H)
CHECK_INCLUDE_FILE ("threads.h"     HAVE_THREADS_H)
CHECK_INCLUDE_FILE ("unistd.h"      HAVE_UNISTD_H)
CHECK_INCLUDE_FILE ("x86intrin.h"   HAVE_X86_INTRIN_H)

if (NOT HAVE_THREADS_H)
    if (WIN32)
        find_package (tinycthread REQUIRED)
    else()
        message(FATAL_ERROR "C11 threads library is required.")
    endif()
endif()

###############################################################################
# General flags

set (WARNS
        -Wall -Wextra -Wpedantic
        -Werror=format-extra-args
        -Werror=invalid-pch
)

if (NOT BUILD_DIST)
    set (MARCH_SETTING -march=native)
endif()

if (SANITIZE)
    set (SANIT -fsanitize-address-use-after-scope -fsanitize=undefined
               -fsanitize=bounds -fsanitize=bool)
    if ("${SANITIZE}" STREQUAL "thread")
        set (SANIT ${SANIT} -fsanitize=thread)
    elseif ("${SANITIZE}" STREQUAL "memory")
        set (SANIT -fsanitize=undefined -fsanitize=memory)
    else ()
        set (SANIT ${SANIT} -fsanitize=address)
    endif()
endif()

message(WARNING "Sanitize set to \"${SANIT}\" -- \"${SANITIZE}\"")

set (BASE ${WARNS} ${MARCH_SETTING} ${SANIT}
          -pipe -fdiagnostics-color=always -m64 -mprefer-vector-width=512)

################################################################################
# Compiler specific flags. Currently these override $CFLAGS.

# set (CFLAGS_DEBUG_COMMON          -Og -g3 -Wextra -Wpedantic -Wformat)
# set (CFLAGS_RELWITHDEBINFO_COMMON -O2 -g  -Wextra -ftree-vectorize -Wextra)
# set (CFLAGS_RELEASE_COMMON        -Ofast -g0 -DNDEBUG -U_FORTIFY_SOURCE)
set (CFLAGS_DEBUG_COMMON          -Og -g3 -Wextra -Wpedantic -Wformat -D_FORTIFY_SOURCE=2 -fstack-protector)
set (CFLAGS_RELWITHDEBINFO_COMMON -O3 -g3 -D_FORTIFY_SOURCE=2 -Wextra -ftree-vectorize -Wextra)
set (CFLAGS_RELEASE_COMMON        -Ofast -ftree-vectorize -g -DNDEBUG -U_FORTIFY_SOURCE)

if (NOT MSVC)
    if (SANITIZE)
        set (CFLAGS_DEBUG_COMMON ${CFLAGS_DEBUG_COMMON}
            -fno-omit-frame-pointer -fno-optimize-sibling-calls)
    endif()
else()
    message(WARNING "Can't possibly sanitize MSVC. Try nuking from orbit")
endif()

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")

    set (WARNS ${WARNS}
         -Wno-gnu -Wno-gnu-zero-variadic-macro-arguments
         -Wno-gnu-statement-expression -Werror=return-type
         -Wno-sign-compare
    )
    set (CLANG_STDLIB -stdlib=libc++)
    set (BASE ${BASE} ${WARNS}
        ${CLANG_STDLIB} -g3 -fintegrated-as -fintegrated-cc1 -fno-legacy-pass-manager
    )
    set (LTO_STR -flto=full -fwhole-program-vtables)

    string (JOIN " " CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}"
            # -fuse-ld=gold -stdlib=libstdc++ -rtlib=compiler-rt --unwindlib=libgcc
            -fuse-ld=lld -rtlib=compiler-rt ${CLANG_STDLIB} --unwindlib=libunwind
            -Wl,--lto-O3 -Wl,--lto-whole-program-visibility ${LTO_STR}
    )

    set (__EXTRA_C_CXX_RELEASE_FLAGS -fvectorize -fslp-vectorize
         -fwhole-program-vtables
        )

elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")

    set (WARNS ${WARNS}
         -Wsuggest-attribute=noreturn -Wsuggest-attribute=format
         -Wattributes
    )
    set (BASE ${BASE} ${WARNS} -fPIC -fpie -g3 -gdwarf-5)
    set (LTO_STR "-flto")

    set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}")
    set (CMAKE_SHARED_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${CMAKE_SHARED_LINKER_FLAGS}")

elseif (MSVC)

   # Allow use of deprecated function names in MSVC (read/write)
   add_definitions(-D_CRT_NONSTDC_NO_DEPRECATE)

endif()

#####################################################################################################
# Misc

if (MINGW)
    FIX_WINDOWS_PATHS(CMAKE_C_FLAGS)
    set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-allow-multiple-definition")
endif()

if (MSVC)

else()
    list(APPEND CMAKE_C_FLAGS_DEBUG ${CFLAGS_DEBUG_COMMON} ${BASE} ${CMAKE_C_FLAGS})
    list(APPEND CMAKE_C_FLAGS_MINSIZEREL ${CFLAGS_RELEASE_COMMON} ${BASE} -Os -s ${CMAKE_C_FLAGS})
    list(APPEND CMAKE_C_FLAGS_RELEASE
        ${CMAKE_C_FLAGS_RELEASE} ${CFLAGS_RELEASE_COMMON}
        ${BASE} ${LTO_STR} ${CMAKE_C_FLAGS} ${__EXTRA_C_CXX_RELEASE_FLAGS})
    list(APPEND CMAKE_C_FLAGS_RELWITHDEBINFO
        ${CMAKE_C_FLAGS_RELWITHDEBINFO} ${CFLAGS_RELWITHDEBINFO_COMMON}
        ${BASE} ${LTO_STR} ${CMAKE_C_FLAGS} ${__EXTRA_C_CXX_RELEASE_FLAGS})

    string(JOIN " " CMAKE_CXX_FLAGS "-std=gnu++${CMAKE_CXX_STANDARD}" "${CMAKE_CXX_FLAGS}")

    set(CMAKE_CXX_FLAGS_DEBUG          ${CMAKE_C_FLAGS_DEBUG}          ${CMAKE_CXX_FLAGS})
    set(CMAKE_CXX_FLAGS_RELEASE        ${CMAKE_C_FLAGS_RELEASE}        ${CMAKE_CXX_FLAGS})
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO ${CMAKE_C_FLAGS_RELWITHDEBINFO} ${CMAKE_CXX_FLAGS})
    set(CMAKE_CXX_FLAGS_MINSIZEREL     ${CMAKE_C_FLAGS_MINSIZEREL}     ${CMAKE_CXX_FLAGS})

    string(TOUPPER ${CMAKE_BUILD_TYPE} _upper_CMAKE_BUILD_TYPE)
    set (ALL_THE_C_FLAGS   ${CMAKE_C_FLAGS_${_upper_CMAKE_BUILD_TYPE}})
    set (ALL_THE_CXX_FLAGS ${CMAKE_CXX_FLAGS_${_upper_CMAKE_BUILD_TYPE}})

    list(JOIN CMAKE_C_FLAGS_DEBUG            " " CMAKE_C_FLAGS_DEBUG            )
    list(JOIN CMAKE_C_FLAGS_RELEASE          " " CMAKE_C_FLAGS_RELEASE          )
    list(JOIN CMAKE_C_FLAGS_RELWITHDEBINFO   " " CMAKE_C_FLAGS_RELWITHDEBINFO   )
    list(JOIN CMAKE_C_FLAGS_MINSIZEREL       " " CMAKE_C_FLAGS_MINSIZEREL       )
    list(JOIN CMAKE_CXX_FLAGS_DEBUG          " " CMAKE_CXX_FLAGS_DEBUG          )
    list(JOIN CMAKE_CXX_FLAGS_RELEASE        " " CMAKE_CXX_FLAGS_RELEASE        )
    list(JOIN CMAKE_CXX_FLAGS_RELWITHDEBINFO " " CMAKE_CXX_FLAGS_RELWITHDEBINFO )
    list(JOIN CMAKE_CXX_FLAGS_MINSIZEREL     " " CMAKE_CXX_FLAGS_MINSIZEREL     )
endif()

#####################################################################################################

configure_file(cmake-config.h.in config.h)
add_subdirectory(src)
