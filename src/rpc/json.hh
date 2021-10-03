#pragma once
#ifndef HGUARD_d_RPC_d_JSON_HH_
#define HGUARD_d_RPC_d_JSON_HH_
/****************************************************************************************/

#include "Common.hh"
#include "rapid.hh"

#include <stack>

class object
{
      using StringRef = rapidjson::GenericStringRef<rapidjson::UTF8<char>::Ch>;

      rapidjson::Document doc_;
      rapidjson::Value   *cur_;
      std::stack<rapidjson::Value *> stack_{};

    public:
      explicit object(rapidjson::Type const ty = rapidjson::Type::kObjectType)
          : doc_(ty), cur_(&doc_)
      {}
      ~object() = default;

      template <typename T>
      rapidjson::Value &add_value(T &&value)
      {
            return cur_->PushBack(std::move(value), doc_.GetAllocator());
      }

      rapidjson::Value *push_value(rapidjson::Type const ty = rapidjson::Type::kObjectType)
      {
            cur_->PushBack(rapidjson::Value(ty), doc_.GetAllocator());
            stack_.push(cur_);
            return cur_ = cur_->End();
      }

      template <typename T>
      rapidjson::Value &add_member(StringRef name, T &&value)
      {
            return cur_->AddMember(std::move(name), std::move(value), doc_.GetAllocator());
      }

      rapidjson::Value *push_member(StringRef &&          name,
                                    rapidjson::Type const ty = rapidjson::Type::kObjectType)
      {
            cur_->AddMember(name, rapidjson::Value(ty), doc_.GetAllocator());
            stack_.push(cur_);
            return cur_ = &cur_->FindMember(std::forward<StringRef &>(name))->value;
      }

      void pop(int const n = 1)
      {
            for (int i = 0; i < (n - 1); ++i)
                  stack_.pop();
            assert(!stack_.empty());
            cur_ = stack_.top();
            stack_.pop();
      }

      ND rapidjson::Document &doc()              { return doc_; }
      ND rapidjson::Value    *get()        const { return cur_; }
      ND rapidjson::Value    *operator()() const { return cur_; }

      DELETE_COPY_CTORS(object);
};

/****************************************************************************************/
#endif // rpc/json.hh
