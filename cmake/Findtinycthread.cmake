set(TINYCTHREAD_ROOT "${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}")
find_path(TINYCTHREAD_INCLUDE_DIR NAMES zlib.h PATHS "${TINYCTHREAD_ROOT}/include" NO_DEFAULT_PATH)
find_library(TINYCTHREAD_LIBRARY_RELEASE NAMES tinycthread  z PATHS "${TINYCTHREAD_ROOT}/lib" NO_DEFAULT_PATH)
find_library(TINYCTHREAD_LIBRARY_DEBUG   NAMES tinycthread z PATHS "${TINYCTHREAD_ROOT}/debug/lib" NO_DEFAULT_PATH)
if(NOT TINYCTHREAD_INCLUDE_DIR OR NOT TINYCTHREAD_LIBRARY_RELEASE OR (NOT TINYCTHREAD_LIBRARY_DEBUG AND EXISTS "${TINYCTHREAD_ROOT}/debug/lib"))
    message("Broken installation of vcpkg port zlib")
endif()
if(CMAKE_VERSION VERSION_LESS 3.4)
    include(SelectLibraryConfigurations)
    select_library_configurations(TINYCTHREAD)
    unset(TINYCTHREAD_FOUND)
endif()
#_find_package(${ARGS})