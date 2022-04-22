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

set (USE_JEMALLOC OFF CACHE BOOL "Use jemalloc")
set (VCPKG_ROOT "" CACHE STRING "Directory in which VCPKG resides (Windows only)")

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


#####################################################################################################
## Library search ##
#####################################################################################################

# add_subdirectory("contrib/uvw")

list(APPEND CMAKE_MODULE_PATH
     "${CMAKE_SOURCE_DIR}/cmake"
     # "${CMAKE_SOURCE_DIR}/cmake/libevent")
     "${CMAKE_SOURCE_DIR}/cmake/gnome-cmake/modules")

if (NOT MINGW AND NOT MSYS)
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
        list (APPEND CMAKE_MODULE_PATH "/usr/lib/cmake" "/usr/share/cmake" "/usr/local/share/cmake" "/usr/local/lib/cmake")
        list (APPEND CMAKE_PREFIX_PATH "/usr/lib/cmake" "/usr/share/cmake" "/usr/local/share/cmake" "/usr/local/lib/cmake")
    endif()
endif()

set(THREADS_PREFER_PTHREAD_FLAG TRUE)
set(CMAKE_THREAD_PREFER_PTHREAD TRUE)

find_package (Threads REQUIRED)
find_package (Boost REQUIRED)
find_package (Libevent CONFIG QUIET)
if (NOT LIBEVENT_FOUND)
    find_package(Libevent QUIET)
endif()

list (APPEND CMAKE_PREFIX_PATH
    "/usr/lib/llvm/14/lib64/cmake/llvm"
    "/usr/lib/llvm/14/lib64/cmake/clang"
)

find_package (Clang CONFIG REQUIRED)
find_package (LLVM CONFIG REQUIRED)

if (NOT LIBEVENT_FOUND)
    if (MSVC)
        message(FATAL_ERROR "libevent2 not found")
    else()
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
endif()

if (MSVC)
    find_package (pthreads REQUIRED)
else ()
    find_package (GLib REQUIRED)
    find_package (LibUV REQUIRED)
    find_package (Unistring REQUIRED)
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

set (CHK_SYM_INC
     "stddef.h" "stdint.h" "stdio.h" "stdlib.h" "string.h" "time.h")

if (WIN32)
    set (CHK_SYM_INC ${CHK_SYM_INC}
         "WinSock2.h" "Windows.h" "process.h" "io.h" "strsafe.h")
elseif (UNIX)
    set (CHK_SYM_INC ${CHK_SYM_INC}
         "sys/socket.h" "unistd.h" "spawn.h" "strings.h")
endif()

check_symbol_exists (err            "${CHK_SYM_INC}"  HAVE_ERR)
check_symbol_exists (posix_spawnp   "${CHK_SYM_INC}"  HAVE_POSIX_SPAWNP)
check_symbol_exists (asprintf       "${CHK_SYM_INC}"  HAVE_ASPRINTF)
check_symbol_exists (dprintf        "${CHK_SYM_INC}"  HAVE_DPRINTF)
check_symbol_exists (mkdtemp        "${CHK_SYM_INC}"  HAVE_MKDTEMP)
check_symbol_exists (mkostemps      "${CHK_SYM_INC}"  HAVE_MKOSTEMPS)
check_symbol_exists (mkstemp        "${CHK_SYM_INC}"  HAVE_MKSTEMP)
check_symbol_exists (strerror_r     "${CHK_SYM_INC}"  HAVE_STRERROR_R)
check_symbol_exists (strerror_s     "${CHK_SYM_INC}"  HAVE_STRERROR_S)
check_symbol_exists (stricmp        "${CHK_SYM_INC}"  HAVE_STRICMP)
check_symbol_exists (strlcat        "${CHK_SYM_INC}"  HAVE_STRLCAT)
check_symbol_exists (strlcpy        "${CHK_SYM_INC}"  HAVE_STRLCPY)
check_symbol_exists (strcpy_s       "${CHK_SYM_INC}"  HAVE_STRCPY_S)
check_symbol_exists (strsep         "${CHK_SYM_INC}"  HAVE_STRSEP)
check_symbol_exists (strcasecmp     "${CHK_SYM_INC}"  HAVE_STRCASECMP)
check_symbol_exists (socketpair     "${CHK_SYM_INC}"  HAVE_SOCKETPAIR)
check_symbol_exists (clock_gettime  "${CHK_SYM_INC}"  HAVE_CLOCK_GETTIME)
check_symbol_exists (nanosleep      "${CHK_SYM_INC}"  HAVE_NANOSLEEP)
check_symbol_exists (timespec_get   "${CHK_SYM_INC}"  HAVE_TIMESPEC_GET)
check_symbol_exists (fork           "${CHK_SYM_INC}"  HAVE_FORK)
check_symbol_exists (pause          "${CHK_SYM_INC}"  HAVE_PAUSE)
check_symbol_exists (pipe2          "${CHK_SYM_INC}"  HAVE_PIPE2)

check_include_file ("afunix.h"      HAVE_AFUNIX_H)
check_include_file ("execinfo.h"    HAVE_EXECINFO_H)
check_include_file ("threads.h"     HAVE_THREADS_H)

if (NOT HAVE_SOCKETPAIR)
    CHECK_SYMBOL_EXISTS (socketpair "socket.h" HAVE_SOCKETPAIR)
endif()

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
        -fsanitize=address,undefined
    )
    set (WARNS ${WARNS}
         -Wno-gnu -Wno-gnu-zero-variadic-macro-arguments
         -Wno-gnu-statement-expression -Werror=return-type
         -Werror=inline-namespace-reopened-noninline
    )

    set (_debug_flags 
        -g -gdwarf-2
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
                     -fvirtual-function-elimination
                     -fsplit-lto-unit
                     )

        if (MINGW OR WIN32)
            set (LTO_LINK_STR ${LTO_STR})
        else()
            set (LTO_LINK_STR ${LTO_STR} -Wl,--lto-O1 -Wl,--lto-whole-program-visibility)
        endif()
    endif()

    #------------------------------------------------------------------------------------
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

    #------------------------------------------------------------------------------------
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
    #------------------------------------------------------------------------------------

    set (__EXTRA_C_CXX_RELEASE_FLAGS -fslp-vectorize)

    set (BASE ${BASE} ${WARNS}
              -fintegrated-as -fintegrated-cc1 -fno-legacy-pass-manager
              ${_debug_flags}
    )
    string (JOIN " " CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}"
            ${_debug_flags})

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
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /MP /utf-8")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP /utf-8")
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


add_subdirectory(src)

#if (TRUE AND (NOT (WIN32 AND "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")))
#if (TRUE OR ((NOT MINGW) AND (NOT "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")))
#   target_precompile_headers(lsp-thing
#       PRIVATE
#       $<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/src/pch.hh>
#   )
#endif()


#----------------------------------------------------------------------------------------------------
# Extra flags, includes

list(JOIN GLib_PKG_CFLAGS " " _glib_cflags )
list(JOIN GLib_PKG_LIBS " " _glib_ldflags)
set_property(TARGET lsp-thing APPEND PROPERTY COMPILE_FLAGS "${_glib_cflags}")

target_include_directories(lsp-thing AFTER PRIVATE
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
    target_include_directories(lsp-thing AFTER PRIVATE
        "${_unistring_location}/include")
    target_link_libraries(lsp-thing
        "${_unistring_location}/lib/libunistring.dll.a")

    if (NOT "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        # FIXME: This is clearly trash and needs to be re-written.
        if (NOT _VCPKG_INSTALLED_DIR)
            message(FATAL_ERROR "No vcpkg found")
        endif()
        if (${CMAKE_BUILD_TYPE} STREQUAL "Debug" OR ${CMAKE_BUILD_TYPE} STREQUAL "RelWithDebInfo")
            set (_WIN32_VCPKG_LIB_PATH "${_VCPKG_INSTALLED_DIR}/x64-windows/debug/lib")
        else()
            set (_WIN32_VCPKG_LIB_PATH "${_VCPKG_INSTALLED_DIR}/x64-windows/lib")
        endif()
    endif ()

    target_include_directories(lsp-thing AFTER PRIVATE
        "${VCPKG_ROOT}/installed/x64-windows/include/glib-2.0"
        "${VCPKG_ROOT}/installed/x64-windows/include/gio-win32-2.0"
        "${VCPKG_ROOT}/installed/x64-windows/lib/glib-2.0/include"
        "D:/Program Files (x86)/Visual Leak Detector/include" # FIXME duh...
    )
    target_link_libraries(lsp-thing
        "${_WIN32_VCPKG_LIB_PATH}/libuv.lib"
        "${_WIN32_VCPKG_LIB_PATH}/glib-2.0.lib"
        "${_WIN32_VCPKG_LIB_PATH}/intl.lib"
        "${_WIN32_VCPKG_LIB_PATH}/iconv.lib"
        "${_WIN32_VCPKG_LIB_PATH}/msgpackc.lib"
        PThreads4W::PThreads4W
        libevent::core
        libevent::extra
        libevent::openssl
        "D:/Program Files (x86)/Visual Leak Detector/lib/Win64/vld.lib"
    )
else() # NOT MSVC
    if (NOT MINGW)
        set_property(TARGET lsp-thing APPEND PROPERTY LINK_FLAGS "-Wl,--as-needed")
    endif()
    set_property(SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/pch.hh APPEND PROPERTY COMPILE_OPTIONS
                 -fpch-debuginfo -fpch-instantiate-templates -fpch-codegen -relocatable-pch
    )
    target_include_directories(lsp-thing AFTER PRIVATE ${UNISTRING_INCLUDE_DIR})
    target_link_libraries(lsp-thing
        ${GLib_LIBRARY}
        ${GLib_PKG_LIBS}
        ${GLib_LIBRARIES}
        ${LIBUV_LIBRARIES}
        ${UNISTRING_LIBRARY}
        ${LIBEVENT_LIBRARIES}
    )
endif()

# set_property(TARGET lsp-thing APPEND PROPERTY INCLUDE_DIRECTORIES
#              "${CMAKE_SOURCE_DIR}/contrib/uvw/include")

#----------------------------------------------------------------------------------------------------

add_definitions(${${LLVM_AVAILABLE_LIBS}})
target_include_directories(lsp-thing AFTER PRIVATE
    ${LLVM_INCLUDE_DIRS}
)
target_link_libraries(lsp-thing
    -L${LLVM_LIBRARY_DIRS}
    -lLLVM
    -lclang
    -lclang-cpp
)

target_link_libraries(lsp-thing
    ${CMAKE_THREAD_LIBS_INIT}
    Threads::Threads
    Boost::boost
    -ltbb
    -lomp
)

#----------------------------------------------------------------------------------------------------

if (NOT Libevent_FOUND)
    if (MSVC)
        message(FATAL_ERROR "Libevent not found. This means something has gone _terribly_ wrong.")
    else ()
        message (WARNING "Libevent not found. Assuming it's there anyway.")
        target_link_libraries(lsp-thing -levent_openssl -levent -levent_core -levent_extra)
    endif()
endif()

if (fmt_FOUND)
    target_link_libraries(lsp-thing fmt::fmt-header-only)
endif()

if (USE_JEMALLOC)
    if (NOT JEMALLOC_FOUND)
        # The cmake module to find jemalloc is pretty lousy. It doesn't hurt
        # too much to just try linking it anyway.
        target_link_libraries(lsp-thing -ljemalloc)
    else()
        target_link_libraries(lsp-thing ${JEMALLOC_LIBRARY})
    endif()
endif()

if (WIN32)
    target_link_libraries(lsp-thing WS2_32.lib)
endif()
