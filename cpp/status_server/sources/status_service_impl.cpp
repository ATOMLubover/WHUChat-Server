#include "status_service_impl.hpp"

#include "aliases.h"
#include "config_mgr.hpp"
#include "redis_mgr.hpp"

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <fmt/format.h>

#include <iostream>

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
    // 查找之前是否缓存了对应的 token
    std::string token
        = RedisMgr::GetInstance()->QueryChatServerToken( request->uuid() );
    // 如果存在则跳出
    if ( token != "" )
    {
        reply->set_token( token );
        reply->set_error( ( std::int32_t ) EnumErrorCode::Success );

        return Status::OK;
    }

    // 以轮询的方式负载均衡
    static int server_idx = 0;

    // 挑选一个可以使用的服务器信息（暂时使用普通轮询）
    ChatServerInfo server;
    {
        std::lock_guard<std::mutex> guard( mtx_servers );

        server_idx = ( server_idx++ ) % ( list_chatsrv_info.size() );
        server = list_chatsrv_info[ server_idx ];
    }
    CacheChatServer( request->uuid(), server.host, server.port );

    // 设置 reply 的内容，gRPC 底层会自动在设置完成后发送
    try
    {
        reply->set_error( ( std::int32_t ) EnumErrorCode::Success );

        if ( token == "" )
        {
            token = ChatServerInfo::GenUuidHelper();
            CacheToken( request->uuid(), token );
        }
        reply->set_token( token );
    }
    catch ( std::exception& exp )
    {
        std::cout << "StatusServiveImpl获取ChatServer处异常：" << exp.what() << std::endl;
        return Status::CANCELLED;
    }

    return Status::OK;
}

void StatusServiceImpl::CacheToken( int uuid, const std::string& token )
{
    RedisMgr::GetInstance()->SetChatServerToken( uuid, token );
    std::cout << fmt::format( "uuid： {}，插入对应token：{}",
        uuid, token ) << std::endl;
}

void StatusServiceImpl::CacheChatServer( int uuid, const std::string& host, const std::string& port )
{
    RedisMgr::GetInstance()->SetUserChatServer( uuid, host, port );
    std::cout << fmt::format( "uuid： {}，插入对应chat server：{}",
        uuid, host + ":" + port ) << std::endl;
}
