#pragma once
#ifndef HGUARD_d_RAPID_HH_
#define HGUARD_d_RAPID_HH_
/****************************************************************************************/

#include "Common.hh"

#ifdef GetObject
# undef GetObject
#endif

#define P00_PASTE_2_HELPER_2(a, b) a ## b
#define P00_PASTE_2_HELPER_1(a, b) P00_PASTE_2_HELPER_2(a, b)
#define PP_PASTE_2(a, b) P00_PASTE_2_HELPER_1(a, b)

#define P00_STRINGIFY_HELPER(x) #x
#define PP_STRINGIFY(x) P00_STRINGIFY_HELPER(x)

#ifdef NDEBUG
#  define RAPIDJSON_ASSERT(x)
#  define RAPIDJSON_NOEXCEPT_ASSERT(x)
#else
#  define RAPIDJSON_ASSERT_THROWS
#  define RAPIDJSON_NOEXCEPT_ASSERT(x) assert(x)
#  define RAPIDJSON_ASSERT(x)                                \
      do {                                                   \
            if (!(x)) [[unlikely]] {                         \
                  err(1, "rapidjson assertion failed: " #x); \
            }                                                \
      } while (0)
#endif

#define RAPIDJSON_SSE2 1
#define RAPIDJSON_SSE42 1
#define RAPIDJSON_USE_MEMBERSMAP 1
#define RAPIDJSON_HAS_CXX11_TYPETRAITS 1
#define RAPIDJSON_HAS_CXX11_RANGE_FOR  1
#define RAPIDJSON_HAS_STDSTRING 1

#include <rapidjson/rapidjson.h>

#include <rapidjson/document.h>
#include <rapidjson/memorybuffer.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stream.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <rapidjson/allocators.h>
#include <rapidjson/cursorstreamwrapper.h>
#include <rapidjson/encodedstream.h>
#include <rapidjson/encodings.h>
#include <rapidjson/filereadstream.h>
#include <rapidjson/filewritestream.h>
#include <rapidjson/fwd.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/pointer.h>
#include <rapidjson/reader.h>
#include <rapidjson/schema.h>
#include <rapidjson/writer.h>

/****************************************************************************************/

namespace emlsp::rpc::json {

using StringRef = rapidjson::GenericStringRef<rapidjson::UTF8<char>::Ch>;
template<typename T>
concept NonStringRef = !std::convertible_to<
      T, rapidjson::GenericStringRef<rapidjson::UTF8<char>::Ch>>;

template <typename Allocator = rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>>
class rapid_doc
{
    private:
      rapidjson::Document doc_;
      rapidjson::Value   *cur_;
      Allocator          &al_;
      std::stack<rapidjson::Value *> stack_ = {};

    public:
      rapid_doc() : doc_(rapidjson::Type::kObjectType),
                    cur_(&doc_),
                    al_(doc_.GetAllocator())
      {}

      ~rapid_doc() = default;

      DELETE_COPY_CTORS(rapid_doc);
      DELETE_MOVE_CTORS(rapid_doc);

      rapidjson::Document &doc()   { return doc_; }
      rapidjson::Value    *cur()   { return cur_; }
      Allocator           &alloc() { return al_; }

      /*--------------------------------------------------------------------------------*/

      void add_member(StringRef &&key, StringRef &&val)
      {
            cur_->AddMember(key, val, al_);
      }

      void add_member(StringRef &&key, rapidjson::Value &val)
      {
            cur_->AddMember(key, std::move(val), al_);
      }

      void add_member(StringRef &&key, rapidjson::Value &&val)
      {
            cur_->AddMember(key, val, al_);
      }

      template <NonStringRef T>
      void add_member(StringRef &&key, T &val)
      {
            cur_->AddMember(key, rapidjson::Value(std::move(val)), al_);
      }

      template <NonStringRef T>
      void add_member(StringRef &&key, T &&val)
      {
            cur_->AddMember(key, rapidjson::Value(val), al_);
      }

      template <NonStringRef T>
      void add_member(T &key, StringRef &&val)
      {
            cur_->AddMember(std::move(key), val, al_);
      }

      template <NonStringRef T>
      void add_member(T &&key, StringRef &&val)
      {
            cur_->AddMember(key, rapidjson::Value(val), al_);
      }

      template <NonStringRef ...Types>
      void add_member(Types &&...args)
      {
            cur_->AddMember(args..., al_);
      }

      /*--------------------------------------------------------------------------------*/

      void add_value(StringRef &&val)
      {
            cur_->PushBack(val, al_);
      }

      template <NonStringRef T>
      void add_value(T &val)
      {
            cur_->PushBack(val, al_);
      }

      template <NonStringRef T>
      void add_value(T &&val)
      {
            cur_->PushBack(val, al_);
      }

      /*--------------------------------------------------------------------------------*/

      void push_member(StringRef &&key, rapidjson::Type const ty = rapidjson::Type::kObjectType)
      {
            cur_->AddMember(key, rapidjson::Value(ty), al_);
            stack_.push(cur_);
            cur_ = &cur_->FindMember(key)->value;
      }

      template <NonStringRef T>
      void push_member(T &&key, rapidjson::Type const ty = rapidjson::Type::kObjectType)
      {
            cur_->AddMember(key, rapidjson::Value(ty), al_);
            stack_.push(cur_);
            cur_ = &cur_->FindMember(key)->value;
      }

      void push_value(rapidjson::Type const ty)
      {
            cur_->PushBack(rapidjson::Value(ty), al_);
            stack_.push(cur_);
            cur_ = &(*cur_)[cur_->Size() - SIZE_C(1)];
      }

      void pop(size_t n = 1)
      {
            assert(n > 0);
            while (n > 1) {
                  --n;
                  stack_.pop();
            }

            assert(!stack_.empty());
            cur_ = stack_.top();
            stack_.pop();
      }
};

} // namespace emlsp::rpc::json

/****************************************************************************************/
#endif
