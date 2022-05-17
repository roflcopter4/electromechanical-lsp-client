#include "Common.hh"
#include "util/util.hh"

#if defined __GNUG__ || defined _LIBCPP_VERSION
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
demangle(_In_z_ char const *raw_name)
{
      char const *name;

#if defined _MSC_VER
      // BUG Doesn't seem to work at all? Meh.
      char buf[4096];
      UnDecorateSymbolName(raw_name, buf, sizeof(buf), UNDNAME_COMPLETE);
      name = buf;
#elif defined __GNUG__ || defined _LIBCPP_VERSION
      int  status;
      auto demangled = std::unique_ptr<char, void (*)(void *)>{
          abi::__cxa_demangle(raw_name, nullptr, nullptr, &status), ::free};

      if (status != 0) {
            fmt::print(stderr, FC("Failed to demangle \"{}\"; status: {}\n"),
                       raw_name, status);
            name = raw_name;
      } else {
            name = demangled.get();
      }
#else
      name = raw_name;
#endif

      return std::string{name};
}


std::string
demangle(std::type_info const &id)
{
      return demangle(id.name());
}


/****************************************************************************************/
} // namespace util
} // namespace emlsp
