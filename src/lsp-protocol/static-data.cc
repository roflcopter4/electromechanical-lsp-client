#include "Common.hh"
#include "static-data.hh"

namespace emlsp::rpc::lsp::data {

#if 0
std::string const initialization_message =
R"({
      "jsonrpc": "2.0",
      "id": 1,
      "method": "initialize",
      "params": {
            "trace":     "on",
            "locale":    "en_US.UTF8",
            "clientInfo": {
                  "name": ")" MAIN_PROJECT_NAME R"(",
                  "version": ")" MAIN_PROJECT_VERSION_STRING R"("
            },
            "capabilities": {
                  "textDocument.documentHighlight.dynamicRegistration": true
            }
      }
})"s;
#endif

} // namespace emlsp::rpc::lsp::data

namespace emlsp::test {

NOINLINE void test01()
{
}

} // namespace emlsp::test 
