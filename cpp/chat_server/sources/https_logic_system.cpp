#include "https_logic_system.hpp"

#include "mysql_mgr.hpp"
#include "redis_mgr.hpp"
#include "cookie_processer.hpp"
#include "include/cli_https_mgr.hpp"
#include "config_mgr.hpp"
#include "svr_wss_mgr.hpp"

#include <json/json.hpp>

#include <iostream>

HttpsLogicSystem::~HttpsLogicSystem()
{
    std::clog << "HttpsLogicSystem被析构" << std::endl;
}

void HttpsLogicSystem::Init()
{
    InitGetHandlers();
    InitPostHandlers();
}

void HttpsLogicSystem::RegisterGetter( std::shared_ptr<SvrHttpsConn> conn )
{
    conn->SetLogicGetter(
        [ self = shared_from_this() ] ( std::shared_ptr<SvrHttpsConn> conn ) -> bool
        {
            switch ( conn->GetRequest()->method() )
            {
                case http::verb::get:
                {
                    // 截断 wss 连接的部分
                    if ( websocket::is_upgrade( *conn->GetRequest() ) )
                    {
                        if ( SvrWssMgr::GetInstance()->CheckHttpsUpgradable( conn ) )
                            SvrWssMgr::GetInstance()->UpgradeConn( conn );
                        // 对于进入 wss 判定的部分必须返回 false
                        return false;
                    }

                    auto handler = self->FindGetHandler( conn->GetUri() );
                    if ( !handler )
                        return true;

                    conn->SetReadHandler( handler );

                    break;
                }
                case http::verb::post:
                {
                    auto handler = self->FindPostHandler( conn->GetUri() );
                    if ( !handler )
                        return true;

                    conn->SetReadHandler( handler );

                    break;
                }
                case http::verb::options:
                {
                    // 设置跨域请求
                    conn->SetReadHandler(
                        std::make_shared<HttpsReadFunc>(
                            [ self ] ( std::shared_ptr<SvrHttpsConn> conn ) -> HttpsResVar
                            {
                                auto res
                                    = std::make_shared<http::response<http::string_body>>();
                                std::string addr_gate_server
                                    = ConfigMgr::GetInstance()[ "gate_server" ][ "host" ]
                                    + ":" + ConfigMgr::GetInstance()[ "gate_server" ][ "port" ];
                                addr_gate_server = "https://" + addr_gate_server;
                                res->set( http::field::access_control_allow_origin, addr_gate_server );
                                res->set( http::field::access_control_allow_methods, "GET, POST, DEL, OPTIONS" );
                                res->set( http::field::access_control_allow_headers, "Content-Type, Accept, Authorization" );
                                // 允许携带凭证
                                res->set( http::field::access_control_allow_credentials, "true" );

                                res->result( http::status::ok );

                                return res;
                            } ) );

                    break;
                }
            }

            return true;
        } );
}

HttpsLogicSystem::HttpsLogicSystem()
{
    std::clog << "HttpsLogicSystem构造" << std::endl;
}

void HttpsLogicSystem::InitGetHandlers()
{
    RegisterGetHandler(
        "/api/v1/chat/models",
        std::make_shared<HttpsReadFunc>(
            [ self = shared_from_this() ] ( std::shared_ptr<SvrHttpsConn> conn ) -> HttpsResVar
            {
                auto response
                    = std::make_shared<http::response<http::string_body>>();
                response->set( http::field::content_type, "application/json" );
                nlohmann::json json_res;

                try
                {
                    // 先检查 cookie
                    if ( !self->CheckCookieWithUuid( *conn->GetRequest(), -1 ) )
                    {
                        json_res.emplace( "error", EnumErrorCode::ErrorChatCookieInvalid );
                        response->body() = json_res.dump();

                        return response;
                    }

                    // 检查成功之后查找数据库中的 models 信息
                    auto models = MySqlMgr::GetInstance()->SelectModels();
                    nlohmann::json json_models;
                    for ( const auto& model : models )
                    {
                        nlohmann::json json_model;
                        json_model.emplace( "id", model.m_id );
                        json_model.emplace( "name", model.m_name );
                        json_model.emplace( "reasonable", model.m_reasonable );
                        json_model.emplace( "online", model.m_online );
                        json_model.emplace( "desc", model.m_desc );

                        json_models.emplace_back( json_model );
                    }

                    json_res.emplace( "models", json_models );

                    response->body() = json_res.dump();

                    // std::string addr_gate_server
                    //     = ConfigMgr::GetInstance()[ "gate_server" ][ "host" ]
                    //     + ":" + ConfigMgr::GetInstance()[ "gate_server" ][ "port" ];
                    // addr_gate_server = "https://" + addr_gate_server;
                    // response->set( http::field::access_control_allow_origin, addr_gate_server );
                    // response->set( http::field::access_control_allow_methods, "GET, POST, DEL, OPTIONS" );
                    // response->set( http::field::access_control_allow_headers, "Content-Type, Accept, Authorization" );
                    // // 允许携带凭证
                    // response->set( http::field::access_control_allow_credentials, "true" );
                }
                catch ( std::exception& exp )
                {
                    std::cerr << "/api/v1/chat/get_models回调函数处发生异常：" << exp.what() << std::endl;

                    json_res.emplace( "error", EnumErrorCode::ErrorException );
                    response->body() = json_res.dump();

                    return response;
                }

                return response;
            } ) );
}

void HttpsLogicSystem::InitPostHandlers()
{
    RegisterPostHandler(
        "/api/v1/chat/history",
        std::make_shared<HttpsReadFunc>(
            [ self = shared_from_this() ] ( std::shared_ptr<SvrHttpsConn> conn ) -> HttpsResVar
            {
                auto response
                    = std::make_shared<http::response<http::string_body>>();
                response->set( http::field::content_type, "application/json" );
                nlohmann::json json_res;

                try
                {
                    // 解析请求体为 json
                    auto json_req = self->ParseJson( *conn->GetRequest() );
                    if ( !json_req.is_object() )
                    {
                        json_res.emplace( "error", EnumErrorCode::ErrorJson );
                        response->body() = json_res.dump();

                        return response;
                    }

                    int uuid = json_req[ "uuid" ].get<int>();

                    // 检查 cookie，并且与 uuid 进行比对
                    if ( !self->CheckCookieWithUuid( *conn->GetRequest(), uuid ) )
                    {
                        nlohmann::json json_res;
                        json_res.emplace( "error", EnumErrorCode::ErrorChatCookieInvalid );
                        response->body() = json_res.dump();

                        return response;
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

                    json_res.emplace( "error", EnumErrorCode::Success );
                    json_res.emplace( "sessions", json_ssns );

                    response->body() = json_res.dump();

                    // std::string addr_gate_server
                    //     = ConfigMgr::GetInstance()[ "gate_server" ][ "host" ]
                    //     + ":" + ConfigMgr::GetInstance()[ "gate_server" ][ "port" ];
                    // addr_gate_server = "https://" + addr_gate_server;
                    // response->set( http::field::access_control_allow_origin, addr_gate_server );
                    // response->set( http::field::access_control_allow_methods, "GET, POST, DEL, OPTIONS" );
                    // response->set( http::field::access_control_allow_headers, "Content-Type, Accept, Authorization" );
                    // // 允许携带凭证
                    // response->set( http::field::access_control_allow_credentials, "true" );
                }
                catch ( std::exception& exp )
                {
                    std::cerr << "/api/v1/chat/history回调函数处发生异常：" << exp.what() << std::endl;

                    json_res.emplace( "error", EnumErrorCode::ErrorException );
                    response->body() = json_res.dump();

                    return response;
                }

                return response;
            } ) );

    RegisterPostHandler(
        "/api/v1/chat/browse_messages",
        std::make_shared<HttpsReadFunc>(
            [ self = shared_from_this() ] ( std::shared_ptr<SvrHttpsConn> conn ) -> HttpsResVar
            {
                auto response
                    = std::make_shared<http::response<http::string_body>>();
                response->set( http::field::content_type, "application/json" );
                nlohmann::json json_res;

                try
                {
                    // 解析请求体为 json
                    auto json_req = self->ParseJson( *conn->GetRequest() );
                    if ( !json_req.is_object() )
                    {
                        json_res.emplace( "error", EnumErrorCode::ErrorJson );
                        response->body() = json_res.dump();

                        return response;
                    }

                    std::cout << json_req.dump( 4 ) << std::endl;
                    int uuid = json_req[ "uuid" ].get<int>();
                    int ssn_id = json_req[ "session_id" ].get<int>();

                    // 检查 cookie，并且与 uuid 进行比对
                    if ( !self->CheckCookieWithUuid( *conn->GetRequest(), uuid ) )
                    {
                        nlohmann::json json_res;
                        json_res.emplace( "error", EnumErrorCode::ErrorChatCookieInvalid );
                        response->body() = json_res.dump();

                        return response;
                    }

                    auto messages
                        = std::move( MySqlMgr::GetInstance()->SelectMessagesInSession(
                            uuid, ssn_id ) );
                    nlohmann::json json_msgs;
                    for ( const auto& msg : messages )
                    {
                        nlohmann::json json_msg = nlohmann::json::parse( msg.m_raw );
                        json_msg.emplace( "id", msg.m_id );
                        json_msg.emplace( "sender", msg.m_sender );
                        json_msgs.emplace_back( json_msg );
                    }

                    json_res.emplace( "error", EnumErrorCode::Success );
                    json_res.emplace( "messages", json_msgs );

                    response->body() = json_res.dump();

                    // std::string addr_gate_server
                    //     = ConfigMgr::GetInstance()[ "gate_server" ][ "host" ]
                    //     + ":" + ConfigMgr::GetInstance()[ "gate_server" ][ "port" ];
                    // addr_gate_server = "https://" + addr_gate_server;
                    // response->set( http::field::access_control_allow_origin, addr_gate_server );
                    // response->set( http::field::access_control_allow_methods, "GET, POST, DEL, OPTIONS" );
                    // response->set( http::field::access_control_allow_headers, "Content-Type, Accept, Authorization" );
                    // // 允许携带凭证
                    // response->set( http::field::access_control_allow_credentials, "true" );
                }
                catch ( std::exception& exp )
                {
                    std::cerr << "/api/v1/chat/browse_messages回调函数处发生异常：" << exp.what() << std::endl;

                    json_res.emplace( "error", EnumErrorCode::ErrorException );
                    response->body() = json_res.dump();

                    return response;
                }

                return response;
            } ) );

    RegisterPostHandler(
        "/api/v1/chat/send_message",
        std::make_shared<HttpsReadFunc>(
            [ self = shared_from_this() ] ( std::shared_ptr<SvrHttpsConn> conn ) -> HttpsResVar
            {
                auto response
                    = std::make_shared<http::response<http::string_body>>();
                response->set( http::field::content_type, "application/json" );
                nlohmann::json json_res;

                try
                {
                    // 解析请求体为 json
                    auto json_req = self->ParseJson( *conn->GetRequest() );
                    if ( !json_req.is_object() )
                    {
                        json_res.emplace( "error", EnumErrorCode::ErrorJson );
                        response->body() = json_res.dump();

                        return response;
                    }

                    int uuid = json_req[ "uuid" ].get<int>();

                    // 检查 cookie，并且与 uuid 进行比对
                    if ( !self->CheckCookieWithUuid( *conn->GetRequest(), uuid ) )
                    {
                        nlohmann::json json_res;
                        json_res.emplace( "error", EnumErrorCode::ErrorChatCookieInvalid );
                        response->body() = json_res.dump();

                        return response;
                    }

                    // 设置延迟发送，然后转发请求
                    conn->SetDelay( true );
                    self->TransferMsgToApiServer( conn );
                }
                catch ( std::exception& exp )
                {
                    std::cerr << "/api/v1/chat/send_message回调函数处发生异常：" << exp.what() << std::endl;

                    json_res.emplace( "error", EnumErrorCode::ErrorException );
                    response->body() = json_res.dump();

                    return response;
                }

                return response;
            } ) );

    RegisterPostHandler(
        "/api/v1/chat/update_title",
        std::make_shared<HttpsReadFunc>(
            [ self = shared_from_this() ] ( std::shared_ptr<SvrHttpsConn> conn ) -> HttpsResVar
            {
                auto response
                    = std::make_shared<http::response<http::string_body>>();
                response->set( http::field::content_type, "application/json" );
                nlohmann::json json_res;

                try
                {
                    // 解析请求体为 json
                    auto json_req = self->ParseJson( *conn->GetRequest() );
                    if ( !json_req.is_object() )
                    {
                        json_res.emplace( "error", EnumErrorCode::ErrorJson );
                        response->body() = json_res.dump();

                        return response;
                    }

                    int uuid = json_req[ "uuid" ].get<int>();
                    int ssn_id = json_req[ "session_id" ].get<int>();
                    std::string title = json_req[ "new_title" ].get<std::string>();

                    // 检查 cookie，并且与 uuid 进行比对
                    if ( !self->CheckCookieWithUuid( *conn->GetRequest(), uuid ) )
                    {
                        nlohmann::json json_res;
                        json_res.emplace( "error", EnumErrorCode::ErrorChatCookieInvalid );
                        response->body() = json_res.dump();

                        return response;
                    }

                    // 验证完后修改对应会话的 title
                    bool result = MySqlMgr::GetInstance()->
                        UpdateSessionTitle( ssn_id, title );
                    if ( !result )
                    {
                        json_res.emplace( "error", EnumErrorCode::ErrorUpdateTitleFailed );
                        response->body() = json_res.dump();

                        return response;
                    }
                }
                catch ( std::exception& exp )
                {
                    std::cerr << "/api/v1/chat/update_title回调函数处发生异常：" << exp.what() << std::endl;

                    json_res.emplace( "error", EnumErrorCode::ErrorException );
                    response->body() = json_res.dump();

                    return response;
                }

                return response;
            } ) );
}

HttpsReadHandler HttpsLogicSystem::FindGetHandler( const std::string& uri )
{
    auto iter = m_get_handlers.find( uri );
    if ( iter == m_get_handlers.end() )
        return nullptr;

    return iter->second;
}

HttpsReadHandler HttpsLogicSystem::FindPostHandler( const std::string& uri )
{
    auto iter = m_post_handlers.find( uri );
    if ( iter == m_post_handlers.end() )
        return nullptr;

    return iter->second;
}

void HttpsLogicSystem::RegisterGetHandler( const std::string& uri, HttpsReadHandler handler )
{
    m_get_handlers.emplace( uri, handler );

    std::clog << "注册GET请求：" << uri << std::endl;
}

void HttpsLogicSystem::RegisterPostHandler( const std::string& uri, HttpsReadHandler handler )
{
    m_post_handlers.emplace( uri, handler );

    std::clog << "注册POST请求：" << uri << std::endl;
}

bool HttpsLogicSystem::CheckCookieWithUuid( const http::request<http::dynamic_body>& req, int uuid )
{
    try
    {    // uuid == 0 时 ApiServer 的请求
        if ( uuid == 0 )
            return true;

        // 如果不存在 Cookie 字段，则直接返回 false
        auto iter_cookie = req.find( "Cookie" );
        if ( iter_cookie == req.end() )
            return false;

        // 反序列化 cookie，检查其中的 uuid 是否匹配
        // uuid == -1 则跳过后面的检查直接返回 true
        auto cookies
            = CookieProcesser::Parse( iter_cookie->value() );
        if ( uuid == -1 )
            return true;

        auto iter_uuid = cookies.find( "uuid" );
        if ( iter_uuid == cookies.end()
            || iter_uuid->second != std::to_string( uuid ) )
            return false;

        // 如果 token 失效/不存在，也返回 false
        if ( !RedisMgr::GetInstance()->QueryChatServerToken( uuid, cookies[ "token" ] ) )
            return false;
    }
    catch ( std::exception& exp )
    {
        std::cerr << "CheckCookieWithUuid处发生异常：" << exp.what() << std::endl;
        // 发生任何解析错误都返回 false
        return false;
    }

    return true;
}

nlohmann::json HttpsLogicSystem::ParseJson( const http::request<http::dynamic_body>& req )
{
    auto req_body = beast::buffers_to_string( req.body().data() );
    return nlohmann::json::parse( req_body );
}

void HttpsLogicSystem::TransferMsgToApiServer( std::shared_ptr<SvrHttpsConn> conn )
{
    http::response<http::string_body> response;
    response.set( http::field::content_type, "application/json" );
    nlohmann::json json_res;

    try
    {
        nlohmann::json json_req = ParseJson( *conn->GetRequest() );

        // 确定是否存在 ssn_id
        auto ssn_id = json_req.find( "session_id" );
        if ( ssn_id == json_req.end() )
        {
            json_res.emplace( "error", EnumErrorCode::ErrorJson );
            response.body() = json_res.dump();

            conn->DoWrite( std::move( response ) );

            return;
        }

        std::cout << json_req.dump( 4 ) << std::endl;

        int uuid = json_req[ "uuid" ].get<int>();
        //std::string sender = json_req[ "prompt" ][ "role" ].get<std::string>();
        int model_id = json_req[ "model_id" ].get<int>();
        // std::string content = json_req[ "prompt" ][ "content" ].get<std::string>();

        // 如果是新 session，则创建一个新的 session
        if ( ssn_id->is_null() )
        {
            int new_ssn_id = MySqlMgr::GetInstance()->CreateSession( uuid );
            // 如果返回小于等于 0，则创建失败
            if ( new_ssn_id <= 0 )
            {
                json_res.emplace( "error", EnumErrorCode::ErrorMySql );
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
                json_res.emplace( "error", EnumErrorCode::ErrorSsnIdInvalid );
                response.body() = json_res.dump();

                conn->DoWrite( std::move( response ) );

                return;
            }
        }

        // 记录用户提出的消息
        if ( MySqlMgr::GetInstance()->CreateMessage(
            *ssn_id, uuid, model_id,
            "", "user",
            json_req.dump() )
            != 0 )
        {
            std::cerr << "数据库记录用户消息失败" << std::endl;

            json_res.emplace( "error", EnumErrorCode::ErrorMySql );
            response.body() = json_res.dump();

            conn->DoWrite( std::move( response ) );

            return;
        }

        // 向 ApiServer 转发 http
        {
            std::cerr << "TransMsgContent试图转发: " << json_req.dump() << std::endl;

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
                    [ conn, session_id = *ssn_id ] ( http::response<http::string_body>&& cli_rsp )
                    {
                        http::response<http::string_body> response;
                        response.set( http::field::content_type, "application/json" );
                        nlohmann::json json_res;

                        std::cerr << "接受到ApiServer回复为：" << cli_rsp.body() << std::endl;

                        nlohmann::json json_cli = nlohmann::json::parse( cli_rsp.body() );
                        auto iter = json_cli.find( "error" );
                        if ( iter == json_cli.end() )
                        {
                            json_res.emplace( "error", EnumErrorCode::ErrorJson );
                            response.body() = json_res.dump();

                            conn->DoWrite( std::move( response ) );

                            return;
                        }

                        json_res.emplace( "error", json_cli[ "error" ] );
                        json_res.emplace( "session_id", session_id );

                        response.body() = json_res.dump();
                        conn->DoWrite( std::move( response ) );

                        std::string addr_gate_server
                            = ConfigMgr::GetInstance()[ "gate_server" ][ "host" ]
                            + ":" + ConfigMgr::GetInstance()[ "gate_server" ][ "port" ];
                        addr_gate_server = "https://" + addr_gate_server;
                        response.set( http::field::access_control_allow_origin, addr_gate_server );
                        response.set( http::field::access_control_allow_methods, "GET, POST, DEL, OPTIONS" );
                        response.set( http::field::access_control_allow_headers, "Content-Type, Accept, Authorization" );
                        // 允许携带凭证
                        response.set( http::field::access_control_allow_credentials, "true" );
                    } ) );
            // 设定 timeout_handler
            CliTimeoutHandler timeout_handler( std::make_shared<
                std::function<void( std::shared_ptr<CliHttpsConn> )>>(
                    [ conn ] ( std::shared_ptr<CliHttpsConn> cli_conn )
                    {
                        std::cerr << cli_conn->ToString() << " 超时" << std::endl;

                        http::response<http::string_body> response;
                        response.set( http::field::content_type, "application/json" );
                        nlohmann::json json_res;

                        json_res.emplace( "error", EnumErrorCode::ErrorApiNotResponding );
                        response.body() = json_res.dump();

                        conn->DoWrite( std::move( response ) );

                        std::string addr_gate_server
                            = ConfigMgr::GetInstance()[ "gate_server" ][ "host" ]
                            + ":" + ConfigMgr::GetInstance()[ "gate_server" ][ "port" ];
                        addr_gate_server = "https://" + addr_gate_server;
                        response.set( http::field::access_control_allow_origin, addr_gate_server );
                        response.set( http::field::access_control_allow_methods, "GET, POST, DEL, OPTIONS" );
                        response.set( http::field::access_control_allow_headers, "Content-Type, Accept, Authorization" );
                        // 允许携带凭证
                        response.set( http::field::access_control_allow_credentials, "true" );
                    } ) );

            // 异步地发送 http 请求
            CliHttpsMgr::GetInstance()->AsyncRequest(
                host, post,
                std::move( req ),
                handler,
                timeout_handler );
        }
    }
    catch ( std::exception& exp )
    {
        std::cerr << "TransferMsgToApiSerevr处产生异常: " << exp.what() << std::endl;
        return;
    }
}
