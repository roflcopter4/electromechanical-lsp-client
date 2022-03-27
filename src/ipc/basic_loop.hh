#pragma once
#ifndef HGUARD__IPC__BASIC_LOOP_HH_
#define HGUARD__IPC__BASIC_LOOP_HH_ //NOLINT
/****************************************************************************************/
#include "Common.hh"

#include "ipc/basic_connection.hh"
#include "ipc/basic_dialer.hh"
#include "ipc/basic_io_connection.hh"
#include "ipc/basic_io_wrapper.hh"

inline namespace emlsp {
namespace ipc {
/****************************************************************************************/


class basic_loop
{
      using this_type = basic_loop;

    public:
      basic_loop() = default;
      ~basic_loop() = default;

      basic_loop(basic_loop const &)                = delete;
      basic_loop(basic_loop &&) noexcept            = delete;
      basic_loop &operator=(basic_loop const &)     = delete;
      basic_loop &operator=(basic_loop &&) noexcept = delete;

      //-------------------------------------------------------------------------------


};


/****************************************************************************************/
} // namespace ipc
} // namespace emlsp
#endif
