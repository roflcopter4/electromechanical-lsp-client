#include "Common.hh"
#include "util/util.hh"

#if defined __GNUC__ && defined __GNU_LIBRARY__
#  include <cxxabi.h>
#elif defined _MSC_VER
#  include <dbghelp.h>
#  pragma comment(lib, "dbghelp.lib")
#endif

inline namespace emlsp {
namespace util {
/****************************************************************************************/


#if defined _MSC_VER
INITIALIZER_HACK(demangle_setup)
{
      SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS);

      if (!SymInitialize(GetCurrentProcess(), nullptr, true))
            win32::error_exit(L"SymInitialize()");
}
#endif


std::string
demangle(_Notnull_ char const *raw_name)
{
      char const *name;

#if defined __GNUG__ && defined __GNU_LIBRARY__
      int  status;
      auto demangled = std::unique_ptr<char, void (*)(void *)>{
          abi::__cxa_demangle(raw_name, nullptr, nullptr, &status), ::free};

      if (status != 0) {
            fmt::print(stderr, FMT_COMPILE("Failed to demangle \"{}\"; status: {}\n"),
                       raw_name, status);
            name = raw_name;
      } else {
            name = demangled.get();
      }
#elif defined _MSC_VER
      // BUG Doesn't seem to work at all? Meh.
      char buf[4096];
      UnDecorateSymbolName(raw_name, buf, sizeof(buf), UNDNAME_COMPLETE);
      name = buf;
#else
      name = raw_name;
#endif

      return std::string{name};
}


std::string
demangle(std::type_info const &id)
{
#if defined __GNUG__
      return demangle(id.name());
#else
      return id.name();
#endif
}


/****************************************************************************************/
} // namespace util
} // namespace emlsp
