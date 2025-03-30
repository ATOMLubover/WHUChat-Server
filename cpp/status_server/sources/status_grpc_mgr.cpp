#include "status_grpc_mgr.hpp"

#include "aliases.h"
#include "config_mgr.hpp"
#include "defer.hpp"

GetChatServerResponse StatusGrpcMgr::GetChatServer( std::int32_t uuid )
{
    ClientContext context;

    GetChatServerResponse reply;
    GetChatServerRequest request;

    request.set_uuid( uuid );

    auto stub = conn_pool.TakeConn();
    Defer defer( [ this, &stub ] () { this->conn_pool.ReturnConn( std::move( stub ) ); } );

    Status status = stub->GetChatServer( &context, request, &reply );

    if ( !status.ok() )
    {
        reply.set_error( static_cast< int >( EnumErrorCode::ErrorGrpc ) );
    }

    return reply;
}

StatusGrpcMgr::StatusGrpcMgr()
    : conn_pool(
        3,
        ConfigMgr::GetInstance()[ "status_server" ][ "host" ], // StatusServer 可以不用和 GateServer 部署在一台主机上
        ConfigMgr::GetInstance()[ "status_server" ][ "port" ] )
{ }
