#include "http_logic_system.hpp"

#include "http_conn.hpp"
#include "websock_conn.hpp"
#include "asio_iocontext_pool.hpp"
#include "websock_mgr.hpp"

#include <fmt/core.h>
#include <json/json.hpp>
#include <boost/beast/websocket.hpp>

#include <iostream>
#include <filesystem>

namespace fs = std::filesystem; // from <filesystem>

#ifdef DEBUG
const std::string HttpLogicSystem::FRONTEND_STATIC_DIR = "../resources/static/frontend";
#else
const std::string HttpLogicSystem::FRONTEND_STATIC_DIR = "./resources/static/frontend";
#endif // DEBUG


HttpLogicSystem::~HttpLogicSystem()
{
    std::cout << "HttpLogicSystem被析构" << std::endl;
}

void HttpLogicSystem::RegisterGet( const std::string& url, HttpHandler handler )
{
    get_handlers.emplace( url, handler );

    std::cout << "注册GET URL: " << url << std::endl;
}

bool HttpLogicSystem::HandleGet( std::shared_ptr<HttpConn> conn )
{
    auto iter_handler = get_handlers.find( conn->get_url );

    // 未找到则返回 false
    if ( iter_handler == get_handlers.end() )
    {
        std::cout << "HttpLogicSystem无法处理GET URL：" << conn->get_url << std::endl;
        return false;
    }

    std::cout << "HttpLogicSystem处理GET请求，其URL：" << conn->get_url << std::endl;
    iter_handler->second( conn );
    return true;
}

void HttpLogicSystem::RegisterPost( const std::string& url, HttpHandler handler )
{
    post_handlers.emplace( url, handler );

    std::cout << "注册POST URL：" << url << std::endl;
}

bool HttpLogicSystem::HandlePost( std::shared_ptr<HttpConn> conn )
{
    auto iter_handler = post_handlers.find( conn->post_url );

    // 未找到则返回 false
    if ( iter_handler == post_handlers.end() )
    {
        std::cout << "HttpLogicSystem无法处理POST URL：" << conn->post_url << std::endl;
        return false;
    }

    std::cout << "HttpLogicSystem处理POST请求，其URL：" << conn->post_url << std::endl;
    iter_handler->second( conn );
    return true;
}

bool HttpLogicSystem::HandleUpgrade( std::shared_ptr<HttpConn> conn )
{
    // 自动握手会发送一条确认升级的响应
    // 之后客户端将与服务器形成全双工通信
    // 只有匹配要求的升级请求才创建 WebSocket
    if ( WebsockMgr::GetInstance()->CheckValidUpgrade( conn ) )
    {
        WebsockMgr::GetInstance()->CreateConn( conn );

        std::cout << "HttpLogicSystem HandleUpgrade函数已成功升级连接" << std::endl;
        return true;
    }

    std::cout << "HttpLogicSystem HandleUpgrade函数拒绝升级" << std::endl;
    return false;
}

HttpLogicSystem::HttpLogicSystem()
{
    // TODO: for test
    {

    }

    // POST 部分
    {
        // // 客户端发送对话
        // RegisterPost(
        //     "/api/v1/chat/send",
        //     [] ( std::shared_ptr<HttpConn> conn )
        //     {
        //         std::string str_req = beast::buffers_to_string( conn->request.buf_recv );
        //         std::cout << "/api/v1/chat/send收到数据：" << str_req << std::endl;


        //     } );
    }

    std::cout << "HttpLogicSystem构造" << std::endl;
}
