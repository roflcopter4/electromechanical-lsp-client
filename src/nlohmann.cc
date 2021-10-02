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
      obj          += {"fuck", {{"three", 3}, {"four", 4}}};
      obj["forks"]  = "ouch";
      obj["banana"] = constants::pi;

      *obj["banana"].get_ptr<double *>() *= 2;

      obj["params"]  = json::object({{"capabilities", json::object({})}, {"version", "0.0.1"}});
      obj["jsonrpc"] = "2.0";
      obj["id"]      = 1;

      obj["params"]["capabilities"]["depression"] = true;

      std::cout << std::setw(4) << obj << '\n';
}

__attribute__((noinline))
void
test2()
{
      static constexpr char const json_string[] =
          R"({"hello":"world","you":"suck","fucking":"idiot","shitstains":72,"PI":3.141592653589793,)"
          R"("method":"initialize","dumbarray":[5,"you stink","oooh you thinks you's tough?",)"
          R"(18446744073709551615,4294967295,-1,55340232221128658000.0,{"orly?":"yarly!","srs?":"ye",)"
          R"("?????":true}],"params":{"capabilities":{"depression":true,"failure":true,"confidence":false},)"
          R"("version":"0"}})";

      try {
            auto const obj = json::parse(std::string{json_string, sizeof json_string - SIZE_C(1)});
            std::cout << obj << "\n\n"
                      << std::setw(4) << obj << '\n';
      } catch (nlohmann::detail::exception &e) {
            std::cerr << e.what() << std::endl;
      }
}

} // namespace emlsp::rpc::json::nloh
