#include "Common.hh"
#include "lsp-protocol/lsp-protocol.hh"
#include "util/exceptions.hh"

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

/****************************************************************************************/

namespace emlsp::rpc::lsp {

/*--------------------------------------------------------------------------------------*/


/*--------------------------------------------------------------------------------------*/

#if 0
int
client::spawn_connection(connection_type const contype, char const *prog, ...)
{
      va_list  ap;
      va_start(ap, prog);
      auto ret = spawn_connection(contype, prog, ap);
      va_end(ap);
      return ret;
}
#endif

#if 0
template <typename... Types>
int client::spawn_connection(_Notnull_ char const *prog, Types &&...args)
{
      char const *unpack[] = {args...};
      return spawn_connection(get_connection_type(), prog, const_cast<char **>(unpack));
}

template <typename... Types>
int client::spawn_connection(connection_type       contype,
                             _Notnull_ char const *prog,
                             Types &&...args)
{
      char const *unpack[] = {args...};
      return spawn_connection(contype, prog, const_cast<char **>(unpack));
}
#endif

#if 0
int
client::spawn_connection(connection_type const contype, char const *prog, va_list ap)
{
      unsigned nargs = 0;
      {
            va_list cp;
            va_copy(cp, ap);
            while (va_arg(cp, char *))
                  ++nargs;
            va_end(cp);
      }
      
      char **argv = new char *[nargs + 1];
      char **ptr = argv;
      char *cur;
      *ptr++ = const_cast<char *>(prog);

      while ((cur = va_arg(ap, char *)))
            *ptr++ = cur;

      auto ret = spawn_connection(contype, prog, argv);
      delete[] argv;
      return ret;
}
#endif

} // namespace emlsp::rpc::lsp
