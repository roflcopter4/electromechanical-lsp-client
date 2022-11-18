# - Find UNISTRING
# Find the unistring includes and library
#
#  UNISTRING_INCLUDE_DIR - where to find unistr.h, etc.
#  UNISTRING_LIBRARY     - unistring library.
#  UNISTRING_FOUND       - True if unistring found.

if (UNISTRING_INCLUDE_DIR)
    # Already in cache, be silent
    set (UNISTRING_FIND_QUIETLY TRUE)
endif (UNISTRING_INCLUDE_DIR)

find_path(UNISTRING_INCLUDE_DIR unistr.h)

set (UNISTRING_NAMES unistring libunistring)
find_library (UNISTRING_LIBRARY NAMES ${UNISTRING_NAMES})

# handle the QUIETLY and REQUIRED arguments and set UNISTRING_FOUND to
# TRUE if all listed variables are TRUE
include (FindPackageHandleStandardArgs)
find_package_handle_standard_args (
    Unistring
    DEFAULT_MSG
    UNISTRING_INCLUDE_DIR
    UNISTRING_LIBRARY
)

mark_as_advanced (UNISTRING_LIBRARY UNISTRING_INCLUDE_DIR UNISTRING_LIBRARY)
