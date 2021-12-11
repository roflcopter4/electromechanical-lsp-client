#include "Common.hh"

#include <iomanip>

#include <nlohmann/json.hpp>

inline namespace emlsp {
namespace constants {
constexpr auto pi = 3.1415926535897932384626433832795028841971693993751; // pi
} // namespace constants

namespace rpc::json::nloh {

using json = nlohmann::json;

NOINLINE
void
test1()
{
      auto obj      = json::object({{"one", 1}, {"two", 2}});
      obj          += {"test", {{"three", 3}, {"four", 4}}};
      obj["forks"]  = "ouch"s;
      obj["banana"] = constants::pi;

      *obj["banana"].get_ptr<double *>() *= 2;

      obj["params"]  = json::object({{"capabilities", json::object({})}, {"version", "0.0.1"}});
      obj["jsonrpc"] = "2.0";
      obj["id"]      = 1;

      obj["params"]["capabilities"]["depression"] = true;

      std::cout << std::setw(4) << obj << '\n';
}

} // namespace rpc::json::nloh
} // namespace emlsp
