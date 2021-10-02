#include "Common.hh"
#include "lsp-protocol/lsp-protocol.hh"

/*
 * Required format arguments:
 *   1. Process ID (integer)
 *   2. Root project directory
 */
[[maybe_unused]] static constexpr
char const initial_msg[] =
R"({
      "jsonrpc": "2.0",
      "id": 1,
      "method": "initialize",
      "params": {
            "processId": %d,
            "rootUri":   "%s",
            "trace":     "off",
            "locale":    "en_US.UTF8",
            "clientInfo": {
                  "name": ")" MAIN_PROJECT_NAME R"(",
                  "version: ")" MAIN_PROJECT_VERSION_STRING R"("
            }
            "capabilities": {
            }
      }
})";

namespace emlsp::lsp::protocol {

std::unique_ptr<message>
client::new_message()
{
      return std::make_unique<message>(this);
}

} // namespace emlsp::lsp::protocol
