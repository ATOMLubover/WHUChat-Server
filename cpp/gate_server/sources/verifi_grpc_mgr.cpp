#include "verifi_grpc_mgr.hpp"

#include "aliases.h"
#include "config_mgr.hpp"
#include "defer.hpp"

GetVerifiResponse VerifiGrpcMgr::GetVerifiCode( const std::string& email )
{
    ClientContext context;

    GetVerifiResponse reply;
    GetVerifiRequest request;

    request.set_email( email );

    auto stub = conn_pool.TakeConn();
    Defer defer( [ this, &stub ] () { this->conn_pool.ReturnConn( std::move( stub ) ); } );

    Status status = stub->GetVerifyCode( &context, request, &reply );

    if ( !status.ok() )
    {
        reply.set_error( static_cast< int >( EnumErrorCode::ErrorGrpc ) );
    }

    return reply;
}

VerifiGrpcMgr::VerifiGrpcMgr()
    : conn_pool(
        3,
        ConfigMgr::GetInstance()[ "verifi_server" ][ "host" ],
        ConfigMgr::GetInstance()[ "verifi_server" ][ "port" ] )
{ }
