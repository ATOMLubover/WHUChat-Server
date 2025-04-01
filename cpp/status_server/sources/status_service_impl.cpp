#include "status_service_impl.hpp"

#include "aliases.h"
#include "config_mgr.hpp"

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

std::string ChatServerInfo::GenUuidHelper()
{
    boost::uuids::uuid uuid = boost::uuids::random_generator()( );
    return boost::uuids::to_string( uuid );
}

StatusServiceImpl::StatusServiceImpl()
{
    int chatsrv_num = std::stoi(
        ConfigMgr::GetInstance()[ "chat_server_info" ][ "num" ] );
    // 注意 ChatServer 的编号是从 0 开始编号的
    for ( int i = 0; i < chatsrv_num; i++ )
    {
        const std::string prefix = "chat_server_";
        std::string serial = std::to_string( i );
        ChatServerInfo info{
                ConfigMgr::GetInstance()[ prefix + serial ][ "host" ],
                ConfigMgr::GetInstance()[ prefix + serial ][ "port" ] };
        list_chatsrv_info.push_back( info );

        std::cout << "ChatServer第" << i << "号信息载入："
            << info.host << ":" << info.port << std::endl;
    }
}

Status StatusServiceImpl::GetChatServer( ServerContext* context, const GetChatServerRequest* request, GetChatServerResponse* reply )
{
    // 以轮询的方式负载均衡
    static int server_idx = 0;
    server_idx = ( server_idx++ ) % ( list_chatsrv_info.size() );
    auto& server = list_chatsrv_info[ server_idx ];

    // 设置 reply 的内容，gRPC 底层会自动在设置完成后发送
    reply->set_host( server.host );
    reply->set_port( server.port );
    reply->set_error( ( int32_t ) EnumErrorCode::Success );
    reply->set_token( ChatServerInfo::GenUuidHelper() );

    return Status::OK;
}
