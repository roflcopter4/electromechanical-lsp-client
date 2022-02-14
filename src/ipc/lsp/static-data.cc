#include "Common.hh"
#include "ipc/lsp/static-data.hh"

inline namespace emlsp {
namespace ipc::lsp::data {

/*
 * Required input
 *   - Process ID
 *   - Root URI
 */
static constexpr char const initialization_message[] =
R"({
    "jsonrpc": "2.0",
    "id": 0,
    "method": "initialize",
    "params": {
         "capabilities": {
             "textDocument": {
                 "semanticTokens": {
                     "dynamicRegistration": true,
                     "requests": {
                         "range": true,
                         "full": {
                             "delta": true
                         }
                     }
                 },
                 "semanticHighlightingCapabilities": {
                     "semanticHighlighting": true
                 },
                 "colorProvider": {
                     "dynamicRegistration": true
                 },
                 "publishDiagnostics": {
                     "relatedInformation": true,
                     "versionSupport": true,
                     "codeDescriptionSupport": true,
                     "dataSupport": true
                 },
                 "typeDefinition": {
                     "linkSupport": true
                 },
                  "codeLens": {
                     "dynamicRegistration": true
                 }
             },

             "workspace": {
                 "applyEdit": false,
                 "didChangeWatchedFiles": {
                     "dynamicRegistration": true
                  }
             }
         },
         "locale": "en_US.UTF8",
         "clientInfo": {
             "name": ")" MAIN_PROJECT_NAME R"(",
             "version": ")" MAIN_PROJECT_VERSION_STRING R"("
         },
         "processId": %d,
         "rootUri": "%s",
         "trace": "verbose"
    }
})";


std::string
init_msg(char const *root)
{
      return fmt::sprintf(FMT_STRING(initialization_message), getpid(), root);
}


UNUSED static constexpr char const foobarbaz[] =
R"({
  "id": 0,
  "jsonrpc": "2.0",
  "method": "initialize",
  "params": {
    "capabilities": {
      "textDocument": {
        "codeAction": {
          "codeActionLiteralSupport": {
            "codeActionKind": {
              "valueSet": [
                "quickfix",
                "refactor",
                "refactor.extract",
                "refactor.inline",
                "refactor.rewrite",
                "source",
                "source.organizeImports"
              ]
            }
          }
        },
        "codeLens": {
          "dynamicRegistration": true
        },
        "colorProvider": {
          "dynamicRegistration": false
        },
        "completion": {
          "completionItem": {
            "documentationFormat": [
              "plaintext",
              "markdown"
            ],
            "insertReplaceSupport": false,
            "snippetSupport": false
          }
        },
        "declaration": {
          "linkSupport": true
        },
        "definition": {
          "linkSupport": true
        },
        "hover": {
          "contentFormat": [
            "plaintext",
            "markdown"
          ]
        },
        "implementation": {
          "linkSupport": true
        },
        "publishDiagnostics": {
          "relatedInformation": true
        },
        "semanticHighlightingCapabilities": {
          "semanticHighlighting": true
        },
        "signatureHelp": {
          "signatureInformation": {
            "documentationFormat": [
              "plaintext",
              "markdown"
            ],
            "parameterInformation": {
              "labelOffsetSupport": true
            }
          }
        },
        "typeDefinition": {
          "linkSupport": true
        }
      },
      "workspace": {
        "applyEdit": true,
        "didChangeWatchedFiles": {
          "dynamicRegistration": true
        }
      }
    },
    "clientInfo": {
      "name": "LanguageClient-neovim",
      "version": "0.1.161"
    },
    "processId": 16652,
    "rootPath": "/home/bml/files/Projects/elemwin",
    "rootUri": "file:///home/bml/files/Projects/elemwin",
    "trace": "verbose"
  }
})";



} // namespace ipc::lsp::data
} // namespace emlsp
