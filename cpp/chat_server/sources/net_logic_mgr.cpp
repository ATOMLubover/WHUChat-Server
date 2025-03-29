#include "net_logic_system.hpp"

#include "http_conn.hpp"
#include "websock_conn.hpp"
#include "asio_iocontext_pool.hpp"

#include <fmt/core.h>
#include <json/json.hpp>
#include <boost/beast/websocket.hpp>

#include <iostream>
#include <filesystem>

namespace fs = std::filesystem; // from <filesystem>

#ifdef DEBUG
const std::string NetLogicSystem::FRONTEND_STATIC_DIR = "../resources/static/frontend";
#else
const std::string NetLogicSystem::FRONTEND_STATIC_DIR = "./resources/static/frontend";
#endif // DEBUG


NetLogicSystem::~NetLogicSystem()
{
    std::cout << "NetLogicSystem被析构" << std::endl;
}

void NetLogicSystem::RegisterGet( const std::string& url, HttpHandler handler )
{
    get_handlers.emplace( url, handler );

    std::cout << "注册GET URL: " << url << std::endl;
}

bool NetLogicSystem::HandleGet( std::shared_ptr<HttpConn> conn )
{
    auto iter_handler = get_handlers.find( conn->get_url );

    // 未找到则返回 false
    if ( iter_handler == get_handlers.end() )
    {
        std::cout << "NetLogicSystem无法处理GET URL：" << conn->get_url << std::endl;
        return false;
    }

    std::cout << "NetLogicSystem处理GET请求，其URL：" << conn->get_url << std::endl;
    iter_handler->second( conn );
    return true;
}

void NetLogicSystem::RegisterPost( const std::string& url, HttpHandler handler )
{
    post_handlers.emplace( url, handler );

    std::cout << "注册POST URL：" << url << std::endl;
}

bool NetLogicSystem::HandlePost( std::shared_ptr<HttpConn> conn )
{
    auto iter_handler = post_handlers.find( conn->post_url );

    // 未找到则返回 false
    if ( iter_handler == post_handlers.end() )
    {
        std::cout << "NetLogicSystem无法处理POST URL：" << conn->post_url << std::endl;
        return false;
    }

    std::cout << "NetLogicSystem处理POST请求，其URL：" << conn->post_url << std::endl;
    iter_handler->second( conn );
    return true;
}

bool NetLogicSystem::IsWebsockUpgrade( std::shared_ptr<HttpConn> conn )
{
    std::cout << "收到WebSocket升级请求：" << conn->request.target() << std::endl;
    return websocket::is_upgrade( conn->request );
}

void NetLogicSystem::HandleUpgrade( std::shared_ptr<HttpConn> conn )
{
    // 自动握手会发送一条确认升级的响应
    // 之后客户端将与服务器形成全双工通信
    std::make_shared<WebsockConn>( conn->GetSocket() )->SyncAccept( conn->request );

    std::cout << "NetLogicSystem HandleUpgrade函数调用完成" << std::endl;
}

NetLogicSystem::NetLogicSystem()
{
    // TODO: for test
    {

    }

    std::cout << "NetLogicSystem构造" << std::endl;
}
