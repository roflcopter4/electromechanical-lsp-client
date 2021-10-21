# /src

set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

add_executable(lsp-thing
    # $<TARGET_OBJECTS:x4c_parser>
    main.cc
    nlohmann.cc
    rapid.cc
    rpc.cc
    client.cc

    event/loop.cc
    event/event.cc
    lsp-protocol/static-data.cc

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
             "${GLib_INCLUDE_DIRS}"
)

set_property(SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/pch.hh APPEND PROPERTY COMPILE_OPTIONS
        -fpch-debuginfo -fpch-instantiate-templates -fpch-codegen -relocatable-pch
)

################################################################################
# Libraries

if (WIN32)
    if (${CMAKE_BUILD_TYPE} STREQUAL "Debug" OR ${CMAKE_BUILD_TYPE} STREQUAL "RelWithDebInfo")
        set (_WIN32_VCPKG_LIB_PATH "${_VCPKG_INSTALLED_DIR}/debug/lib")
		target_link_libraries(lsp-thing ${TINYCTHREAD_LIBRARY_DEBUG})
    else()
        set (_WIN32_VCPKG_LIB_PATH "${_VCPKG_INSTALLED_DIR}/x64-windows/lib")
		target_link_libraries(lsp-thing ${TINYCTHREAD_LIBRARY_RELEASE})
    endif()
endif()

target_link_libraries(lsp-thing
    ${CMAKE_THREAD_LIBS_INIT}
    #fmt::fmt
    rapidjson
    nlohmann_json
    ${GLib_LIBRARY}
    #-luv
)

if (USE_JEMALLOC STREQUAL "YES")
    target_link_libraries(x4c ${JEMALLOC_LIBRARY})
endif()

if (WIN32)
    target_link_libraries(lsp-thing
        ws2_32.lib
    )
endif()
