#pragma once
#ifndef HGUARD__IPC__BASIC_PROTOCOL_CONNECTION_HH_
#define HGUARD__IPC__BASIC_PROTOCOL_CONNECTION_HH_ //NOLINT
/****************************************************************************************/
#include "Common.hh"

#include "ipc/basic_sync_thing.hh"
#include "ipc/ipc_connection.hh"

inline namespace emlsp {
namespace ipc {
/****************************************************************************************/


namespace stolen {

namespace detail {
template <typename T>
using expected = std::pair<T, int>;

template <typename T>
using callback = std::function<void(expected<T>)>;
} // namespace detail


/*======================================================================================*/


template <typename Value>
      REQUIRES(false)
class transport_interface
{
      using this_type = transport_interface<Value>;
      using str_ref   = util::string_ref<char>;

    public:
      using value_type = Value;

      virtual ~transport_interface()                                  = default;
      transport_interface(transport_interface const &)                = delete;
      transport_interface(transport_interface &&) noexcept            = delete;
      transport_interface &operator=(transport_interface const &)     = delete;
      transport_interface &operator=(transport_interface &&) noexcept = delete;

      virtual int notify(str_ref Method, Value Params)            = 0;
      virtual int request(str_ref Method, Value Params, Value ID) = 0;
      virtual int reply(Value ID, Value Result)                   = 0;

      class MessageHandler
      {
          public:
            virtual ~MessageHandler()                             = default;
            MessageHandler(MessageHandler const &)                = default;
            MessageHandler(MessageHandler &&) noexcept            = default;
            MessageHandler &operator=(MessageHandler const &)     = default;
            MessageHandler &operator=(MessageHandler &&) noexcept = default;

            // Handler returns true to keep processing messages, or false to shut down.
            virtual int on_notify(str_ref Method, Value Params)            = 0;
            virtual int on_request(str_ref Method, Value Params, Value ID) = 0;
            virtual int on_reply(Value ID, Value Result)                   = 0;
      };

      virtual int dispatch(MessageHandler &) = 0;
};


} // namespace stolen


/*
  Bind.notification("initialized",                      this, &ClangdLSPServer::onInitialized);
  Bind.notification("textDocument/didOpen",             this, &ClangdLSPServer::onDocumentDidOpen);
  Bind.notification("textDocument/didClose",            this, &ClangdLSPServer::onDocumentDidClose);
  Bind.notification("textDocument/didChange",           this, &ClangdLSPServer::onDocumentDidChange);
  Bind.notification("textDocument/didSave",             this, &ClangdLSPServer::onDocumentDidSave);
  Bind.notification("workspace/didChangeConfiguration", this, &ClangdLSPServer::onChangeConfiguration);
  Bind.notification("workspace/didChangeWatchedFiles",  this, &ClangdLSPServer::onFileEvent);
  Bind.method("$/memoryUsage",                          this, &ClangdLSPServer::onMemoryUsage);
  Bind.method("callHierarchy/incomingCalls",            this, &ClangdLSPServer::onCallHierarchyIncomingCalls);
  Bind.method("clangd/inlayHints",                      this, &ClangdLSPServer::onInlayHints);
  Bind.method("shutdown",                               this, &ClangdLSPServer::onShutdown);
  Bind.method("sync",                                   this, &ClangdLSPServer::onSync);
  Bind.method("textDocument/ast",                       this, &ClangdLSPServer::onAST);
  Bind.method("textDocument/codeAction",                this, &ClangdLSPServer::onCodeAction);
  Bind.method("textDocument/completion",                this, &ClangdLSPServer::onCompletion);
  Bind.method("textDocument/declaration",               this, &ClangdLSPServer::onGoToDeclaration);
  Bind.method("textDocument/definition",                this, &ClangdLSPServer::onGoToDefinition);
  Bind.method("textDocument/documentHighlight",         this, &ClangdLSPServer::onDocumentHighlight);
  Bind.method("textDocument/documentLink",              this, &ClangdLSPServer::onDocumentLink);
  Bind.method("textDocument/documentSymbol",            this, &ClangdLSPServer::onDocumentSymbol);
  Bind.method("textDocument/foldingRange",              this, &ClangdLSPServer::onFoldingRange);
  Bind.method("textDocument/formatting",                this, &ClangdLSPServer::onDocumentFormatting);
  Bind.method("textDocument/hover",                     this, &ClangdLSPServer::onHover);
  Bind.method("textDocument/implementation",            this, &ClangdLSPServer::onGoToImplementation);
  Bind.method("textDocument/onTypeFormatting",          this, &ClangdLSPServer::onDocumentOnTypeFormatting);
  Bind.method("textDocument/prepareCallHierarchy",      this, &ClangdLSPServer::onPrepareCallHierarchy);
  Bind.method("textDocument/prepareRename",             this, &ClangdLSPServer::onPrepareRename);
  Bind.method("textDocument/rangeFormatting",           this, &ClangdLSPServer::onDocumentRangeFormatting);
  Bind.method("textDocument/references",                this, &ClangdLSPServer::onReference);
  Bind.method("textDocument/rename",                    this, &ClangdLSPServer::onRename);
  Bind.method("textDocument/selectionRange",            this, &ClangdLSPServer::onSelectionRange);
  Bind.method("textDocument/semanticTokens/full",       this, &ClangdLSPServer::onSemanticTokens);
  Bind.method("textDocument/semanticTokens/full/delta", this, &ClangdLSPServer::onSemanticTokensDelta);
  Bind.method("textDocument/signatureHelp",             this, &ClangdLSPServer::onSignatureHelp);
  Bind.method("textDocument/switchSourceHeader",        this, &ClangdLSPServer::onSwitchSourceHeader);
  Bind.method("textDocument/symbolInfo",                this, &ClangdLSPServer::onSymbolInfo);
  Bind.method("textDocument/typeDefinition",            this, &ClangdLSPServer::onGoToType);
  Bind.method("textDocument/typeHierarchy",             this, &ClangdLSPServer::onTypeHierarchy);
  Bind.method("typeHierarchy/resolve",                  this, &ClangdLSPServer::onResolveTypeHierarchy);
  Bind.method("workspace/executeCommand",               this, &ClangdLSPServer::onCommand);
  Bind.method("workspace/symbol",                       this, &ClangdLSPServer::onWorkspaceSymbol);
  Bind.command(ApplyFixCommand,                         this, &ClangdLSPServer::onCommandApplyEdit);
  Bind.command(ApplyTweakCommand,                       this, &ClangdLSPServer::onCommandApplyTweak);

  ApplyWorkspaceEdit     = Bind.outgoingMethod("workspace/applyEdit");
  PublishDiagnostics     = Bind.outgoingNotification("textDocument/publishDiagnostics");
  ShowMessage            = Bind.outgoingNotification("window/showMessage");
  NotifyFileStatus       = Bind.outgoingNotification("textDocument/clangd.fileStatus");
  CreateWorkDoneProgress = Bind.outgoingMethod("window/workDoneProgress/create");
  BeginWorkDoneProgress  = Bind.outgoingNotification("$/progress");
  ReportWorkDoneProgress = Bind.outgoingNotification("$/progress");
  EndWorkDoneProgress    = Bind.outgoingNotification("$/progress");
  SemanticTokensRefresh  = Bind.outgoingMethod("workspace/semanticTokens/refresh");
 */


/*======================================================================================*/


class basic_protocol_interface
      : public basic_sync_thing
{
    protected:
      bool want_close_ = false;

    public:
      basic_protocol_interface()            = default;
       ~basic_protocol_interface() override = default;

      DELETE_ALL_CTORS(basic_protocol_interface);

      /*------------------------------------------------------------------------------*/

      virtual int notification()   = 0;
      virtual int request()        = 0;
      virtual int response()       = 0;
      virtual int error_response() = 0;

      ND virtual uv_poll_cb  poll_callback() const       = 0;
      ND virtual uv_timer_cb timer_callback() const      = 0;
      ND virtual uv_alloc_cb pipe_alloc_callback() const = 0;
      ND virtual uv_read_cb  pipe_read_callback() const  = 0;
      ND virtual void       *data()                      = 0;
};


template <typename T>
concept BasicProtocolConnectionVariant = std::derived_from<T, basic_protocol_interface>;


template <typename Connection, template <class> typename IOWrapper>
class basic_protocol_connection
      : public Connection,
        public IOWrapper<Connection>,
        public basic_protocol_interface
{
      using this_type = basic_protocol_connection<Connection, IOWrapper>;

    public:
      using connection_type = Connection;
      using io_wrapper_type = IOWrapper<Connection>;

      basic_protocol_connection()
            : Connection(), io_wrapper_type(dynamic_cast<Connection *>(this))
      {}

      ~basic_protocol_connection() override = default;

      DELETE_ALL_CTORS(basic_protocol_connection);
};


/*======================================================================================*/
} // namespace ipc
} // namespace emlsp
#endif
