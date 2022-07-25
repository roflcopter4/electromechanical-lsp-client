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
      using procinfo_t = util::detail::procinfo_t;

      base_connection            &con_;
      detail::spawner_connection *spawner_;

    public:
      explicit basic_protocol_interface(base_connection &connection)
          : con_(connection)
      {
            spawner_ = dynamic_cast<detail::spawner_connection *>(&con_);
      }

      ~basic_protocol_interface() override = default;

      DELETE_ALL_CTORS(basic_protocol_interface);

      /*------------------------------------------------------------------------------*/

      virtual int notification()   = 0;
      virtual int request()        = 0;
      virtual int response()       = 0;
      virtual int error_response() = 0;

      ND virtual constexpr uv_poll_cb         poll_callback() const       = 0;
      ND virtual constexpr uv_alloc_cb        pipe_alloc_callback() const = 0;
      ND virtual constexpr uv_read_cb         pipe_read_callback() const  = 0;
      ND virtual constexpr uv_timer_cb        timer_callback() const      = 0;
      ND virtual constexpr void const        *data() const                = 0;
      ND virtual constexpr void              *data()                      = 0;
      ND virtual constexpr std::string const &get_key() const &           = 0;
         virtual           void               set_key(std::string key)    = 0;

      /*------------------------------------------------------------------------------*/

      ND procinfo_t const &pid() const & { return con_.pid(); }
      int  waitpid() const noexcept { return con_.waitpid(); }
      void close()   const          { con_.close(); }
      void kill()    const          { con_.kill(); }

      ND detail::base::base_connection_impl_interface       &impl()       & { return con_.impl(); }
      ND detail::base::base_connection_impl_interface const &impl() const & { return con_.impl(); }

      /*------------------------------------------------------------------------------*/

      ssize_t raw_write(std::string const &buf)                 const { return con_.raw_write(buf); }
      ssize_t raw_write(std::string const &buf, int flags)      const { return con_.raw_write(buf, flags); }
      ssize_t raw_write(std::string_view const &buf)            const { return con_.raw_write(buf); }
      ssize_t raw_write(std::string_view const &buf, int flags) const { return con_.raw_write(buf, flags); }

      ssize_t raw_read(void *buf, size_t const nbytes) const
      {
            return con_.raw_read(buf, nbytes);
      }

      ssize_t raw_read(void *buf, size_t const nbytes, int const flags) const
      {
            return con_.raw_read(buf, nbytes, flags);
      }

      ssize_t raw_write(void const *buf, size_t const nbytes) const
      {
            return con_.raw_write(buf, nbytes);
      }

      ssize_t raw_write(void const *buf, size_t const nbytes, int const flags) const
      {
            return con_.raw_write(buf, nbytes, flags);
      }

      ssize_t raw_writev(void *buf, size_t const nbytes, int const flags) const
      {
            return con_.raw_writev(buf, nbytes, flags);
      }

      /*------------------------------------------------------------------------------*/

      ND size_t   available()      const { return con_.available(); }
      ND intptr_t raw_descriptor() const { return con_.raw_descriptor(); }

      ND constexpr detail::base::socket_connection_base_impl *impl_socket()
      {
            assert(spawner_ != nullptr);
            return spawner_->impl_socket();
      }

      ND constexpr detail::base::socket_connection_base_impl const *impl_socket() const
      {
            assert(spawner_ != nullptr);
            return spawner_->impl_socket();
      }

      ND constexpr detail::base::libuv_pipe_handle_impl *impl_libuv()
      {
            assert(spawner_ != nullptr);
            return spawner_->impl_libuv();
      }

      ND constexpr detail::base::libuv_pipe_handle_impl const *impl_libuv() const
      {
            assert(spawner_ != nullptr);
            return spawner_->impl_libuv();
      }
};


template <typename T>
concept ProtocolConnectionVariant = std::derived_from<T, basic_protocol_interface>;


template <typename Connection, template <class> typename IOWrapper>
      REQUIRES (BasicConnectionVariant<Connection> &&
                io::WrapperVariant<IOWrapper<Connection>>)
class basic_protocol_connection
      : public Connection,
        public IOWrapper<Connection>,
        public basic_protocol_interface
{
      using this_type = basic_protocol_connection<Connection, IOWrapper>;

      std::string key_name_;

    public:
      using connection_type = Connection;
      using io_wrapper_type = IOWrapper<Connection>;

      basic_protocol_connection()
          : Connection(),
            io_wrapper_type(dynamic_cast<Connection &>(*this)),
            basic_protocol_interface(dynamic_cast<base_connection &>(*this))
      {}

      ~basic_protocol_connection() override = default;

      DELETE_ALL_CTORS(basic_protocol_connection);

      /*------------------------------------------------------------------------------*/

      /* Doing this to skirt around multiple inheritance feels filthy somehow. */
      using connection_type::available;
      using connection_type::close;
      using connection_type::impl;
      using connection_type::pid;
      using connection_type::raw_descriptor;
      using connection_type::raw_read;
      using connection_type::raw_write;
      using connection_type::raw_writev;
      using connection_type::waitpid;
      using basic_protocol_interface::impl_socket;
      using basic_protocol_interface::impl_libuv;

      /*------------------------------------------------------------------------------*/

      void set_key(std::string key) final
      {
            key_name_ = std::move(key);
      }

      ND constexpr std::string const &get_key() const & final
      {
            return key_name_;
      }
};


/*======================================================================================*/
} // namespace ipc
} // namespace emlsp
#endif
