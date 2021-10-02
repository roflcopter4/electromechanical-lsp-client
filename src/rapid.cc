// ReSharper disable CppDeclarationHidesLocal
#include "Common.hh"

#include "rapid.hh"

constexpr auto constant_pi = 3.14159265358979323846;   // pi

#define DUMP_EXCEPTION(e)                                                         \
      std::cerr << fmt::format(R"(Caught exception "{}")" "\n"                    \
                               R"(" at line {}, in file "{}", in function "{}")", \
                               (e).what(), __LINE__, __FILE__, __FUNCTION__)      \
                << std::endl

namespace emlsp::rpc::json::rapid
{

using rapidjson::Document;
using rapidjson::Type;
using rapidjson::Value;

__attribute__((noinline))
void
test1()
{
      auto  *doc       = new Document(Type::kObjectType);
      auto &&allocator = doc->GetAllocator();

#define ADDMEMBER(a, b) AddMember((a), (b), allocator)
#define PUSH(x) PushBack((x), allocator)

      doc->ADDMEMBER("hello", "world");
      doc->ADDMEMBER("you", "suck");
      doc->ADDMEMBER("fucking", "idiot");
      doc->ADDMEMBER("shitstains", 72);
      doc->ADDMEMBER("PI", constant_pi);
      doc->ADDMEMBER("method", "initialize");
      doc->ADDMEMBER("dumbarray", Value{Type::kArrayType});

      try {
            auto obj = Value{Type::kObjectType};
            obj.ADDMEMBER("capabilities", Value{Type::kObjectType});
            obj.ADDMEMBER("version", "0");
            doc->ADDMEMBER("params", std::move(obj));
      } catch (std::exception &e) {
            DUMP_EXCEPTION(e);
      }

      try {
            auto *obj = &doc->FindMember("params")->value.FindMember("capabilities")->value;
            obj->ADDMEMBER("depression", true);
            obj->ADDMEMBER("failure", true);
            obj->ADDMEMBER("confidence", false);
      } catch (std::exception &e) {
            DUMP_EXCEPTION(e);
      }

      try {
            auto *obj = &doc->FindMember("dumbarray")->value;
            obj->PUSH(5);
            obj->PUSH("you stink");
            obj->PUSH("oooh you thinks you's tough?");
            obj->PUSH(~INT64_C(0));
            obj->PUSH(~UINT32_C(0));
            obj->PUSH(~INT32_C(0));
            obj->PUSH(static_cast<double>(static_cast<long double>(UINT64_MAX) * 3.0L));

            auto o2 = Value{Type::kObjectType};
            if (o2.IsArray())
                  throw std::runtime_error("Yeah, it's a fucking array.");

            o2.ADDMEMBER("orly?", "yarly!");
            o2.ADDMEMBER("srs?", "ye");
            o2.ADDMEMBER("?????", true);

            obj->PUSH(o2);

      } catch (std::exception &e) {
            DUMP_EXCEPTION(e);
      }

#undef ADDMEMBER
#undef PUSH

      rapidjson::MemoryBuffer ss;
      rapidjson::Writer       writer(ss);
      doc->Accept(writer);

      fwrite(ss.GetBuffer(), 1, ss.GetSize(), stdout);
      putc('\n', stdout);

      delete doc;
}

__attribute__((noinline))
void
test2()
{
      static constexpr char const json_string[] =
          R"({"hello":"world","you":"suck","fucking":"idiot","shitstains":72,"PI":3.141592653589793,"method":"initialize",)"
          R"("dumbarray":[5,"you stink","oooh you thinks you's tough?",18446744073709551615,4294967295,-1,55340232221128658000.0,)"
          R"({"orly?""yarly!","srs?":"ye","?????":true}],"params":{"capabilities":{"depression":true,"failure":true,"confidence":false},"version":"0"}})";

      Document doc;
      doc.Parse(json_string, sizeof(json_string));

      rapidjson::MemoryBuffer ss;
      rapidjson::PrettyWriter writer(ss);
      doc.Accept(writer);

      fwrite(ss.GetBuffer(), 1, ss.GetSize(), stdout);
}

} // namespace emlsp::rpc::json::rapid
