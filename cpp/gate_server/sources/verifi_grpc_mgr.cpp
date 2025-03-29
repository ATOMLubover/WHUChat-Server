#include "include/verifi_grpc_mgr.hpp"

#include "include/aliases.h"
#include "include/config_mgr.hpp"

GetVerifiResponse VerifiGrpcMgr::GetVerificationCode( const std::string& email )
{
    ClientContext context;

    GetVerifiResponse reply;
    GetVerifiRequest request;

    request.set_email( email );

    auto stub = conn_pool.TakeConn();
    Status status = stub->GetVerifyCode( &context, request, &reply );

    if ( !status.ok() )
    {
        reply.set_error( static_cast< int >( EnumErrorCode::ErrorGrpc ) );
    }

    conn_pool.ReturnConn( std::move( stub ) );
    return reply;
}

VerifiGrpcMgr::VerifiGrpcMgr()
    : conn_pool( 3, "127.0.0.1",
        ConfigMgr::GetInstance()[ "vrf_server" ][ "port" ] )
{ }
