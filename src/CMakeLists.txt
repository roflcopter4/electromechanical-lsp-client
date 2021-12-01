# /src

set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

add_executable(lsp-thing
    # $<TARGET_OBJECTS:x4c_parser>
    main.cc
    #nlohmann.cc
    rapid.cc
    #rpc.cc
    connection_impl.cc

    event/loop.cc
    event/event.cc
    event/test.cc
    lsp-protocol/static-data.cc
    lsp-protocol/lsp-client.cc
    mpack-cxx/mpack.cc

    mpack/format.cc

    msgpack/dumper.cc

    basic_loop.cc
    why/simple.cc

    util/err.c
    util/tmpfile.c
    util/util.cc
    util/c_util.c
)

if (TRUE OR NOT "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
   target_precompile_headers(lsp-thing
       PRIVATE
       $<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/pch.hh>
   )
endif()

set_property(TARGET lsp-thing APPEND PROPERTY INCLUDE_DIRECTORIES
             "${CMAKE_CURRENT_SOURCE_DIR}"
             # "${GLib_INCLUDE_DIRS}"
             # ${_glib_cflags}
             # $<JOIN:${GLib_INCLUDE_DIRS}, >
             ${LIBUV_INCLUDE_DIRS}
             "${CMAKE_SOURCE_DIR}/contrib/rapidjson/include"
             "${CMAKE_SOURCE_DIR}/include/uvw/src"
             ${fmt_INCLUDE_DIRECTORIES}
)

list(JOIN GLib_PKG_LIBS " " _glib_ldflags)

if (MSVC)
    list(JOIN GLib_PKG_CFLAGS " " _glib_cflags )
    message("${_glib_cflags}")
    set_property(TARGET lsp-thing APPEND PROPERTY COMPILE_FLAGS "${_glib_cflags}")
else ()
    list(JOIN GLib_PKG_CFLAGS " " _glib_cflags )
    message("${_glib_cflags}")
    set_property(TARGET lsp-thing APPEND PROPERTY COMPILE_FLAGS "${_glib_cflags}")
    set_property(SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/pch.hh APPEND PROPERTY COMPILE_OPTIONS
            -fpch-debuginfo -fpch-instantiate-templates -fpch-codegen -relocatable-pch
    )
endif()

################################################################################
# Libraries

if (MSVC)
    if (${CMAKE_BUILD_TYPE} STREQUAL "Debug" OR ${CMAKE_BUILD_TYPE} STREQUAL "RelWithDebInfo")
        set (_WIN32_VCPKG_LIB_PATH "${_VCPKG_INSTALLED_DIR}/x64-windows/debug/lib")
        target_link_libraries(lsp-thing ${TINYCTHREAD_LIBRARY_DEBUG})
    else()
        set (_WIN32_VCPKG_LIB_PATH "${_VCPKG_INSTALLED_DIR}/x64-windows/lib")
        target_link_libraries(lsp-thing ${TINYCTHREAD_LIBRARY_RELEASE})
    endif()
elseif (NOT HAVE_THREADS_H)
    add_library(tinycthread OBJECT "${CMAKE_SOURCE_DIR}/contrib/tinycthread/source/tinycthread.c")
    target_link_libraries(lsp-thing tinycthread)
    set_property(TARGET lsp-thing APPEND PROPERTY INCLUDE_DIRECTORIES
                 "${CMAKE_SOURCE_DIR}/contrib/tinycthread/source")
endif()

target_link_libraries(lsp-thing
    ${CMAKE_THREAD_LIBS_INIT}
    ${GLib_LIBRARY}
	${GLib_PKG_LIBS}

    ${GLib_LIBRARIES}
    ${LIBUV_LIBRARIES}
    #${LIBUV_LIBRARY}
)

if (fmt_FOUND)
	target_link_libraries(lsp-thing fmt::fmt-header-only)
endif()

if (MSVC)
	target_link_libraries(lsp-thing
		"${_WIN32_VCPKG_LIB_PATH}/libuv.lib"
		"${_WIN32_VCPKG_LIB_PATH}/glib-2.0.lib"
		"${_WIN32_VCPKG_LIB_PATH}/intl.lib"
		"${_WIN32_VCPKG_LIB_PATH}/msgpackc.lib"
    )
endif()

if (USE_JEMALLOC STREQUAL "YES")
    target_link_libraries(x4c ${JEMALLOC_LIBRARY})
endif()

if (WIN32)
    target_link_libraries(lsp-thing
        ws2_32.lib
    )
endif()
