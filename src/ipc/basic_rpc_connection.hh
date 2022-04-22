#pragma once
#ifndef HGUARD__IPC__BASIC_RPC_CONNECTION_HH_
#define HGUARD__IPC__BASIC_RPC_CONNECTION_HH_ //NOLINT
/****************************************************************************************/
#include "Common.hh"

#include "ipc/basic_connection.hh"
#include "ipc/basic_dialer.hh"
#include "ipc/basic_io_connection.hh"
#include "ipc/basic_io_wrapper.hh"

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
class transport_interface
{
      using this_type  = transport_interface<Value>;

    public:
      using value_type = Value;

      virtual ~transport_interface()                                  = default;
      transport_interface(transport_interface const &)                = delete;
      transport_interface(transport_interface &&) noexcept            = delete;
      transport_interface &operator=(transport_interface const &)     = delete;
      transport_interface &operator=(transport_interface &&) noexcept = delete;

      virtual int notify(util::string_ref Method, Value Params)            = 0;
      virtual int request(util::string_ref Method, Value Params, Value ID) = 0;
      virtual int reply(Value ID, Value Result)                            = 0;

      class MessageHandler
      {
          public:
            virtual ~MessageHandler()                             = default;
            MessageHandler(MessageHandler const &)                = default;
            MessageHandler(MessageHandler &&) noexcept            = default;
            MessageHandler &operator=(MessageHandler const &)     = default;
            MessageHandler &operator=(MessageHandler &&) noexcept = default;

            // Handler returns true to keep processing messages, or false to shut down.
            virtual int on_notify(util::string_ref Method, Value Params)            = 0;
            virtual int on_request(util::string_ref Method, Value Params, Value ID) = 0;
            virtual int on_reply(Value ID, Value Result)                            = 0;
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


template <typename Connection, template <typename> class IOWrapper>
      REQUIRES (IsBasicConnectionVariant<Connection> &&
                io::IsWrapperVariant<IOWrapper<Connection>>)
class basic_rpc_connection : public basic_io_connection<Connection, IOWrapper>
{
      using this_type = basic_rpc_connection<Connection, IOWrapper>;
      using base_type = basic_io_connection<Connection, IOWrapper>;

      std::atomic_uint64_t request_id_ = 0;

    protected:
      bool want_close_ = false;

    public:
      using connection_type = Connection;
      using io_wrapper_type = IOWrapper<Connection>;
      using value_type      = typename io_wrapper_type::value_type;

      basic_rpc_connection()           = default;
      ~basic_rpc_connection() override = default;

      basic_rpc_connection(basic_rpc_connection const &)                = delete;
      basic_rpc_connection(basic_rpc_connection &&) noexcept            = default;
      basic_rpc_connection &operator=(basic_rpc_connection const &)     = delete;
      basic_rpc_connection &operator=(basic_rpc_connection &&) noexcept = default;

      /*------------------------------------------------------------------------------*/

      virtual int notification()   = 0;
      virtual int request()        = 0;
      virtual int response()       = 0;
      virtual int error_response() = 0;

      ND virtual uv_poll_cb  poll_callback() const  = 0;
      ND virtual uv_timer_cb timer_callback() const = 0;
      ND virtual void       *data()                 = 0;
};


/*======================================================================================*/


class callback_bridge
{
    public:
      callback_bridge()          = default;
      virtual ~callback_bridge() = default;

      DEFAULT_ALL_CTORS(callback_bridge);

      /*------------------------------------------------------------------------------*/

      ND virtual uv_poll_cb  poll_callback() const  = 0;
      ND virtual uv_timer_cb timer_callback() const = 0;
      ND virtual void       *data()                 = 0;

      ND virtual std::type_info const &type_info() const & = 0;
};


/*======================================================================================*/

} // namespace ipc
} // namespace emlsp

#include "ipc/rpc/lsp.hh"
#include "ipc/rpc/neovim.hh"

inline namespace emlsp {
namespace ipc {


template <typename Ty>
class neovim_callback_bridge final : public callback_bridge
{
    public:
      explicit neovim_callback_bridge(Ty *connection)
            : connection_(connection)
      {}
      explicit neovim_callback_bridge(std::unique_ptr<Ty> &&connection)
            : connection_(std::move(connection))
      {}

      ~neovim_callback_bridge() override = default;

      neovim_callback_bridge(neovim_callback_bridge &&) noexcept            = default;
      neovim_callback_bridge(neovim_callback_bridge const &)                = delete;
      neovim_callback_bridge &operator=(neovim_callback_bridge &&) noexcept = default;
      neovim_callback_bridge &operator=(neovim_callback_bridge const &)     = delete;

      /*------------------------------------------------------------------------------*/

      ND uv_poll_cb  poll_callback() const override { return nullptr; }
      ND uv_timer_cb timer_callback() const override { return [](uv_timer_t *){/**/}; }

      ND void *data() override { return connection_.get(); }
      ND std::type_info const &type_info() const & override { return typeid(*this); }

      ND Ty *con() { return connection_.get(); }

    private:
      std::unique_ptr<Ty> connection_;
};


/****************************************************************************************/
} // namespace ipc
} // namespace emlsp
#endif
