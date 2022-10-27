
set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

set (lsp-thing_TOPSOURCES
    "main.cc"
    "testing01.cc"
    "testing02.cc"
    "testing03.cc"
)
set(lsp-thing_SOURCES_UTIL_C
    "util/c/c_util.c"
    "util/c/err.c"
    "util/c/tmpfile.c"
)
set (lsp-thing_SOURCES
    # "main.cc"
    # "testing01.cc"
    # "testing02.cc"
    # "testing03.cc"

    "mpack/format.cc"
    "msgpack/dumper.cc"

    "ipc/connection_impl.cc"
    "ipc/rpc/lsp/static-data.cc"

    "util/demangle.cc"
    "util/signals.cc"
    "util/socket.cc"
    "util/util.cc"

    "util/recode/recode.cc"
)

# Include headers to make using MSVC less infuriatingly tedious.
set (lsp-thing_TOPHEADERS
    "Common.hh"
    "pch.hh"
    "lazy.hh"
)
set(lsp-thing_HEADERS_UTIL_C
    "util/c/c_util.h"
    "util/c/debug_trap.h"
    "util/c/initializer.h"
    "util/c/macros.h"
    "util/c/myerr.h"
    "util/c/types.h"
)
set (lsp-thing_HEADERS
    "json/rapid.hh"
    "msgpack/dumper.hh"

    "ipc/toploop.hh"

    "ipc/connection_impl.hh"
    "ipc/basic_sync_thing.hh"
    "ipc/basic_protocol_connection.hh"
    "ipc/ipc_connection.hh"
    "ipc/misc.hh"
    #"ipc/connection_interface.hh"

    "ipc/io/basic_io_wrapper.hh"
    "ipc/io/msgpack_io_wrapper.hh"
    "ipc/io/ms_jsonrpc_io_wrapper.hh"

    "ipc/protocols/msgpack_connection.hh"
    "ipc/protocols/ms_jsonrpc_connection.hh"

    "ipc/rpc/lsp.hh"
    "ipc/rpc/lsp/static-data.hh"
    "ipc/rpc/neovim.hh"
    "ipc/rpc/rpc.hh"

    # "util/errno.hh"
    "util/concepts.hh"
    "util/formatters.hh"
    "util/hackish_templates.hh"
    "util/misc.hh"
    "util/util.hh"

    "util/recode/recode.hh"
    "util/recode/recode_private.hh"
    "util/recode/strlen.hh"
)


add_executable(lsp-thing
    ${lsp-thing_SOURCES}
    ${lsp-thing_HEADERS}
    ${lsp-thing_TOPSOURCES}
    ${lsp-thing_TOPHEADERS}
    ${lsp-thing_SOURCES_UTIL_C}
    ${lsp-thing_HEADERS_UTIL_C}

    "${CMAKE_BINARY_DIR}/config.h"
)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}"
             PREFIX "Source Files/src"
             FILES ${lsp-thing_SOURCES} ${lsp-thing_HEADERS})
source_group("Source Files/src/zzz-Root"
             FILES ${lsp-thing_TOPSOURCES} ${lsp-thing_TOPHEADERS})
source_group("Source Files/src/util/C"
             FILES ${lsp-thing_SOURCES_UTIL_C} ${lsp-thing_HEADERS_UTIL_C})

unset (lsp-thing_SOURCES)
unset (lsp-thing_HEADERS)
unset (lsp-thing_TOPSOURCES)
unset (lsp-thing_TOPHEADERS)
unset (lsp-thing_SOURCES_UTIL_C)
unset (lsp-thing_HEADERS_UTIL_C)
