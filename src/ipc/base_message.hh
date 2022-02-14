#pragma once
#ifndef HGUARD__IPC__BASE_MESSAGE_HH_
#define HGUARD__IPC__BASE_MESSAGE_HH_ //NOLINT

#include "Common.hh"
#include "rapid.hh"

inline namespace emlsp {
namespace ipc {
/****************************************************************************************/

template <typename MsgType>
class base_message
{
    public:
      using message_type = MsgType;
      using base_type    = base_message<MsgType>;

    protected:
      MsgType msg_;

    public:
      base_message() = default;
      virtual ~base_message() = default;

      ND MsgType const &msg() const { return msg_; }
      ND MsgType       &msg()       { return msg_; }

      virtual int request() = 0;
};

/****************************************************************************************/
} // namespace ipc
} // namespace emlsp
#endif // base_message.hh
