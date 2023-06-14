#pragma once
#ifndef HGUARD__IPC__RPC__LSP___STATIC_DATA_HH_
#define HGUARD__IPC__RPC__LSP___STATIC_DATA_HH_ //NOLINT
/****************************************************************************************/

#include "Common.hh"
#include <string>

inline namespace MAIN_PACKAGE_NAMESPACE {
namespace ipc::lsp::data {

extern std::string init_msg(char const *root);

} // namespace ipc::lsp::data
} // namespace MAIN_PACKAGE_NAMESPACE

/****************************************************************************************/
#endif
