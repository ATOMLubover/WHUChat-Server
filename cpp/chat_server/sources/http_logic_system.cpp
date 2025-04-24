#include "http_logic_system.hpp"

#include "http_conn.hpp"
#include "websock_conn.hpp"
#include "asio_iocontext_pool.hpp"
#include "websock_mgr.hpp"
#include "mysql_mgr.hpp"
#include "date_processer.hpp"
#include "redis_mgr.hpp"
#include "defer.hpp"
#include "asio_iocontext_pool.hpp"
#include "config_mgr.hpp"
#include "cookie_processer.hpp"
#include "cli_http_mgr.hpp"
#include "sync_logger.hpp"
#include "svr_https_conn.hpp"

#include <fmt/core.h>
#include <json/json.hpp>
#include <boost/beast/websocket.hpp>

#include <iostream>
#include <filesystem>
#include <functional>

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

void HttpLogicSystem::TransMsgContent( std::shared_ptr<HttpConn> conn )
{
    nlohmann::json json_rsp;
    nlohmann::json json_req = nlohmann::json::parse( conn->request.body() );

    if ( !json_req.is_object() )
    {
        std::cout << "/api/v1/send_message接受到无法解析的json" << std::endl;
        json_rsp.emplace( "error", EnumErrorCode::ErrorJson );
        conn->WriteRspBody( json_rsp.dump() );
        return;
    }

    // 确定是否存在 ssn_id
    auto ssn_id = json_req.find( "session_id" );
    if ( ssn_id == json_req.end() )
    {
        json_rsp.emplace( "error", EnumErrorCode::ErrorJson );
        conn->WriteRspBody( json_rsp.dump() );
        return;
    }

    int uuid = json_req[ "uuid" ].get<int>();
    std::string sender = json_req[ "prompt" ][ "role" ].get<std::string>();
    std::string content = json_req[ "prompt" ][ "content" ].get<std::string>();

    // 如果是新 session，则创建一个新的 session
    if ( ssn_id->is_null() )
    {
        int new_ssn_id = MySqlMgr::GetInstance()->CreateSession( uuid );
        // 如果返回小于等于 0，则创建失败
        if ( new_ssn_id <= 0 )
        {
            json_rsp.emplace( "error", EnumErrorCode::ErrorMySql );
            return;
        }

        // 否则创建成功，更新 request 和程序中的 session_id
        json_req[ "session_id" ] = new_ssn_id;
        *ssn_id = new_ssn_id;
    }
    // 如果 session 存在，则检查是否存在该 session
    else if ( ssn_id->is_number_integer() )
    {
        if ( !MySqlMgr::GetInstance()->CheckSessionExisting( *ssn_id ) )
        {
            json_rsp.emplace( "error", EnumErrorCode::ErrorSsnIdInvalid );
            conn->WriteRspBody( json_rsp.dump() );
            return;
        }
    }

    // 记录用户提出的消息
    if ( MySqlMgr::GetInstance()->CreateMessage(
        *ssn_id, uuid, 100,
        content, sender,
        json_req.dump() )
        != 0 )
    {
        SyncLogger::GetInstance()->Log(
            LogLevel::Error,
            "数据库记录用户消息失败" );
        json_rsp.emplace( "error", EnumErrorCode::ErrorMySql );
        conn->WriteRspBody( json_rsp.dump() );
        return;
    }

    // 向 ApiServer 转发 http
    {
        SyncLogger::GetInstance()->Log(
            LogLevel::Debug,
            "TransMsgContent试图转发: {}",
            json_req.dump() );

        // 确定 ApiServer 的地址和端口
        std::string host = ConfigMgr::GetInstance()[ "api_server" ][ "host" ];
        std::string post = ConfigMgr::GetInstance()[ "api_server" ][ "port" ];

        // 构建 http 请求
        http::request<http::string_body> req{
            http::verb::post,  "/get_response", 11 };
        // 设置 Content-Type 头
        req.set( http::field::content_type, "application/json" );
        req.set( http::field::host, host );
        req.body() = json_req.dump(); // 写入 json
        // 最后设置 Content-Length
        req.set( http::field::content_length,
            std::to_string( req.body().size() ) );

        // 设定 rsp handler
        CliRspHandler handler( std::make_shared<
            std::function<void( http::response<http::string_body>&& )>>(
                [ conn ] ( http::response<http::string_body>&& cli_rsp )
                {
                    nlohmann::json json_rsp;

                    SyncLogger::GetInstance()->Log(
                        LogLevel::Debug,
                        "接受到ApiServer回复为：{}",
                        cli_rsp.body() );

                    nlohmann::json json_cli = nlohmann::json::parse( cli_rsp.body() );
                    auto iter = json_cli.find( "error" );
                    if ( iter == json_cli.end() )
                    {
                        json_rsp.emplace( "error", EnumErrorCode::ErrorJson );
                        conn->WriteRspBody( json_rsp.dump() );
                        return;
                    }

                    json_rsp.emplace( "error", json_cli[ "error" ] );
                    conn->WriteRspBody( json_rsp.dump() );

                    // 使原链接返回响应
                    conn->AsyncWriteResponse();
                } ) );
        // 设定 timeout_handler
        CliTimeoutHandler timeout_handler( std::make_shared<
            std::function<void( std::shared_ptr<CliHttpConn> )>>(
                [ conn ] ( std::shared_ptr<CliHttpConn> cli_conn )
                {
                    SyncLogger::GetInstance()->Log(
                        LogLevel::Error,
                        "{} 超时", cli_conn->ToString() );

                    nlohmann::json json_rsp;
                    json_rsp.emplace( "error", EnumErrorCode::ErrorApiNotResponding );
                    conn->WriteRspBody( json_rsp.dump() );

                    // 使原链接返回响应
                    conn->AsyncWriteResponse();
                } ) );

        // 异步地发送 http 请求
        CliHttpMgr::GetInstance()->AsyncRequest(
            host, post,
            std::move( req ),
            handler,
            timeout_handler );

        // 最后保证原 http conn 不要自动发送响应，而是等待 cli conn 处理完成
        conn->SetDelay( true );
    }
}

bool HttpLogicSystem::CheckSessionCookie( std::map<std::string, std::string>& map_cookies )
{
    // 检查是否有 uuid 键值对
    auto iter = map_cookies.find( "uuid" );
    if ( iter == map_cookies.end() )
        return false;
    int uuid = std::stoi( iter->second );

    // 检查 token 是否有效
    // 当过期三天，redis 会自动删除对应的 token
    if ( !RedisMgr::GetInstance()->QueryChatServerToken( uuid, map_cookies[ "token" ] ) )
        return false;

    return true;
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

void HttpLogicSystem::ProcessConn( std::shared_ptr<SvrHttpsConn> conn )
{
    //     conn->SetReadHandler(
    //         std::make_shared<std::function<ResponseVar( ReqType )>>(
    //             [ self = shared_from_this() ] ( ReqType req ) -> ResponseVar
    //             {
    //                 std::string url = req->target();

    //             } ) );
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
    // 获取所有模型列表
    RegisterGet(
        "/api/v1/chat/models",
        [] ( std::shared_ptr<HttpConn> conn )
        {
            nlohmann::json json_rsp;

            auto iter = conn->request.find( "Cookie" );
            if ( iter == conn->request.end() )
            {
                json_rsp.emplace( "error", EnumErrorCode::ErrorChatCookieInvalid );
                conn->WriteRspBody( json_rsp.dump() );
                return;
            }

            auto map_cookies
                = CookieProcesser::Parse( iter->value() );
            if ( !CheckSessionCookie( map_cookies ) )
            {
                json_rsp.emplace( "error", EnumErrorCode::ErrorChatCookieInvalid );
                conn->WriteRspBody( json_rsp.dump() );
                return;
            }

            std::list<ModelInfo> models = MySqlMgr::GetInstance()->SelectModels();
            nlohmann::json json_models;
            for ( auto iter = models.begin()
                ; iter != models.end()
                ; ++iter )
            {
                nlohmann::json json_model;
                json_model.emplace( "id", iter->m_id );
                json_model.emplace( "name", iter->m_name );
                json_model.emplace( "desc", iter->m_desc );

                json_models.emplace_back( json_model );
            }

            json_rsp.emplace( "models", json_models );
            conn->WriteRspBody( json_rsp.dump() );
        } );

    // 发送问题
    RegisterPost(
        "/api/v1/chat/send_message",
        [] ( std::shared_ptr<HttpConn> conn )
        {
            nlohmann::json json_rsp;

            auto iter = conn->request.find( "Cookie" );
            if ( iter == conn->request.end() )
            {
                json_rsp.emplace( "error", EnumErrorCode::ErrorChatCookieInvalid );
                conn->WriteRspBody( json_rsp.dump() );
                return;
            }

            auto map_cookies
                = CookieProcesser::Parse( iter->value() );
            if ( !CheckSessionCookie( map_cookies ) )
            {
                json_rsp.emplace( "error", EnumErrorCode::ErrorChatCookieInvalid );
                conn->WriteRspBody( json_rsp.dump() );
                return;
            }

            if ( nlohmann::json::parse( conn->request.body() )[ "uuid" ].get<int>()
                != std::stoi( map_cookies[ "uuid" ] ) )
            {
                json_rsp.emplace( "error", EnumErrorCode::ErrorChatCookieInvalid );
                conn->WriteRspBody( json_rsp.dump() );
                return;
            }

            // 转发客户端的请求
            TransMsgContent( conn );
        } );

    // 获取历史会话
    RegisterPost(
        "/api/v1/chat/history",
        [] ( std::shared_ptr<HttpConn> conn )
        {
            nlohmann::json json_rsp;

            auto iter = conn->request.find( "Cookie" );
            if ( iter == conn->request.end() )
            {
                json_rsp.emplace( "error", EnumErrorCode::ErrorChatCookieInvalid );
                conn->WriteRspBody( json_rsp.dump() );
                return;
            }

            auto map_cookies
                = CookieProcesser::Parse( iter->value() );
            if ( !CheckSessionCookie( map_cookies ) )
            {
                json_rsp.emplace( "error", EnumErrorCode::ErrorChatCookieInvalid );
                conn->WriteRspBody( json_rsp.dump() );
                return;
            }

            nlohmann::json json_req = nlohmann::json::parse( conn->request.body() );
            int uuid = json_req[ "uuid" ].get<int>();

            if ( uuid != std::stoi( map_cookies[ "uuid" ] ) )
            {
                json_rsp.emplace( "error", EnumErrorCode::ErrorChatCookieInvalid );
                conn->WriteRspBody( json_rsp.dump() );
                return;
            }

            auto sessions
                = std::move( MySqlMgr::GetInstance()->SelectSessions( uuid ) );
            nlohmann::json json_ssns;
            for ( const auto& ssn : sessions )
            {
                nlohmann::json json_ssn;
                json_ssn.emplace( "id", ssn.m_id );
                json_ssn.emplace( "uuid", ssn.m_uuid );
                json_ssn.emplace( "title", ssn.m_title );
                json_ssn.emplace( "updated_at", ssn.m_updated_at );

                json_ssns.emplace_back( json_ssn );
            }

            json_rsp.emplace( "error", EnumErrorCode::Success );
            json_rsp.emplace( "sessions", json_ssns );
            conn->WriteRspBody( json_rsp.dump() );
        } );

    // 获取指定会话中所有的 message
    RegisterPost(
        "/api/v1/chat/browse_messages",
        [] ( std::shared_ptr<HttpConn> conn )
        {
            nlohmann::json json_rsp;

            auto iter = conn->request.find( "Cookie" );
            if ( iter == conn->request.end() )
            {
                json_rsp.emplace( "error", EnumErrorCode::ErrorChatCookieInvalid );
                conn->WriteRspBody( json_rsp.dump() );
                return;
            }

            auto map_cookies
                = CookieProcesser::Parse( iter->value() );
            if ( !CheckSessionCookie( map_cookies ) )
            {
                json_rsp.emplace( "error", EnumErrorCode::ErrorChatCookieInvalid );
                conn->WriteRspBody( json_rsp.dump() );
                return;
            }

            nlohmann::json json_req = nlohmann::json::parse( conn->request.body() );
            int uuid = json_req[ "uuid" ].get<int>();
            int ssn_id = json_req[ "session_id" ].get<int>();

            if ( uuid != 0 &&
                uuid != std::stoi( map_cookies[ "uuid" ] ) )
            {
                json_rsp.emplace( "error", EnumErrorCode::ErrorChatCookieInvalid );
                conn->WriteRspBody( json_rsp.dump() );
                return;
            }

            auto messages
                = std::move( MySqlMgr::GetInstance()->SelectMessagesInSession(
                    uuid, ssn_id ) );
            nlohmann::json json_msgs;
            for ( const auto& msg : messages )
            {
                // nlohmann::json json_msg = nlohmann::json::parse( msg );
                // json_msgs.emplace_back( json_msg );
            }

            json_rsp.emplace( "error", EnumErrorCode::Success );
            json_rsp.emplace( "messages", json_msgs );
            conn->WriteRspBody( json_rsp.dump() );
        } );

    std::cout << "HttpLogicSystem构造" << std::endl;
}
