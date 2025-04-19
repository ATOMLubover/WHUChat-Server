#include "https_logic_system.hpp"

#include "mysql_mgr.hpp"
#include "redis_mgr.hpp"
#include "cookie_processer.hpp"
#include "config_mgr.hpp"
#include "status_grpc_mgr.hpp"
#include "verifi_grpc_mgr.hpp"

#include <json/json.hpp>
#include <fmt/format.h>

#include <iostream>
#include <filesystem>

const std::string HttpsLogicSystem::FRONTEND_STATIC_DIR = "../resources/static/frontend";

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
    RegisterDir( FRONTEND_STATIC_DIR, "login/" );
    RegisterDir( FRONTEND_STATIC_DIR, "chat/", true );

    const std::string ICO_URL = "/favicon.ico";
    RegisterGetHandler(
        ICO_URL,
        std::make_shared<HttpsReadFunc>(
            [ self = shared_from_this(), ICO_URL ] ( std::shared_ptr<SvrHttpsConn> conn ) -> HttpsResVar
            {
                auto response
                    = std::make_shared<http::response<http::file_body>>();
                response->set( http::field::content_type, "application/octet-stream" );
                nlohmann::json json_res;

                try
                {
                    const std::string REL_PATH = FRONTEND_STATIC_DIR + ICO_URL;
                    response->body() = std::move( PrepareFileBody( REL_PATH ) );
                    response->result( http::status::ok );
                }
                catch ( std::exception& exp )
                {
                    std::cerr << "/favicon.ico回调函数处发生异常：" << exp.what() << std::endl;

                    auto res_err
                        = std::make_shared<http::response<http::string_body>>();
                    nlohmann::json json_res;

                    json_res.emplace( "error", EnumErrorCode::ErrorException );
                    res_err->body() = json_res.dump();
                    res_err->set( http::field::content_type, "application/json" );

                    res_err->result( http::status::ok );

                    return res_err;
                }

                return response;
            } ) );

    RegisterGetHandler(
        "/home",
        std::make_shared<HttpsReadFunc>(
            [ self = shared_from_this() ] ( std::shared_ptr<SvrHttpsConn> conn ) -> HttpsResVar
            {
                auto response
                    = std::make_shared<http::response<http::string_body>>();
                response->result( http::status::temporary_redirect );

                try
                {
                    // 不存在 cookie 或者 cookie 失效，则直接导到 /login
                    if ( !CheckCookie( *conn->GetRequest() ) )
                    {
                        response->set( http::field::location, "/login" );
                        std::cerr << fmt::format( "SvrHttpsConn(ID: {})cookie检测不通过：{}\n",
                            conn->GetId(), conn->GetRequest()->target() );
                        return response;
                    }

                    // 有效则到到 /chat 页面
                    response->set( http::field::location, "/chat" );
                }
                catch ( std::exception& exp )
                {
                    std::cerr << "/home回调函数处发生异常：" << exp.what() << std::endl;

                    auto res_err
                        = std::make_shared<http::response<http::string_body>>();
                    nlohmann::json json_res;

                    json_res.emplace( "error", EnumErrorCode::ErrorException );
                    res_err->body() = json_res.dump();
                    res_err->set( http::field::content_type, "application/json" );

                    res_err->result( http::status::ok );

                    return res_err;
                }

                return response;
            } ) );

    RegisterGetHandler(
        "/api/v1/gate/get_chatserver",
        std::make_shared<HttpsReadFunc>(
            [ self = shared_from_this() ] ( std::shared_ptr<SvrHttpsConn> conn ) -> HttpsResVar
            {
                auto response
                    = std::make_shared<http::response<http::string_body>>();
                response->result( http::status::ok );
                nlohmann::json json_res;

                try
                {
                    // 不存在 cookie 或者 cookie 失效，则直接返回对应错误
                    if ( !CheckCookie( *conn->GetRequest() ) )
                    {
                        std::cerr << fmt::format( "SvrHttpsConn(ID: {})cookie检测不通过：{}\n",
                            conn->GetId(), conn->GetRequest()->target() );

                        json_res.emplace( "error", EnumErrorCode::ErrorLoginCookieInvalid );

                        response->body() = json_res.dump();
                        return response;
                    }

                    std::map<std::string, std::string> map_cookies
                        = CookieProcesser::Parse( ( *conn->GetRequest() )[ "cookie" ] );
                    // 在 redis 中查询 uuid 对应的 ChatServer 地址
                    std::string addr = RedisMgr::GetInstance()->GetChatServerAddr(
                        std::stoi( map_cookies[ "uuid" ] ) );
                    if ( addr == "" )
                    {
                        std::cerr << fmt::format( "SvrHttpsConn(ID: {})未找到对应的ChatServer：{}\n",
                            conn->GetId(), addr );

                        json_res.emplace( "error", EnumErrorCode::ErrorRedis );

                        response->body() = json_res.dump();
                        return response;
                    }

                    // 一切正常，则回传 addr
                    json_res.emplace( "addr", addr );
                    json_res.emplace( "error", EnumErrorCode::Success );

                    response->body() = json_res.dump();
                }
                catch ( std::exception& exp )
                {
                    std::cerr << "/home回调函数处发生异常：" << exp.what() << std::endl;

                    auto res_err
                        = std::make_shared<http::response<http::string_body>>();
                    nlohmann::json json_res;

                    json_res.emplace( "error", EnumErrorCode::ErrorException );
                    res_err->body() = json_res.dump();
                    res_err->set( http::field::content_type, "application/json" );

                    res_err->result( http::status::ok );

                    return res_err;
                }

                return response;
            } ) );
}

void HttpsLogicSystem::InitPostHandlers()
{
    RegisterPostHandler(
        "/api/v1/gate/register",
        std::make_shared<HttpsReadFunc>(
            [ self = shared_from_this() ] ( std::shared_ptr<SvrHttpsConn> conn ) -> HttpsResVar
            {
                auto response
                    = std::make_shared<http::response<http::string_body>>();
                response->set( http::field::content_type, "application/json" );
                response->result( http::status::ok );
                nlohmann::json json_res;

                try
                {
                    std::string str_req = beast::buffers_to_string( conn->GetRequest()->body().data() );
                    std::cout << "/api/v1/gate/register收到数据：" << str_req << std::endl;

                    // 解析请求体
                    nlohmann::json json_req = ParseJson( *conn->GetRequest() );
                    if ( json_req.is_null() )
                    {
                        std::cout << "/api/v1/gate/register解析JSON出错" << std::endl;
                        json_res.emplace( "error", EnumErrorCode::ErrorJson );

                        response->body() = json_res.dump();
                        return response;
                    }

                    std::string username = json_req[ "username" ].get<std::string>();
                    std::string email = json_req[ "email" ].get<std::string>();
                    std::string password = json_req[ "password" ].get<std::string>();
                    std::string repassword = json_req[ "repassword" ].get<std::string>();
                    std::string vrf_code = json_req[ "vrf_code" ].get<std::string>();

                    // 检查密码是否相同
                    if ( password != repassword )
                    {
                        json_res.emplace( "error", EnumErrorCode::ErrorPwdIncorreponds );

                        response->body() = json_res.dump();
                        return response;
                    }

                    // 检查验证码是否有效
                    if ( RedisMgr::GetInstance()->CheckVrfValid( email, vrf_code ) )
                    {
                        json_res.emplace( "error", EnumErrorCode::ErrorVrfInvalid );

                        response->body() = json_res.dump();
                        return response;
                    }

                    // 最后尝试注册
                    UserInfo info{
                        username,
                        email,
                        password };
                    int result = MySqlMgr::GetInstance()->RegisterUser( info );
                    switch ( result )
                    {
                        case -3: // email 已存在
                            json_res.emplace( "error", EnumErrorCode::ErrorEmailConflicts );
                            break;

                        case -2: // 用户名已存在
                            json_res.emplace( "error", EnumErrorCode::ErrorUsernameExists );
                            break;

                        case -1: // MySQL 或者程序错误
                            json_res.emplace( "error", EnumErrorCode::ErrorMySql );
                            break;

                        case 0: // 未定义错误
                            json_res.emplace( "error", EnumErrorCode::ErrorException );
                            break;

                        default: // 当返回值大于 0 时，是 uuid
                            json_res.emplace( "target_uuid", result );
                            json_res.emplace( "target_email", email );
                            json_res.emplace( "error", EnumErrorCode::Success );

                            // 特别注意的是，必须在此处废弃之前的验证码
                            RedisMgr::GetInstance()->DelVrfEmail( email );
                            break;
                    }

                    response->body() = json_res.dump();
                }
                catch ( std::exception& exp )
                {
                    std::cerr << "/api/v1/gate/register回调函数处发生异常：" << exp.what() << std::endl;

                    auto res_err
                        = std::make_shared<http::response<http::string_body>>();
                    nlohmann::json json_res;

                    json_res.emplace( "error", EnumErrorCode::ErrorException );
                    res_err->body() = json_res.dump();
                    res_err->set( http::field::content_type, "application/json" );

                    res_err->result( http::status::ok );

                    return res_err;
                }

                return response;
            } ) );

    RegisterPostHandler(
        "/api/v1/gate/login",
        std::make_shared<HttpsReadFunc>(
            [ self = shared_from_this() ] ( std::shared_ptr<SvrHttpsConn> conn ) -> HttpsResVar
            {
                auto response
                    = std::make_shared<http::response<http::string_body>>();
                response->set( http::field::content_type, "application/json" );
                response->result( http::status::ok );
                nlohmann::json json_res;

                try
                {
                    std::string str_req = beast::buffers_to_string( conn->GetRequest()->body().data() );
                    std::cout << "/api/v1/gate/login收到数据：" << str_req << std::endl;

                    // 解析请求体
                    nlohmann::json json_req = ParseJson( *conn->GetRequest() );
                    if ( json_req.is_null() )
                    {
                        std::cout << "/api/v1/login解析JSON出错" << std::endl;

                        response->body() = json_res.dump();
                        return response;
                    }

                    std::string email = json_req[ "email" ].get<std::string>();
                    std::string password = json_req[ "password" ].get<std::string>();

                    // 先查看是否有对应 email 被注册
                    int uuid = MySqlMgr::GetInstance()->SelectUserUuid( email );
                    if ( uuid <= 0 )
                    {
                        json_res.emplace( "error", EnumErrorCode::ErrorEmailInvalid );

                        response->body() = json_res.dump();
                        return response;
                    }

                    // 验证密码是否与存储的一致
                    std::string pwd = MySqlMgr::GetInstance()->SelectUserPwd( email );
                    if ( pwd != password )
                    {
                        json_res.emplace( "error", EnumErrorCode::ErrorPwdWrong );

                        response->body() = json_res.dump();
                        return response;
                    }

                    // 最后调用 StatusServer 分配 ChatServer
                    auto grpc_rsp
                        = StatusGrpcMgr::GetInstance()->GetChatServer( uuid );
                    if ( grpc_rsp.error() != ( std::int32_t ) EnumErrorCode::Success )
                    {
                        json_res.emplace( "error", grpc_rsp.error() );

                        response->body() = json_res.dump();
                        return response;
                    }

                    // 更新数据库中用户的 updated_at
                    if ( !MySqlMgr::GetInstance()->UpdateUserUpdatedAt( uuid ) )
                    {
                        json_res.emplace( "uuid", uuid );
                        json_res.emplace( "error", EnumErrorCode::ErrorMySql );

                        response->body() = json_res.dump();
                        return response;
                    }

                    // 直接重定向
                    response->set( http::field::location, "/chat" );
                    response->result( http::status::temporary_redirect );

                    // 并且最后写入用于免密登录的 cookie
                    // 同时更新数据库中用户的 updated_at 时间
                    response->insert(
                        http::field::set_cookie,
                        CookieProcesser::Serialize(
                            "uuid",
                            std::to_string( uuid ),
                            {
                                { "Path", "/" },
                                { "HttpOnly", "" },
                                { "Secure", "" },
                                { "SameSite", "None"},
                                { "Max-Age", "259200" } // 三天的 expire time
                            } ) );
                    response->insert(
                        http::field::set_cookie, // 设置 uuid cookie
                        CookieProcesser::Serialize(
                            "token",
                            grpc_rsp.token(),
                            {
                                { "Path", "/" },
                                { "HttpOnly", "" },
                                { "Secure", "" },
                                { "SameSite", "None"},
                                { "Max-Age", "259200" }
                            } ) );
                }
                catch ( std::exception& exp )
                {
                    std::cerr << "/api/v1/gate/login回调函数处发生异常：" << exp.what() << std::endl;

                    auto res_err
                        = std::make_shared<http::response<http::string_body>>();
                    nlohmann::json json_res;

                    json_res.emplace( "error", EnumErrorCode::ErrorException );
                    res_err->body() = json_res.dump();
                    res_err->set( http::field::content_type, "application/json" );

                    res_err->result( http::status::ok );

                    return res_err;
                }

                return response;
            } ) );

    RegisterPostHandler(
        "/api/v1/gate/send_vrf",
        std::make_shared<HttpsReadFunc>(
            [ self = shared_from_this() ] ( std::shared_ptr<SvrHttpsConn> conn ) -> HttpsResVar
            {
                auto response
                    = std::make_shared<http::response<http::string_body>>();
                response->set( http::field::content_type, "application/json" );
                response->result( http::status::ok );
                nlohmann::json json_res;

                try
                {
                    std::string str_req = beast::buffers_to_string( conn->GetRequest()->body().data() );
                    std::cout << "/api/v1/gate/login收到数据：" << str_req << std::endl;

                    // 解析请求体
                    nlohmann::json json_req = ParseJson( *conn->GetRequest() );
                    if ( json_req.is_null() )
                    {
                        std::cout << "/api/v1/login解析JSON出错" << std::endl;
                        json_res.emplace( "error", EnumErrorCode::ErrorJson );

                        response->body() = json_res.dump();
                        return response;
                    }

                    std::string email = json_req[ "email" ].get<std::string>();

                    // 检查 email 是否被注册过
                    if ( MySqlMgr::GetInstance()->SelectUserUuid( email ) >= 0 )
                    {
                        std::cout << email << "已被注册" << std::endl;
                        json_res.emplace( "error", EnumErrorCode::ErrorEmailConflicts );

                        response->body() = json_res.dump();
                        return response;
                    }

                    // 发送验证码
                    auto result
                        = VerifiGrpcMgr::GetInstance()->GetVerifiCode( email );
                    if ( result.error() != ( std::int32_t ) EnumErrorCode::Success )
                    {
                        json_res.emplace( "error", result.error() );

                        response->body() = json_res.dump();
                        return response;
                    }

                    // 若是一切正常，则返回响应
                    json_res.emplace( "target_email", email );
                    json_res.emplace( "error", result.error() );

                    response->body() = json_res.dump();
                }
                catch ( std::exception& exp )
                {
                    std::cerr << "/api/v1/chat/send_message回调函数处发生异常：" << exp.what() << std::endl;

                    auto res_err
                        = std::make_shared<http::response<http::string_body>>();
                    nlohmann::json json_res;

                    json_res.emplace( "error", EnumErrorCode::ErrorException );
                    res_err->body() = json_res.dump();
                    res_err->set( http::field::content_type, "application/json" );

                    res_err->result( http::status::ok );

                    return res_err;
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

void HttpsLogicSystem::RegisterDir(
    const std::string& prefix_offset, const std::string& url_dir, bool need_cookie )
{
    namespace fs = std::filesystem;

    if ( url_dir.empty() || url_dir.back() != '/' )
        return;

    std::string full_dir = prefix_offset + "/" + url_dir; // 完整静态文件相对路径
    bool is_dir = fs::exists( full_dir ) && fs::is_directory( full_dir );
    if ( !is_dir )
        return;

    std::string root = url_dir; // 裸路径（无最后斜杠）
    root.pop_back();

    // 先手动注册 dir 本身的重定向
    RegisterGetHandler(
        "/" + root, // 去除尾部的斜杠
        std::make_shared<HttpsReadFunc>(
            [ self = shared_from_this(), url_dir, root ] ( std::shared_ptr<SvrHttpsConn> conn ) -> HttpsResVar
            {
                auto response
                    = std::make_shared<http::response<http::string_body>>();

                try
                {
                    response->result( http::status::moved_permanently );
                    response->set( http::field::location, "/" + url_dir + "index.html" );
                }
                catch ( std::exception& exp )
                {
                    std::cerr << "/" + root << "回调函数处发生异常：" << exp.what() << std::endl;

                    auto res_err
                        = std::make_shared<http::response<http::string_body>>();
                    nlohmann::json json_res;

                    json_res.emplace( "error", EnumErrorCode::ErrorException );
                    res_err->body() = json_res.dump();
                    res_err->set( http::field::content_type, "application/json" );

                    res_err->result( http::status::ok );

                    return res_err;
                }

                return response;
            } ) );
    RegisterGetHandler(
        "/" + url_dir, // 根目录重定向到 index.html
        std::make_shared<HttpsReadFunc>(
            [ self = shared_from_this(), url_dir ] ( std::shared_ptr<SvrHttpsConn> conn ) -> HttpsResVar
            {
                auto response
                    = std::make_shared<http::response<http::string_body>>();

                try
                {
                    response->result( http::status::moved_permanently );
                    response->set( http::field::location, "/" + url_dir + "index.html" );
                }
                catch ( std::exception& exp )
                {
                    std::cerr << "/" + url_dir << "回调函数处发生异常：" << exp.what() << std::endl;

                    auto res_err
                        = std::make_shared<http::response<http::string_body>>();
                    nlohmann::json json_res;

                    json_res.emplace( "error", EnumErrorCode::ErrorException );
                    res_err->body() = json_res.dump();
                    res_err->set( http::field::content_type, "application/json" );

                    res_err->result( http::status::ok );

                    return res_err;
                }

                return response;
            } ) );

    // 在逐个注册每个文件的获得
    std::vector<std::string> files = GetDirFiles( full_dir );
    for ( const auto& rel_path : files )
    {
        std::string url = rel_path; // 后端获取文件的相对路径与实际上的前端 url 并不相通
        url.erase( 0, FRONTEND_STATIC_DIR.size() );
        RegisterGetHandler(
            url,
            std::make_shared<HttpsReadFunc>(
                [ self = shared_from_this(), need_cookie, rel_path ] ( std::shared_ptr<SvrHttpsConn> conn ) -> HttpsResVar
                {
                    auto response
                        = std::make_shared<http::response<http::file_body>>();

                    try
                    {
                        // 如果需要 cookie，则额外验证 cookie
                        if ( need_cookie )
                        {
                            if ( !CheckCookie( *conn->GetRequest() ) )
                            {
                                auto res_err
                                    = std::make_shared<http::response<http::string_body>>();
                                // 有错误，重定位到 login 页面
                                res_err->result( http::status::temporary_redirect );
                                res_err->set( http::field::location, "/login" );

                                return res_err;
                            }
                        }

                        // 假设 cookie 通过验证，开始传输文件
                        response->set( http::field::server, "ChatServer" );
                        response->set( http::field::content_type,
                            GetMimeType( rel_path ) );
                        response->body() = std::move( PrepareFileBody( rel_path ) );

                        response->result( http::status::ok );
                    }
                    catch ( std::exception& exp )
                    {
                        std::cerr << "/api/v1/chat/send_message回调函数处发生异常：" << exp.what() << std::endl;

                        auto res_err
                            = std::make_shared<http::response<http::string_body>>();
                        nlohmann::json json_res;

                        json_res.emplace( "error", EnumErrorCode::ErrorException );
                        res_err->body() = json_res.dump();
                        res_err->set( http::field::content_type, "application/json" );

                        res_err->result( http::status::ok );

                        return res_err;
                    }

                    return response;
                } ) );
    }
}

std::vector<std::string> HttpsLogicSystem::GetDirFiles( const std::string& dir )
{
    namespace fs = std::filesystem;

    fs::path dir_path( dir );

    std::vector<fs::path> file_pathes;

    // 检查路径是否存在并且是一个目录  
    if ( fs::exists( dir_path ) && fs::is_directory( dir_path ) )
    {
        // 遍历目录  
        for ( const auto& entry : fs::directory_iterator( dir_path ) )
        {
            // 仅在文件时添加到文件路径列表  
            if ( fs::is_regular_file( entry.path() ) )
            {
                file_pathes.push_back( entry.path() );
            }
            // 如果是目录，递归调用以获取其中的文件  
            else if ( fs::is_directory( entry.path() ) )
            {
                auto nestedFiles = GetDirFiles( entry.path().string() );
                file_pathes.insert( file_pathes.end(), nestedFiles.begin(), nestedFiles.end() );
            }
        }
    }

    std::vector<std::string> files( file_pathes.size() );
    int idx = 0;
    for ( auto& path : file_pathes )
    {
        files[ idx ] = file_pathes[ idx ].string();
        idx++;
    }
    return files;
}

std::string HttpsLogicSystem::GetMimeType( const std::string& file )
{
    std::size_t last_dot = file.find_last_of( '.' );
    // 没有找到 dot 的当做二进制传输
    if ( last_dot == std::string::npos )
        return "application/octet-stream";

    std::string ext = file.substr( last_dot + 1 );
    if ( ext == "txt" ) return "text/plain; charset=utf-8";
    if ( ext == "html" || ext == "htm" ) return "text/html; charset=utf-8";
    if ( ext == "js" ) return "application/js";
    if ( ext == "css" ) return "text/css; charset=utf-8";
    if ( ext == "jpg" || ext == "jpeg" ) return "image/jpeg";
    if ( ext == "png" ) return "image/png";
    if ( ext == "pdf" ) return "application/pdf";
    return "application/octet-stream";
}

http::file_body::value_type HttpsLogicSystem::PrepareFileBody( const std::string& file )
{
    // 尝试打开文件
    beast::error_code err;
    http::file_body::value_type body;
    body.open( file.c_str(), beast::file_mode::scan, err );

    if ( err )
    {
        std::cerr << file << "创建file_body::value_type失败：" << err.message() << std::endl;
        return {};
    }

    return std::move( body );
}

bool HttpsLogicSystem::CheckCookie( const http::request<http::dynamic_body>& req )
{
    try
    {
        // 如果不存在 Cookie 字段，则直接返回 false
        auto iter_cookie = req.find( "Cookie" );
        if ( iter_cookie == req.end() )
            return false;

        // 反序列化 cookie，检查其中的 uuid 是否匹配 token
        auto cookies
            = CookieProcesser::Parse( iter_cookie->value() );
        auto iter_uuid = cookies.find( "uuid" );
        if ( iter_uuid == cookies.end() )
            return false;

        // 如果 token 失效/不存在，也返回 false
        if ( !RedisMgr::GetInstance()->CheckToken(
            std::stoi( iter_uuid->second ), cookies[ "token" ] ) )
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