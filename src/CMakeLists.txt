
set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

add_executable(
    lsp-thing

    "main.cc"
    "sigh.cc"
    "testing01.cc"

    "ipc/connection_impl.cc"
    "ipc/rpc/lsp/static-data.cc"

    "util/demangle.cc"
    "util/recode.cc"
    "util/signals.cc"
    "util/util.cc"

    "util/c_util.c"
    "util/err.c"
    "util/tmpfile.c"

    "mpack/format.cc"
    "msgpack/dumper.cc"

    # Include headers to make using MSVC less infuriatingly tedious.
    "Common.hh"
    "pch.hh"
    "rapid.hh"
    "toploop.hh"

    "ipc/basic_connection.hh"
    "ipc/basic_dialer.hh"
    "ipc/basic_io_connection.hh"
    "ipc/basic_io_wrapper.hh"
    "ipc/basic_loop.hh"
    "ipc/basic_rpc_connection.hh"
    "ipc/connection_impl.hh"

    "ipc/rpc/lsp.hh"
    "ipc/rpc/lsp/lsp_connection.hh"
    "ipc/rpc/lsp/static-data.hh"

    "ipc/rpc/neovim.hh"
    "ipc/rpc/neovim/api.hh"
    "ipc/rpc/neovim/neovim_connection.hh"

    "util/concepts.hh"
    "util/exceptions.hh"
    "util/formatters.hh"
    "util/hackish_templates.hh"
    "util/recode.hh"
    "util/strings.hh"
    "util/util.hh"

    "util/c_util.h"
    "util/debug_trap.h"
    "util/initializer_hack.h"
    "util/macros.h"
    "util/myerr.h"
    "util/types.h"

    "msgpack/dumper.hh"

    "${CMAKE_BINARY_DIR}/config.h"
)
