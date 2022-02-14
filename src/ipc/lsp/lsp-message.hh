#pragma once
#ifndef HGUARD__IPC__LSP___MESSAGE_HH_
#define HGUARD__IPC__LSP___MESSAGE_HH_ //NOLINT

#include "Common.hh"
#include "rapid.hh"

#include "ipc/base_message.hh"

inline namespace emlsp {
namespace ipc::lsp {
/****************************************************************************************/


class Message //: public ipc::base_message<rapidjson::Document>
{
    public:
      friend class Response;
      friend class Notification;
      friend class Request;

      enum class Type {
            INVALID      = 0,
            REQUEST      = 1,
            NOTIFICATION = 2,
            RESPONSE     = 3,
      };

    private:
      rapidjson::Document doc_{rapidjson::Type::kObjectType};

      constexpr static std::string_view const type_repr_[] = {
          "Invalid/Uninitialized"sv,
          "Request"sv,
          "Notification"sv,
          "Response"sv};

      Type type_{Type::INVALID};

    protected:
      explicit Message(Type const t) : type_(t) {}

    public:
      Message();

      ND auto const &msg() const { return doc_; }
      ND auto       &msg() { return doc_; }

      ND constexpr auto type() const { return type_; }

      ND constexpr auto const &type_repr() const
      {
            return type_repr_[static_cast<int>(type_)];
      }
};


class Request : public Message
{
    public:
      Request() : Message(Type::REQUEST) {}
};


class Notification : public Message
{
    public:
      Notification() : Message(Type::NOTIFICATION) {}
};


class Response : public Message
{
    public:
      Response() : Message(Type::RESPONSE) {}
};


/****************************************************************************************/
} // namespace ipc::lsp
} // namespace emlsp
#endif // lsp-message.hh
