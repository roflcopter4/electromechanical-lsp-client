// ReSharper disable CppDeclarationHidesLocal
#include "Common.hh"
#include "rapid.hh"

constexpr auto constant_pi = 3.14159265358979323846;   // pi

inline namespace emlsp {
namespace ipc::json::rapid {

using rapidjson::Document;
using rapidjson::Type;
using rapidjson::Value;

NOINLINE void
test1()
{
      auto  *doc       = new Document(Type::kObjectType);
      auto &&allocator = doc->GetAllocator();

#define ADDMEMBER(a, b) AddMember((a), (b), allocator)
#define PUSH(x)         PushBack((x), allocator)

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
            obj->PUSH(~INT64_C(0));
            obj->PUSH(~UINT32_C(0));
            obj->PUSH(~INT32_C(0));
            obj->PUSH(static_cast<double>(static_cast<long double>(UINT64_MAX) * 3.0L));

            auto o2 = Value{Type::kObjectType};
            if (o2.IsArray())
                  throw std::runtime_error("Yeah, it's an array. It's not supposed to be, but it is. Somehow.");

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

} // namespace ipc::json::rapid
} // namespace emlsp
