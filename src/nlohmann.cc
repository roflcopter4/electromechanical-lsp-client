#include "Common.hh"

#include <iomanip>

namespace constants
{
constexpr auto pi = 3.14159265358979323846; // pi
} // namespace constants

#include <nlohmann/json.hpp>

namespace emlsp::rpc::json::nloh
{

using json = nlohmann::json;

__attribute__((noinline))
void
test1()
{
      auto obj      = json::object({{"one", 1}, {"two", 2}});
      obj          += {"test", {{"three", 3}, {"four", 4}}};
      obj["forks"]  = "ouch";
      obj["banana"] = constants::pi;

      *obj["banana"].get_ptr<double *>() *= 2;

      obj["params"]  = json::object({{"capabilities", json::object({})}, {"version", "0.0.1"}});
      obj["jsonrpc"] = "2.0";
      obj["id"]      = 1;

      obj["params"]["capabilities"]["depression"] = true;

      std::cout << std::setw(4) << obj << '\n';
}

} // namespace emlsp::rpc::json::nloh
