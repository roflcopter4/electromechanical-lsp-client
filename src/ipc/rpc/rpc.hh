#pragma once
#ifndef HGUARD__Pq35Cbif0LfjXglI
#define HGUARD__Pq35Cbif0LfjXglI //NOLINT
/****************************************************************************************/
#include "Common.hh"
#include "ipc/basic_protocol_connection.hh"
#include "ipc/toploop.hh"
#include <ipc/protocols/msgpack_connection.hh>

inline namespace MAIN_PACKAGE_NAMESPACE {
namespace ipc::rpc {
/****************************************************************************************/


class basic_rpc_interface
{
      using this_type = basic_rpc_interface;

    public:
      basic_rpc_interface() = default;
};


template <typename ConnectionType, template <class> typename ProtocolType>
    requires BasicConnectionVariant<ConnectionType> &&
             ProtocolConnectionVariant<ProtocolType<ConnectionType>>
class alignas(4096) basic_rpc_connection final
      : public ProtocolType<ConnectionType>
{
      using this_type = basic_rpc_connection<ConnectionType, ProtocolType>;

    public:
      using connection_type = ConnectionType;
      using protocol_type   = ProtocolType<ConnectionType>;

    protected:
      basic_rpc_connection() = default;

    public:
      ~basic_rpc_connection() override = default;

      DELETE_ALL_CTORS(basic_rpc_connection);

      static auto new_unique() { return std::unique_ptr<this_type>(new this_type); }
      static auto new_shared() { return std::shared_ptr<this_type>(new this_type); }

      /*------------------------------------------------------------------------------*/
};


/****************************************************************************************/
} // namespace ipc::rpc
} // namespace MAIN_PACKAGE_NAMESPACE
#endif
