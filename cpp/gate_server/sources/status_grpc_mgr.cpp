#include "status_grpc_mgr.hpp"

#include "aliases.h"
#include "config_mgr.hpp"
#include "defer.hpp"

#include <chrono>
#include <exception>

GetChatServerResponse StatusGrpcMgr::GetChatServer( std::int32_t uuid )
{
    ClientContext context;

    GetChatServerResponse reply;
    GetChatServerRequest request;

    request.set_uuid( uuid );

    auto stub = conn_pool.TakeConn();
    Defer defer( [ this, &stub ] () { this->conn_pool.ReturnConn( std::move( stub ) ); } );

    Status status;
    try
    {
        status = stub->GetChatServer( &context, request, &reply );
    }
    catch ( std::out_of_range& exp )
    {
        std::cout << "StatusServer异常" << std::endl;
        reply.set_error( static_cast< int >( EnumErrorCode::ErrorServerNotResponding ) );
        return reply;
    }

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
