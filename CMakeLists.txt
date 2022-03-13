cmake_minimum_required (VERSION 3.20.0)

project (emlsp LANGUAGES C CXX VERSION 0.0.1)
include (CheckFunctionExists)
include (CheckSymbolExists)
include (CheckIncludeFile)
include (FindPkgConfig)

# TODO
# Fix this horrid disaster. This should _NOT_ be one big file.

###############################################################################
# Options

option(USE_JEMALLOC "Use jemalloc" OFF)
option(VCPKG_ROOT "Directory in which VCPKG resides (Windows only)" "")

###############################################################################
# Includes and defines

if (MSVC)
    set (CMAKE_REQUIRED_DEFINITIONS -DHAVE_CONFIG_H)
else()
    set (CMAKE_REQUIRED_DEFINITIONS -D__USE_ISOC99 -D__USE_ISOC11
         -D__USE_ISOCXX11 -D_GNU_SOURCE -DHAVE_CONFIG_H)
    if (MINGW)
        list(APPEND CMAKE_REQUIRED_DEFINITIONS -D__MINGW__ -D__USE_MINGW_ANSI_STDIO=1)
    endif()
endif()
add_definitions(${CMAKE_REQUIRED_DEFINITIONS})

if (NOT CMAKE_BUILD_TYPE)
    message("Setting build type to \"Release\"")
    set (CMAKE_BUILD_TYPE "Release")
endif()

if ((CMAKE_BUILD_TYPE STREQUAL "Debug") OR
    (CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo"))
    set (DEBUG 1)
endif()

set (CMAKE_C_STANDARD 17)
set (CMAKE_CXX_STANDARD 20)
set (CMAKE_CXX_STANDARD_REQUIRED TRUE)

###############################################################################
# Win32 junk. Sigh.

macro(FIX_WINDOWS_PATHS _pathvar)
    string(REPLACE "\\" "/" ${_pathvar} "${${_pathvar}}")
endmacro()

if (WIN32)
    foreach( v $ENV{PKG_CONFIG_PATH} )
        string(REPLACE "/" "\\" _temp_pkgconfig_path "${_temp_pkgconfig_path}")
        list(APPEND _temp_pkgconfig_path "${v}")
    endforeach()
    string(REPLACE ";" ":" _temp_pkgconfig_path "${_temp_pkgconfig_path}")
    set(ENV{PKG_CONFIG_PATH} "${_temp_pkgconfig_path}")
    unset(_temp_pkgconfig_path)
endif()

#if (MSVC)
#    execute_process(COMMAND pkgconf.exe --cflags glib-2.0
#                    RESULT_VARIABLE _my_pkgconfig_retval
#                    OUTPUT_VARIABLE GLib_PKG_CFLAGS
#                    ERROR_VARIABLE _my_pkgconfig_error
#                    OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_STRIP_TRAILING_WHITESPACE)
#
#    if (${_my_pkgconfig_retval} GREATER 0)
#        message(FATAL_ERROR "Failed to locate glib: ${_my_pkgconfig_error}")
#    endif()
#    execute_process(COMMAND pkgconf.exe --libs glib-2.0
#                    OUTPUT_VARIABLE GLib_PKG_LIBS
#                    OUTPUT_STRIP_TRAILING_WHITESPACE)
#
#endif()


#####################################################################################################
## Library search ##
#####################################################################################################

# add_subdirectory("contrib/uvw")

list(APPEND CMAKE_MODULE_PATH
     "${CMAKE_SOURCE_DIR}/cmake"
     # "${CMAKE_SOURCE_DIR}/cmake/libevent")
     "${CMAKE_SOURCE_DIR}/cmake/gnome-cmake/modules")

if (NOT MINGW)
    if (WIN32)
        if (NOT VCPKG_ROOT)
            message(FATAL_ERROR "Must provide a definition for VCPKG_ROOT (ie the directory with vcpkg.exe")
        endif()
        message( "INSTALLED DIR IS \"${VCPKG_ROOT}\"" )
        list(APPEND CMAKE_MODULE_PATH "${VCPKG_ROOT}/scripts/buildsystems")
        #set (VCPKG_ENABLED "YES")
		#set (VcpkgEnabled CACHE BOOL TRUE)
        include (vcpkg)
    else()
        # list (APPEND CMAKE_MODULE_PATH "/usr/lib/cmake" "/usr/share/cmake" "/usr/local/share/cmake" "/usr/local/lib/cmake")
        # list (APPEND CMAKE_PREFIX_PATH "/usr/lib/cmake" "/usr/share/cmake" "/usr/local/share/cmake" "/usr/local/lib/cmake")
    endif()
endif()

set(THREADS_PREFER_PTHREAD_FLAG TRUE)
set(CMAKE_THREAD_PREFER_PTHREAD TRUE)

find_package (Threads REQUIRED)
find_package (Boost REQUIRED)
find_package (Libevent)

if (NOT MSVC AND NOT LIBEVENT_FOUND)
    pkg_check_modules(LIBEVENT_BASE libevent REQUIRED)
    pkg_check_modules(LIBEVENT_CORE libevent_core REQUIRED)
    pkg_check_modules(LIBEVENT_EXTRA libevent_extra REQUIRED)
    pkg_check_modules(LIBEVENT_PTHREADS libevent_pthreads)
    set (LIBEVENT_LIBRARIES
        ${LIBEVENT_BASE_LDFLAGS} ${LIBEVENT_CORE_LDFLAGS}
        ${LIBEVENT_EXTRA_LDFLAGS} ${LIBEVENT_PTHREADS_LDFLAGS})
    set (LIBEVENT_INCLUDE_DIRS
        ${LIBEVENT_BASE_CFLAGS_I} ${LIBEVENT_CORE_CFLAGS_I}
        ${LIBEVENT_EXTRA_CFLAGS_I} ${LIBEVENT_PTHREADS_CFLAGS_I})
endif()

if (MSVC)
    find_package (pthreads REQUIRED)
else ()
    find_package (GLib REQUIRED)
    find_package (LibUV REQUIRED)
    #find_package (fmt 8.0.0)
    #find_package (nlohmann_json REQUIRED)
    #find_package (RapidJSON REQUIRED)
endif()

#if (NOT fmt_FOUND)
if (TRUE)
    set (fmt_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/contrib/fmt/include")
endif()

if (USE_JEMALLOC)
    find_package (Jemalloc)
endif()


###############################################################################
# Symbol and header checks

CHECK_SYMBOL_EXISTS (err            "err.h"        HAVE_ERR)
CHECK_SYMBOL_EXISTS (posix_spawnp   "spawn.h"      HAVE_POSIX_SPAWNP)
CHECK_SYMBOL_EXISTS (asprintf       "stdio.h"      HAVE_ASPRINTF)
CHECK_SYMBOL_EXISTS (dprintf        "stdio.h"      HAVE_DPRINTF)
CHECK_SYMBOL_EXISTS (mkdtemp        "stdlib.h"     HAVE_MKDTEMP)
CHECK_SYMBOL_EXISTS (mkostemps      "stdlib.h"     HAVE_MKOSTEMPS)
CHECK_SYMBOL_EXISTS (mkstemp        "stdlib.h"     HAVE_MKSTEMP)
CHECK_SYMBOL_EXISTS (strerror_r     "string.h"     HAVE_STRERROR_R)
CHECK_SYMBOL_EXISTS (strerror_s     "string.h"     HAVE_STRERROR_S)
CHECK_SYMBOL_EXISTS (stricmp        "string.h"     HAVE_STRICMP)
CHECK_SYMBOL_EXISTS (strlcat        "string.h"     HAVE_STRLCAT)
CHECK_SYMBOL_EXISTS (strlcpy        "string.h"     HAVE_STRLCPY)
CHECK_SYMBOL_EXISTS (strcpy_s       "string.h"     HAVE_STRCPY_S)
CHECK_SYMBOL_EXISTS (strsep         "string.h"     HAVE_STRSEP)
CHECK_SYMBOL_EXISTS (strcasecmp     "strings.h"    HAVE_STRCASECMP)
CHECK_SYMBOL_EXISTS (socketpair     "sys/socket.h" HAVE_SOCKETPAIR)
CHECK_SYMBOL_EXISTS (clock_gettime  "time.h"       HAVE_CLOCK_GETTIME)
CHECK_SYMBOL_EXISTS (nanosleep      "time.h"       HAVE_NANOSLEEP)
CHECK_SYMBOL_EXISTS (timespec_get   "time.h"       HAVE_TIMESPEC_GET)
CHECK_SYMBOL_EXISTS (fork           "unistd.h"     HAVE_FORK)
CHECK_SYMBOL_EXISTS (pause          "unistd.h"     HAVE_PAUSE)
CHECK_SYMBOL_EXISTS (pipe2          "unistd.h"     HAVE_PIPE2)

CHECK_INCLUDE_FILE ("execinfo.h"    HAVE_EXECINFO_H)
CHECK_INCLUDE_FILE ("stdatomic.h"   HAVE_STDATOMIC_H)
CHECK_INCLUDE_FILE ("stdnoreturn.h" HAVE_STDNORETURN_H)
CHECK_INCLUDE_FILE ("threads.h"     HAVE_THREADS_H)
CHECK_INCLUDE_FILE ("unistd.h"      HAVE_UNISTD_H)

if (NOT HAVE_SOCKETPAIR)
    CHECK_SYMBOL_EXISTS (socketpair "socket.h" HAVE_SOCKETPAIR)
endif()

#if (NOT HAVE_THREADS_H)
#    if (WIN32)
#        find_package (tinycthread REQUIRED)
#    else()
#        message(WARNING
#            "C11 threads library is required. I'm going to assume you have "
#            "tinycthreads installed because I'm too lazy to write anything "
#            "to search for it. If you do not, this will not go well."
#        )
#    endif()
#endif()

configure_file(cmake-config.h.in config.h)


#####################################################################################################
## General flags ##
#####################################################################################################


set (WARNS
        -Wall -Wextra
        -Werror=format-extra-args
        -Wimplicit-fallthrough
        -Werror=invalid-pch
)
set (C_ONLY_WARNS
        -Werror=implicit
        -Werror=implicit-function-declaration
        -Werror=incompatible-pointer-types
        -Werror=int-conversion
        -Werror=pointer-to-int-cast
)

if (NOT BUILD_DIST)
    set (MARCH_SETTING -march=native)
endif()

if (SANITIZE)
    set (SANIT -fsanitize=undefined -fsanitize=bounds -fsanitize=bool)
    if ("${SANITIZE}" STREQUAL "thread")
        set (SANIT ${SANIT} -fsanitize=thread)
    elseif ("${SANITIZE}" STREQUAL "memory")
        set (SANIT -fsanitize=undefined -fsanitize=memory)
    else ()
        set (SANIT ${SANIT} -fsanitize=address -fsanitize-address-use-after-scope)
    endif()
endif()

set (BASE ${WARNS} ${MARCH_SETTING} ${SANIT}
    -pipe -fdiagnostics-color=always
    -mprefer-vector-width=512
)
          
set (CFLAGS_DEBUG_COMMON
     -O0 -g -UNDEBUG -D_FORTIFY_SOURCE=2
     -Wextra -Wpedantic -Wformat -U_FORTIFY_SOURCE
)
set (CFLAGS_RELWITHDEBINFO_COMMON
     -O2 -g -D_FORTIFY_SOURCE=2
     -Wextra -ftree-vectorize -Wextra -U_FORTIFY_SOURCE
)
set (CFLAGS_RELEASE_COMMON
     -Ofast -ftree-vectorize -g
     -DNDEBUG -U_FORTIFY_SOURCE
)


################################################################################
# Compiler specific flags. Currently these override $CFLAGS.


if (NOT MSVC)
    if (SANITIZE)
        set (CFLAGS_DEBUG_COMMON ${CFLAGS_DEBUG_COMMON}
            -fno-omit-frame-pointer -fno-optimize-sibling-calls)
    endif()
else()
    message(WARNING "Can't possibly sanitize MSVC. Try nuking from orbit")
endif()


##########################################################################################
# Per compiler config. Probably mostly temporary?

#-----------------------------------------------------------------------------------------
#-- CLANG --
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")

    set (WARNS ${WARNS}
        #-Weverything
        -Wno-c++98-compat -Wno-c++98-compat-pedantic
        -Wno-zero-as-null-pointer-constant
        -Wno-disabled-macro-expansion -Wno-reserved-macro-identifier -Wno-unused-macros
        -Wno-weak-vtables
        -Wno-shorten-64-to-32
        -Wno-ctad-maybe-unsupported
        -Wno-missing-prototypes
    )
    set (WARNS ${WARNS}
         -Wno-gnu -Wno-gnu-zero-variadic-macro-arguments
         -Wno-gnu-statement-expression -Werror=return-type
         -Werror=inline-namespace-reopened-noninline
    )

    if (${CMAKE_BUILD_TYPE} STREQUAL "Release")
        set (LTO_STR -flto=full
                     -fwhole-program-vtables
                     -fvirtual-function-elimination
            #         -fsplit-lto-unit
            )

        if (MINGW OR WIN32)
            set (LTO_LINK_STR -Wl,--lto-O3 ${LTO_STR})
        else()
            set (LTO_LINK_STR ${LTO_STR} -Wl,--lto-O3 -Wl,--lto-whole-program-visibility)
        endif()

        #set (LTO_STR -flto=full)
        # set (LTO_LINK_STR ${LTO_STR} -g3
        #     -Wl,-Xlink=-mllvm:-march=native
        #     -Wl,-Xlink=-mllvm:--aggressive-ext-opt
        #     -Wl,-Xlink=-mllvm:--extra-vectorizer-passes
        #     -Wl,-Xlink=-mllvm:--exhaustive-register-search
        #     -Wl,-Xlink=-mllvm:--enable-unsafe-fp-math
        #     -Wl,-Xlink=-mllvm:--optimize-regalloc
        #     -Wl,-Xlink=-mllvm:--scalar-evolution-use-expensive-range-sharpening
        #     -Wl,-Xlink=-mllvm:--slp-max-vf=0
        #     -Wl,-Xlink=-mllvm:--slp-vectorize-hor
        #     -Wl,-Xlink=-mllvm:--whole-program-visibility
        #     -Wl,-Xlink=-mllvm:--x86-indirect-branch-tracking
        #     -Wl,-Xlink=-mllvm:--interleave-loops
        #     -Wl,-Xlink=-mllvm:--ir-outliner
        #     -Wl,-Xlink=-mllvm:--enable-post-misched
        #     -Wl,-Xlink=-mllvm:--enable-nontrivial-unswitch
        #     -Wl,-Xlink=-mllvm:--enable-nonnull-arg-prop
        # )
    else()
        set (LTO_STR -flto=full
                     -fwhole-program-vtables
                     #-fvirtual-function-elimination
                     # -fsplit-lto-unit
                     -g1 -gdwarf-5)
        set (LTO_LINK_STR  ${LTO_STR})
    endif()

    if (WIN32 OR MINGW OR MSYS)
        if (FALSE)
            set (BASE ${BASE} -fansi-escape-codes -target x86_64-w64-windows -fc++-abi=microsoft)
            string (JOIN " " CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}"
                    ${CLANG_STDLIB} ${LTO_STR}
                    -target x86_64-w64-windows -fc++-abi=microsoft
            )
        else()
            set (CLANG_STDLIB -stdlib=libc++)
            set (BASE ${BASE} -fansi-escape-codes)
            string (JOIN " " CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}"
                    ${CLANG_STDLIB}
                    ${LTO_LINK_STR}
                    -fuse-ld=lld
                    -rtlib=compiler-rt
                    --unwindlib=libunwind
            )
        endif()

    else() # NOT WIN32

        # set (CLANG_STDLIB -stdlib=libc++)
        set (CLANG_STDLIB -stdlib=libstdc++)
        string (JOIN " " CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}"
                -fuse-ld=lld
                # -rtlib=libgcc
                -rtlib=compiler-rt
                -unwindlib=libgcc
                # -unwindlib=libunwind
                -rdynamic
                -Wl,--as-needed
                ${CLANG_STDLIB}
                ${LTO_LINK_STR}
        )
    endif()

    set (__EXTRA_C_CXX_RELEASE_FLAGS -fslp-vectorize)

    set (BASE ${BASE} ${WARNS} #-fPIC -fpie -masm=intel
              -fintegrated-as -fintegrated-cc1 -fno-legacy-pass-manager
    )

    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -frtti ${CLANG_STDLIB} -std=gnu++20")

    find_program(LLVM_AR "llvm-ar" REQUIRED)
    find_program(LLVM_NM "llvm-nm" REQUIRED)
    find_program(LLVM_RANLIB "llvm-ranlib" REQUIRED)
    set (CMAKE_AR "${LLVM_AR}")
    set (CMAKE_NM "${LLVM_NM}")
    set (CMAKE_RANLIB "${LLVM_RANLIB}")

#-----------------------------------------------------------------------------------------
#-- GCC --
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")

    set (WARNS ${WARNS}
         -Wsuggest-attribute=noreturn -Wsuggest-attribute=format
         -Wsuggest-attribute=const -Wsuggest-attribute=pure
         -Wsuggest-attribute=cold -Wsuggest-attribute=malloc
         -Wattributes -fdiagnostics-show-option
    )
    set (BASE ${BASE} ${WARNS} -g3 -gdwarf-5)
    set (LTO_STR "-flto -fuse-linker-plugin")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -frtti -std=gnu++20")

    string (JOIN " " CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}"
        -fuse-ld=bfd -Wl,-O3
        ${LTO_STR}
    )
    if (MINGW)
    else ()
        string (JOIN " " CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}"
            -rdynamic
        )
    endif()
    

#-----------------------------------------------------------------------------------------
#-- MSVC --
elseif (MSVC)

   # Allow use of "deprecated" function names in MSVC (read/write)
   add_definitions(-D_CRT_NONSTDC_NO_DEPRECATE)
   set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /Zc:preprocessor /std:c17")
   set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zc:preprocessor /Zc:__cplusplus /std:c++20")

endif()

set (CMAKE_SHARED_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${CMAKE_SHARED_LINKER_FLAGS}")


#####################################################################################################
# Misc

if (MINGW)
    FIX_WINDOWS_PATHS(CMAKE_C_FLAGS)
    set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-allow-multiple-definition")
endif()

if (MSVC)
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /utf-8")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /utf-8")
else()
    list(APPEND CMAKE_C_FLAGS_DEBUG ${CFLAGS_DEBUG_COMMON} ${BASE} ${CMAKE_C_FLAGS} ${LTO_STR})
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
## TARGETS ##
#####################################################################################################


set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

add_executable(lsp-thing
    "src/main.cc"
    "src/rapid.cc"
    #"src/sukitup.cc"
    "src/dothemsg.cc"
    "src/sigh.cc"

    "src/libevent.cc"
    #"src/killme.cc"
    "src/util/charconv.cc"
    "src/util/charconv.hh"

    "src/ipc/base_connection.cc"
    "src/ipc/connection_impl.cc"
    "src/ipc/base_client.cc"

    "src/ipc/lsp/static-data.cc"
    "src/ipc/lsp/lsp-client.cc"

    "src/util/c_util.c"
    "src/util/err.c"
    "src/util/tmpfile.c"
    "src/util/demangle.cc"
    "src/util/util.cc"

    "src/util/formatters.hh"

    # Include headers to make using MSVC less infuriatingly tedious.
    "src/Common.hh"
    #"src/ipc/asio_connection.hh"
    "src/ipc/base_client.hh"
    "src/ipc/base_connection.hh"
    "src/ipc/base_message.hh"
    "src/ipc/base_wait_loop.hh"
    "src/ipc/connection_impl.hh"
    "src/ipc/dialer.hh"
    "src/ipc/forward.hh"
    "src/ipc/lsp/lsp-client.hh"
    "src/ipc/lsp/lsp-connection.hh"
    "src/ipc/lsp/lsp-message.hh"
    "src/ipc/lsp/static-data.hh"
    "src/ipc/main_loop.hh"
    "src/ipc/totality.hh"
    "src/ipc/rpc/basic_wrapper.hh"
    #"src/ipc/mpack/mpack-connection.hh"
    "src/pch.hh"
    "src/rapid.hh"
    "src/util/concepts.hh"
    "src/util/exceptions.hh"
    "src/util/util.hh"
    "src/util/c_util.h"
    "src/util/initializer_hack.h"
    "src/util/macros.h"
    "src/util/myerr.h"
    "src/util/hackish_templates.hh"

    "${CMAKE_CURRENT_BINARY_DIR}/config.h"
)

#if (TRUE AND (NOT (WIN32 AND "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")))
if (TRUE OR ((NOT MINGW) AND (NOT "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")))
   target_precompile_headers(lsp-thing
       PRIVATE
       $<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/src/pch.hh>
   )
endif()

#####################################################################################################
# Extra flags, includes

list(JOIN GLib_PKG_CFLAGS " " _glib_cflags )
list(JOIN GLib_PKG_LIBS " " _glib_ldflags)
set_property(TARGET lsp-thing APPEND PROPERTY COMPILE_FLAGS "${_glib_cflags}")

set_property(TARGET lsp-thing APPEND PROPERTY INCLUDE_DIRECTORIES
             "${CMAKE_CURRENT_BINARY_DIR}"
             "${CMAKE_SOURCE_DIR}/src"
             "${CMAKE_SOURCE_DIR}/contrib/rapidjson/include"
             #"${CMAKE_SOURCE_DIR}/contrib/uvw/src"
             #"${CMAKE_SOURCE_DIR}/contrib/uvw/jpcre2/src"
             ${fmt_INCLUDE_DIRECTORIES}
             ${LIBEVENT_INCLUDE_DIRS}
             ${LIBUV_INCLUDE_DIRS}
)


#####################################################################################################
## Linking ##
#####################################################################################################


if (MSVC)
    set (_unistring_location "${CMAKE_SOURCE_DIR}/contrib/libunistring")
    set_property(TARGET lsp-thing APPEND PROPERTY INCLUDE_DIRECTORIES
        "${_unistring_location}/include")
    target_link_libraries(lsp-thing
        "${_unistring_location}/lib/libunistring.dll.a")
else()
    if (MINGW)
    #target_link_libraries(lsp-thing "dbghelp.lib")
    else()
        set_property(TARGET lsp-thing APPEND PROPERTY LINK_FLAGS "-Wl,--as-needed")
    endif()
    target_link_libraries(lsp-thing -lunistring)
    set_property(SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/pch.hh APPEND PROPERTY COMPILE_OPTIONS
            -fpch-debuginfo -fpch-instantiate-templates -fpch-codegen -relocatable-pch
    )
endif()

if (MSVC AND NOT "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    # FIXME: This is clearly trash and needs to be re-written.

    if (NOT _VCPKG_INSTALLED_DIR)
        message(FATAL_ERROR "No vcpkg found")
    endif()

    if (${CMAKE_BUILD_TYPE} STREQUAL "Debug" OR ${CMAKE_BUILD_TYPE} STREQUAL "RelWithDebInfo")
        set (_WIN32_VCPKG_LIB_PATH "${_VCPKG_INSTALLED_DIR}/x64-windows/debug/lib")
        #target_link_libraries(lsp-thing ${TINYCTHREAD_LIBRARY_DEBUG})
    else()
        set (_WIN32_VCPKG_LIB_PATH "${_VCPKG_INSTALLED_DIR}/x64-windows/lib")
        #target_link_libraries(lsp-thing ${TINYCTHREAD_LIBRARY_RELEASE})
    endif()

endif()
#elseif (NOT HAVE_THREADS_H)
#if(NOT HAVE_THREADS_H)
#    add_library(tinycthread OBJECT "${CMAKE_SOURCE_DIR}/contrib/tinycthread/source/tinycthread.c")
#    target_link_libraries(lsp-thing tinycthread)
#    set_property(TARGET lsp-thing APPEND PROPERTY INCLUDE_DIRECTORIES
#                 "${CMAKE_SOURCE_DIR}/contrib/tinycthread/source")
#endif()

# set_property(TARGET lsp-thing APPEND PROPERTY INCLUDE_DIRECTORIES
#              "${CMAKE_SOURCE_DIR}/contrib/uvw/include")

#----------------------------------------------------------------------------------------------------

target_link_libraries(lsp-thing
    ${CMAKE_THREAD_LIBS_INIT}
    ${LIBEVENT_LIBRARIES}
    Threads::Threads
    #-luv -ltbb
    # uvw::uvw
    #-levent_core
    #-lunistring
    #-ljsoncpp -ljsonrpccpp-client -ljsonrpccpp-common -ljsonrpccpp-stub -ljsonrpccpp-server
)

#----------------------------------------------------------------------------------------------------

if (NOT Libevent_FOUND)
    if (MSVC)
        message(FATAL_ERROR "BAD BAD")
    #    # Obvious temporary rage-induced workaround
    #    set (_libevent_location "D:\\ass\\GIT\\libevent\\mingw\\")
    #    set_property(TARGET lsp-thing APPEND PROPERTY INCLUDE_DIRECTORIES
    #        "${_libevent_location}/include"
    #    )
    #    set_property(TARGET lsp-thing APPEND PROPERTY CMAKE_LIBRARY_PATH
    #        "${_libevent_location}/lib" "${_libevent_location}/bin"
    #    )

    #    target_link_libraries(lsp-thing
    #        "${_libevent_location}\\lib\\libevent.dll.a"
    #        "${_libevent_location}\\lib\\libevent_core.dll.a"
    #        "${_libevent_location}\\lib\\libevent_extra.dll.a"
    #        "${_libevent_location}\\lib\\libevent_openssl.dll.a"
    #    )
    else ()
        message (WARNING "Libevent not found. Assuming it's there anyway.")
        target_link_libraries(lsp-thing -levent_openssl -levent -levent_core -levent_extra)
    endif()
endif()


if (fmt_FOUND)
    target_link_libraries(lsp-thing fmt::fmt-header-only)
endif()


if (MSVC)
    set_property(TARGET lsp-thing APPEND PROPERTY INCLUDE_DIRECTORIES
        "${VCPKG_ROOT}/installed/x64-windows/include/glib-2.0"
		"${VCPKG_ROOT}/installed/x64-windows/include/gio-win32-2.0"
		"${VCPKG_ROOT}/installed/x64-windows/lib/glib-2.0/include"
        "D:/Program Files (x86)/Visual Leak Detector/include"
	)
    target_link_libraries(lsp-thing
        "${_WIN32_VCPKG_LIB_PATH}/libuv.lib"
        "${_WIN32_VCPKG_LIB_PATH}/glib-2.0.lib"
        "${_WIN32_VCPKG_LIB_PATH}/intl.lib"
        "${_WIN32_VCPKG_LIB_PATH}/iconv.lib"
        "${_WIN32_VCPKG_LIB_PATH}/msgpackc.lib"
		PThreads4W::PThreads4W
        "D:/Program Files (x86)/Visual Leak Detector/lib/Win64/vld.lib"
    )

else()
    target_link_libraries(lsp-thing
        ${GLib_LIBRARY}
        ${GLib_PKG_LIBS}
        ${GLib_LIBRARIES}
        ${LIBUV_LIBRARIES}
    )
endif()


if (USE_JEMALLOC STREQUAL "YES")
    if (NOT JEMALLOC_FOUND)
        # The cmake module to find jemalloc is pretty lousy. It doesn't hurt
        # too much to just try linking it anyway.
        target_link_libraries(lsp-thing -ljemalloc)
    else()
        target_link_libraries(lsp-thing ${JEMALLOC_LIBRARY})
    endif()
endif()


if (WIN32)
    target_link_libraries(lsp-thing ws2_32.lib)
endif()
