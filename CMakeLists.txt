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

list(APPEND CMAKE_REQUIRED_DEFINITIONS -DHAVE_CONFIG_H)
if (NOT MSVC)
    list(APPEND CMAKE_REQUIRED_DEFINITIONS -D_GNU_SOURCE)
    if (MINGW)
        list(APPEND CMAKE_REQUIRED_DEFINITIONS 
            -D__MINGW__
            #-D__USE_MINGW_ANSI_STDIO=0
        )
    endif()
endif()
if (WIN32)
    list(APPEND CMAKE_REQUIRED_DEFINITIONS 
        -D_WIN32_WINNT=0x0A00
        -D_STL_WIN32_WINNT=0x0A00
        -D_CRT_NONSTDC_NO_DEPRECATE
        -DWIN32_LEAN_AND_MEAN
        -DNOMINMAX
        -D__NO_BYPASS_SAL=0
    )
endif()

add_definitions(${CMAKE_REQUIRED_DEFINITIONS})

if (NOT DEFINED CMAKE_BUILD_TYPE)
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
set (CMAKE_C_EXTENSIONS ON)
set (CMAKE_CXX_EXTENSIONS ON)

###############################################################################
# Win32 junk. Sigh.

macro(FIX_WINDOWS_PATHS _pathvar)
    string(REPLACE "/" "/" ${_pathvar} "${${_pathvar}}")
endmacro()

if (WIN32)
    foreach( v $ENV{PKG_CONFIG_PATH} )
        string(REPLACE "/" "/" _temp_pkgconfig_path "${_temp_pkgconfig_path}")
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
     "${CMAKE_SOURCE_DIR}/cmake/gnome-cmake/modules")

if (NOT MINGW AND NOT MSYS)
    if (WIN32)
        if (NOT VCPKG_ROOT)
            message(FATAL_ERROR "Must provide a definition for VCPKG_ROOT (ie the directory with vcpkg.exe")
        endif()
        message( "INSTALLED DIR IS \"${VCPKG_ROOT}\"" )
        list(APPEND CMAKE_MODULE_PATH "${VCPKG_ROOT}/scripts/buildsystems")
        set (VCPKG_ENABLED CACHE BOOL FALSE)
        set (VcpkgEnabled CACHE BOOL FALSE)
        include (vcpkg)
    else()
        list (APPEND CMAKE_MODULE_PATH "/usr/lib/cmake" "/usr/share/cmake" "/usr/local/share/cmake" "/usr/local/lib/cmake")
        list (APPEND CMAKE_PREFIX_PATH "/usr/lib/cmake" "/usr/share/cmake" "/usr/local/share/cmake" "/usr/local/lib/cmake")
    endif()
endif()

find_package (Boost REQUIRED)
find_package (msgpack REQUIRED COMPONENTS msgpack-cxx)
find_package (GLib REQUIRED)

if (MSVC)
    find_package (LibUV REQUIRED)
    find_package (Unistring QUIET)
else ()
    find_package (Threads)
    find_package (LibUV REQUIRED CONFIG)
    find_package (Unistring REQUIRED)

    #find_package (fmt 8.0.0)
    #find_package (nlohmann_json REQUIRED)
    #find_package (RapidJSON REQUIRED)
endif()

#if (NOT fmt_FOUND)
if (TRUE)
    set (fmt_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/contrib/fmt/include")
    message(WARNING "have ----> ${fmt_INCLUDE_DIRECTORIES}")
endif()

if (USE_JEMALLOC)
    find_package (Jemalloc)
endif()


#####################################################################################################
## Symbol and header checks ##
#####################################################################################################

set (CHK_SYM_INC
     "stddef.h" "stdint.h" "stdio.h" "stdlib.h" "string.h" "time.h")

if (WIN32)
    set (CHK_SYM_INC ${CHK_SYM_INC}
         "WinSock2.h" "Windows.h" "process.h" "io.h" "strsafe.h" "direct.h" "time.h")
    if (MINGW)
        set (CHK_SYM_INC ${CHK_SYM_INC} "unistd.h")
    endif()
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

check_include_file (afunix.h      HAVE_AFUNIX_H -DADDRESS_FAMILY=int)
check_include_file (execinfo.h    HAVE_EXECINFO_H)
check_include_file (threads.h     HAVE_THREADS_H)

if (NOT HAVE_SOCKETPAIR)
    check_symbol_exists (socketpair "socket.h" HAVE_SOCKETPAIR)
endif()

if (WIN32 AND NOT HAVE_AFUNIX_H)
    message(FATAL_ERROR "afunix.h is required")
endif()

configure_file(cmake-config.h.in config.h)

# Configure compilers
include ("SetCompilerFlags")


#####################################################################################################
## TARGETS ##
#####################################################################################################


add_subdirectory(src)

if (MSVC OR (FALSE AND NOT (MINGW AND "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")))
  target_precompile_headers(lsp-thing
      PRIVATE
      $<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/src/pch.hh>
  )
else()
    # set_property(TARGET lsp-thing APPEND_STRING PROPERTY COMPILE_FLAGS " -include pch.hh")
    set_property(TARGET lsp-thing APPEND_STRING PROPERTY COMPILE_FLAGS " -include Common.hh")
endif()


#----------------------------------------------------------------------------------------------------
# Extra flags, includes

include_directories(msgpackc-cxx)

list(JOIN GLib_PKG_CFLAGS " " _glib_cflags )
list(JOIN GLib_PKG_LIBS " " _glib_ldflags)
set_property(TARGET lsp-thing APPEND_STRING PROPERTY COMPILE_FLAGS " ${_glib_cflags}")

target_include_directories(
    lsp-thing
    AFTER PRIVATE
    "${CMAKE_CURRENT_BINARY_DIR}"
    "${CMAKE_SOURCE_DIR}/src"
    "${CMAKE_SOURCE_DIR}/contrib/rapidjson/include"
    ${fmt_INCLUDE_DIRECTORIES}
    
    #${MPI_C_COMPILER_INCLUDE_DIRS}
    #${LIBEVENT_INCLUDE_DIRS}
    #${LIBUV_INCLUDE_DIRS}
    #${msgpack_INCLUDE_DIRCTORIES}
    #${msgpack_INCLUDE_DIRS}
    #${msgpack_INCLUDE_DIR}
)

if (MINGW)
        target_include_directories(lsp-thing AFTER PRIVATE "/opt/prefix/include")
endif()


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
        #"${_WIN32_VCPKG_LIB_PATH}/msgpackc.lib"
        # PThreads4W::PThreads4W
        "D:/Program Files (x86)/Visual Leak Detector/lib/Win64/vld.lib"
    )
#----------------------------------------------------------------------------------------------------
else() # NOT MSVC
#----------------------------------------------------------------------------------------------------
    if (WIN32) # FIXME THIS SUCKS
        target_include_directories(lsp-thing AFTER PRIVATE "D:/new_msys64/opt/prefix/include")
        target_link_libraries(lsp-thing "-LD:/new_msys64/opt/prefix/lib" -ldetours)
    endif()
    
    if (MINGW)
        if (${CMAKE_BUILD_TYPE} STREQUAL "Debug" OR ${CMAKE_BUILD_TYPE} STREQUAL "RelWithDebInfo")
            target_link_libraries(lsp-thing -lssp kernel32.lib)
        endif()
    else()
        set_property(TARGET lsp-thing APPEND PROPERTY LINK_FLAGS "-Wl,--as-needed")
        target_link_libraries(lsp-thing -ldl -lrt -lbacktrace)
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
        uv
        ${UNISTRING_LIBRARY}
        -lquadmath
        # ${REAL_LIBEVENT_LIBRARIES}
    )
endif()

#----------------------------------------------------------------------------------------------------

# add_definitions(${${LLVM_AVAILABLE_LIBS}})
# target_include_directories(lsp-thing AFTER PRIVATE
#     ${LLVM_INCLUDE_DIRS}
# )
# target_link_libraries(lsp-thing
#     -L${LLVM_LIBRARY_DIRS}
#     -lLLVM
#     -lclang
#     -lclang-cpp
# )

#include(msgpackc msgpackc-cxx)

target_link_libraries(lsp-thing
    ${CMAKE_THREAD_LIBS_INIT}
    # Threads::Threads
    Boost::boost
    # ${MPI_mpi_LIBRARY}
    # ${MPI_mpi_cxx_LIBRARY}
    #msgpackc
)

#----------------------------------------------------------------------------------------------------

# if (NOT Libevent_FOUND)
#     if (MSVC)
#         message(FATAL_ERROR "Libevent not found. This means something has gone _terribly_ wrong.")
#     else ()
#         message (WARNING "Libevent not found. Assuming it's there anyway.")
#         target_link_libraries(lsp-thing -levent_openssl -levent -levent_core -levent_extra)
#     endif()
# endif()

if (fmt_FOUND)
    message(FATAL_ERROR "Impossible?")
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
    target_link_libraries(lsp-thing ws2_32.lib dbghelp.lib)
endif()

set_property(TARGET lsp-thing APPEND_STRING PROPERTY COMPILE_FLAGS " -I${fmt_INCLUDE_DIRECTORIES}")