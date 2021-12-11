#ifndef HGUARD__IPC__BASE_MESSAGE_HH_
#define HGUARD__IPC__BASE_MESSAGE_HH_ //NOLINT
#pragma once

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

    private:
      MsgType msg_{};

    public:
      base_message() = default;
};

/****************************************************************************************/
} // namespace ipc
} // namespace emlsp
#endif // base_message.hh
