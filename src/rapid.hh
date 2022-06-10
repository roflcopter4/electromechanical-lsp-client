#pragma once
#ifndef HGUARD__RAPID_HH_
#define HGUARD__RAPID_HH_ //NOLINT
/****************************************************************************************/

#include "Common.hh"

#ifdef GetObject
# undef GetObject
#endif

#include <rapidjson/rapidjson.h>

#include <rapidjson/document.h>
#include <rapidjson/memorybuffer.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stream.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <rapidjson/allocators.h>
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

inline namespace emlsp {
namespace ipc::json {
/****************************************************************************************/


using StringRef = rapidjson::GenericStringRef<rapidjson::UTF8<char>::Ch>;

template<typename T>
concept NonStringRef =
    !std::convertible_to<T, rapidjson::GenericStringRef<rapidjson::UTF8<char>::Ch>>;

/*
 * Dumbass wrapper.
 */
template <typename Allocator = rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>>
class rapid_doc
{
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

      rapid_doc(rapid_doc const &) = delete;
      rapid_doc &operator=(rapid_doc const &) = delete;
      rapid_doc(rapid_doc &&) noexcept        = delete;
      rapid_doc &operator=(rapid_doc &&) noexcept = delete;

      ND rapidjson::Document &doc()   { return doc_; }
      ND rapidjson::Value    *cur()   { return cur_; }
      ND Allocator           &alloc() { return al_; }

      /*--------------------------------------------------------------------------------*/

      template <size_t N1, size_t N2>
      __forceinline void add_member(char const (&key)[N1], char const (&val)[N2])
      {
            cur_->AddMember(key, val, al_);  // Two stringrefs
      }

      __forceinline void add_member(StringRef &&key, rapidjson::Value &val)
      {
            cur_->AddMember((key), val, al_);
      }

      __forceinline void add_member(StringRef &&key, rapidjson::Value &&val)
      {
            cur_->AddMember((key), std::forward<rapidjson::Value>(val), al_);
      }

      template <typename T>
            REQUIRES (util::concepts::Integral<T>)
      __forceinline void add_member(StringRef &&key, char const *str, T const len)
      {
            cur_->AddMember((key), rapidjson::Value(str, static_cast<rapidjson::SizeType>(len)), al_);
      }

      template <typename T>
            REQUIRES (NonStringRef<T> && !util::concepts::StringVariant<T>)
      __forceinline void add_member(StringRef &&key, T &&val)
      {
            cur_->AddMember(key, rapidjson::Value(std::forward<T>(val)), al_);
      }

      template <typename T>
            REQUIRES (NonStringRef<T> && !util::concepts::StringVariant<T>)
      __forceinline void add_member(StringRef &&key, T &val)
      {
            cur_->AddMember(key, rapidjson::Value(val), al_);
      }

      template <typename T = std::string_view>
            REQUIRES (std::same_as<T, std::string_view>)
      __forceinline void add_member(StringRef &&key, T const &val)
      {
            cur_->AddMember(key, rapidjson::Value(
                  val.data(), static_cast<rapidjson::SizeType>(val.size())), al_);
      }

      template <typename T = std::string>
            REQUIRES (std::same_as<T, std::string>)
      __forceinline void add_member(StringRef &&key, T const &val)
      {
            cur_->AddMember(key, val, al_);
      }

      template <typename T>
            REQUIRES(
                  NonStringRef<T> &&
                  std::same_as<T, char *> &&
                  !util::concepts::Const<T> &&
                  !util::concepts::Reference<T>
            )
      __forceinline void add_member(StringRef &&key, T &&val)
      {
            cur_->AddMember(key, rapidjson::Value(val, ::strlen(val)), al_);
      }

      template <typename T>
            REQUIRES (NonStringRef<T>)
      __forceinline void add_member(T &key, StringRef &&val)
      {
            cur_->AddMember(key, rapidjson::Value(val), al_);
      }

      template <typename T>
            REQUIRES (NonStringRef<T>)
      __forceinline void add_member(T &&key, StringRef &&val)
      {
            cur_->AddMember(key, (val), al_);
      }

#if 1
      template <NonStringRef ...Types>
      __forceinline void add_member(Types &&...args)
      {
            cur_->AddMember(std::forward<Types &&>(args)..., al_);
      }
#endif

#if 0
      template <NonStringRef T1, NonStringRef T2>
      void add_member(T1 arg1, T2 arg2)
      {
            cur_->AddMember(std::forward<T1 &>(arg1), std::forward<T2 &>(arg2), al_);
      }
#endif

      /*--------------------------------------------------------------------------------*/

      __forceinline void add_value(StringRef &&val)
      {
            cur_->PushBack((val), al_);
      }

      template <typename T>
            REQUIRES(NonStringRef<T>)
      __forceinline void add_value(T &val)
      {
            cur_->PushBack(val, al_);
      }

      template <typename T>
            REQUIRES (NonStringRef<T>)
      __forceinline void add_value(T &&val)
      {
            cur_->PushBack(val, al_);
      }

      /*--------------------------------------------------------------------------------*/

      __forceinline void set_member(StringRef &&key, rapidjson::Type const ty = rapidjson::Type::kObjectType)
      {
            // cur_->AddMember((key), rapidjson::Value(ty), al_);
            // cur_ = &cur_->FindMember(key)->value;
            cur_ = &cur_->AddMember((key), rapidjson::Value(ty), al_);
      }

      template <typename T>
            REQUIRES (NonStringRef<T>)
      __forceinline void set_member(T &&key, rapidjson::Type const ty = rapidjson::Type::kObjectType)
      {
            // cur_->AddMember(key, rapidjson::Value(ty), al_);
            // cur_ = &cur_->FindMember(key)->value;
            cur_ = &cur_->AddMember(key, rapidjson::Value(ty), al_);
      }

      __forceinline void set_value(rapidjson::Type const ty)
      {
            cur_->PushBack(rapidjson::Value(ty), al_);
            cur_ = &(*cur_)[cur_->Size() - 1ULL];
      }

      /*--------------------------------------------------------------------------------*/

      void push_member(StringRef &&key, rapidjson::Type const ty = rapidjson::Type::kObjectType)
      {
            cur_->AddMember(key , rapidjson::Value(ty), al_);
            stack_.push(cur_);
            cur_ = &cur_->FindMember(key)->value;
      }

      template <typename T>
            REQUIRES (NonStringRef<T>)
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
            cur_ = &cur_->GetArray()[cur_->Size() - 1ULL];
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

      void clear()
      {
            while (!stack_.empty())
                  stack_.pop();
            cur_ = &doc_;
      }
};

/****************************************************************************************/
} // namespace ipc::json
} // namespace emlsp
#endif
