#ifndef HGUARD__IPC__LSP___MESSAGE_HH_
#define HGUARD__IPC__LSP___MESSAGE_HH_ //NOLINT
#pragma once

#include "Common.hh"
#include "rapid.hh"

/****************************************************************************************/

inline namespace emlsp {
namespace ipc::lsp {

class base_message
{
    public:
      using message_type = rapidjson::Document;

    private:
      rapidjson::Document doc_{};

    public:
      base_message() = default;
};

} // namespace ipc::lsp 
} // namespace emlsp

/****************************************************************************************/
#endif // lsp-message.hh
