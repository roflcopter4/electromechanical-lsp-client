
set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

set (lsp-thing_SOURCES
    "main.cc"
    "testing01.cc"
    "testing02.cc"

    "mpack/format.cc"
    "msgpack/dumper.cc"

    "ipc/connection_impl.cc"
    "ipc/rpc/lsp/static-data.cc"

    "util/c_util.c"
    "util/err.c"
    "util/tmpfile.c"

    "util/demangle.cc"
    "util/signals.cc"
    "util/util.cc"

    "util/recode/recode.cc"
)


# Include headers to make using MSVC less infuriatingly tedious.
set (lsp-thing_HEADERS
    "Common.hh"
    "pch.hh"
    "rapid.hh"
    "toploop.hh"

    "msgpack/dumper.hh"

    "ipc/connection_impl.hh"
    "ipc/basic_sync_thing.hh"
    "ipc/basic_protocol_connection.hh"
    "ipc/ipc_connection.hh"
    "ipc/misc.hh"

    "ipc/io/basic_io_wrapper.hh"
    "ipc/io/msgpack_io_wrapper.hh"
    "ipc/io/ms_jsonrpc_io_wrapper.hh"

    "ipc/protocols/msgpack_connection.hh"
    "ipc/protocols/ms_jsonrpc_connection.hh"

    "ipc/rpc/lsp.hh"
    "ipc/rpc/lsp/static-data.hh"
    "ipc/rpc/neovim.hh"

    "util/c_util.h"
    "util/debug_trap.h"
    "util/initializer_hack.h"
    "util/macros.h"
    "util/myerr.h"
    "util/types.h"

    # "util/errno.hh"
    "util/concepts.hh"
    "util/exceptions.hh"
    "util/formatters.hh"
    "util/hackish_templates.hh"
    "util/strings.hh"
    "util/util.hh"

    "util/recode/recode.hh"
    "util/recode/recode_private.hh"
    "util/recode/strlen.hh"

    "${CMAKE_BINARY_DIR}/config.h"
)

add_executable(lsp-thing
    ${lsp-thing_SOURCES}
    ${lsp-thing_HEADERS}
)

unset (lsp-thing_SOURCES)
unset (lsp-thing_HEADERS)
