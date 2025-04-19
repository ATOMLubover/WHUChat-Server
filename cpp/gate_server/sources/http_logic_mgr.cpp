#include "http_logic_mgr.hpp"

#include "http_conn.hpp"
#include "verifi_grpc_mgr.hpp"
#include "defer.hpp"
#include "redis_mgr.hpp"
#include "mysql_mgr.hpp"
#include "status_grpc_mgr.hpp"
#include "config_mgr.hpp"
#include "cookie_processer.hpp"
#include "date_processer.hpp"

#include <fmt/core.h>
#include <json/json.hpp>

#include <iostream>
#include <filesystem>
#include <sstream>

namespace fs = std::filesystem; // from <filesystem>

#ifdef DEBUG
const std::string HttpLogicMgr::FRONTEND_STATIC_DIR = "../resources/static/frontend";
#else
const std::string HttpLogicMgr::FRONTEND_STATIC_DIR = "./resources/static/frontend";
#endif // DEBUG


HttpLogicMgr::~HttpLogicMgr()
{
    std::cout << "HttpLogicMgr被析构" << std::endl;
}

void HttpLogicMgr::RegisterGet( const std::string& url, HttpHandler handler )
{
    get_handlers.emplace( url, handler );

    std::cout << "注册GET URL: " << url << std::endl;
}

bool HttpLogicMgr::HandleGet( std::shared_ptr<HttpConn> conn )
{
    auto iter_handler = get_handlers.find( conn->get_url );

    // 未找到则返回 false
    if ( iter_handler == get_handlers.end() )
    {
        std::cout << "HttpLogicMgr无法处理get url：" << conn->get_url << std::endl;
        return false;
    }

    std::cout << "HttpLogicMgr处理get请求，其url：" << conn->get_url << std::endl;
    iter_handler->second( conn );
    return true;
}

void HttpLogicMgr::RegisterPost( const std::string& url, HttpHandler handler )
{
    post_handlers.emplace( url, handler );

    std::cout << "注册POST URL：" << url << std::endl;
}

void HttpLogicMgr::InitGet()
{
    // 注册整个 login 页面
    AutoRegDir( FRONTEND_STATIC_DIR, "login/" );

    // 注册整个 chat 页面
    AutoRegDir( FRONTEND_STATIC_DIR, "chat/", EnumCheckCookie::Safe );

    // GET 返回网站 icon
    const std::string ICO_URL = "/favicon.ico";
    RegisterGet(
        ICO_URL,
        [ ICO_URL ] ( std::shared_ptr<HttpConn> conn )
        {
            const std::string REL_PATH = HttpLogicMgr::FRONTEND_STATIC_DIR + ICO_URL;
            auto filebody =
                std::move( HttpLogicMgr::PrepareFileBodyHelper( REL_PATH ) );

            // 分配文件响应体
            conn->ConstructFileBody();
            conn->file_response->set(
                http::field::content_type, HttpLogicMgr::GetMimeHelper( REL_PATH ) );
            conn->WriteRspBody( std::move( filebody ) );
        } );

    // 实现重定向登录
    RegisterGet(
        "/home",
        [] ( std::shared_ptr<HttpConn> conn )
        {
            // 当前端传送了有效的 cookie 时，服务器直接指示重定向到 /chat 页面
            // 但是当前端没有传送有效的 cookie 时，服务器指示重定向到 /login 页面
            try
            {
                conn->ConstructDynamicBody();
                // 保证重定向
                Defer defer(
                    [ &conn ]
                    {
                        conn->dynamic_response->result( http::status::temporary_redirect );
                    } );

                // 如果没有找到 cookie，指示重定向到 /login 页面
                auto iter = conn->request.find( http::field::cookie );
                if ( iter == conn->request.end() )
                {
                    conn->dynamic_response->set( http::field::location, "/login" );
                    return;
                }

                // 找到 cookie 则解析
                std::string str_cookie = iter->value();
                auto map_cookies
                    = std::move( CookieProcesser::Parse( str_cookie ) );
                // 如果解析不成功
                if ( !CheckCookieValid( map_cookies ) )
                {
                    conn->dynamic_response->set( http::field::location, "/login" );
                    return;
                }

                // 如果 cookie 有效，则重定向到 /chat 页面
                conn->dynamic_response->set( http::field::location, "/chat" );
            }
            catch ( std::exception& exp )
            {
                std::cout << "HttpLogicMgr处理/gate/home处异常：" << exp.what() << std::endl;

                // 这里的处理会覆盖上述设定
                conn->dynamic_response->result( http::status::internal_server_error );
            }
        } );

    // 返回可以访问的 ChatServer 信息
    RegisterGet(
        "/api/v1/get_chatserver",
        [ this ] ( std::shared_ptr<HttpConn> conn )
        {
            try
            {
                conn->ConstructDynamicBody();
                nlohmann::json json_rsp;
                // 保证响应
                Defer defer(
                    [ &conn, &json_rsp ]
                    {
                        conn->WriteRspBody( json_rsp.dump() );
                        conn->dynamic_response->result( http::status::ok );
                    } );

                // 如果没有找到 cookie，返回 1011
                auto iter = conn->request.find( http::field::cookie );
                if ( iter == conn->request.end() )
                {
                    json_rsp.emplace( "error", EnumErrorCode::ErrorLoginCookieInvalid );
                    return;
                }

                // 找到 cookie 则解析
                std::string str_cookie = iter->value();
                auto map_cookies
                    = std::move( CookieProcesser::Parse( str_cookie ) );
                // 如果解析不成功
                if ( !CheckCookieValid( map_cookies ) )
                {
                    json_rsp.emplace( "error", EnumErrorCode::ErrorLoginCookieInvalid );
                    return;
                }

                // 查询该用户可以访问的 ChatServer
                int uuid = std::stoi( map_cookies[ "uuid" ] );
                std::string addr = RedisMgr::GetInstance()->GetChatServerAddr( uuid );
                if ( addr == "" )
                {
                    json_rsp.emplace( "error", EnumErrorCode::ErrorRedis );
                    return;
                }

                json_rsp.emplace( "addr", addr );
                json_rsp.emplace( "error", EnumErrorCode::Success );
            }
            catch ( std::exception& exp )
            {
                std::cout << "HttpLogicMgr处理/api/v1/get_chatserver处异常：" << exp.what() << std::endl;

                conn->dynamic_response->result( http::status::internal_server_error );
                return;
            }
        } );
}

void HttpLogicMgr::InitPost()
{
    // 注册
    RegisterPost(
        "/api/v1/register",
        [] ( std::shared_ptr<HttpConn> conn )
        {
            conn->ConstructDynamicBody();
            conn->dynamic_response->set( http::field::content_type, "application/json; charset=utf-8" );
            nlohmann::json json_rsp; // 响应体为 JSON

            std::string str_req = beast::buffers_to_string( conn->request.body().data() );
            std::cout << "/api/v1/register收到数据：" << str_req << std::endl;

            // 为了保证能够写入内容，使用 Defer
            Defer defer_write(
                [ conn, &json_rsp ] ()
                {
                    conn->dynamic_response->result( http::status::ok );
                    conn->WriteRspBody( json_rsp.dump() );
                } );

            // 解析请求体
            nlohmann::json json_req = HttpLogicMgr::ParseJsonHelper( str_req );
            if ( json_req.is_null() )
            {
                std::cout << "/api/v1/register解析JSON出错" << std::endl;
                json_rsp.emplace( "error", EnumErrorCode::ErrorJson );
                return;
            }

            std::string username = json_req[ "username" ].get<std::string>();
            std::string email = json_req[ "email" ].get<std::string>();
            std::string password = json_req[ "password" ].get<std::string>();
            std::string repassword = json_req[ "repassword" ].get<std::string>();
            std::string vrf_code = json_req[ "vrf_code" ].get<std::string>();

            // 检查密码是否相同
            if ( password != repassword )
            {
                json_rsp.emplace( "error", EnumErrorCode::ErrorPwdIncorreponds );
                return;
            }

            // 检查验证码是否有效
            if ( RedisMgr::GetInstance()->CheckVrfValid( email, vrf_code ) )
            {
                json_rsp.emplace( "error", EnumErrorCode::ErrorVrfInvalid );
                return;
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
                    json_rsp.emplace( "error", EnumErrorCode::ErrorEmailConflicts );
                    break;

                case -2: // 用户名已存在
                    json_rsp.emplace( "error", EnumErrorCode::ErrorUsernameExists );
                    break;

                case -1: // MySQL 或者程序错误
                    json_rsp.emplace( "error", EnumErrorCode::ErrorMySql );
                    break;

                case 0: // 未定义错误
                    json_rsp.emplace( "error", EnumErrorCode::ErrorException );
                    break;

                default: // 当返回值大于 0 时，是 uuid
                    json_rsp.emplace( "target_uuid", result );
                    json_rsp.emplace( "target_email", email );
                    json_rsp.emplace( "error", EnumErrorCode::Success );

                    // 特别注意的是，必须在此处废弃之前的验证码
                    RedisMgr::GetInstance()->DelVrfEmail( email );
                    break;
            }
        } );

    // 登录
    RegisterPost(
        "/api/v1/login",
        [] ( std::shared_ptr<HttpConn> conn )
        {
            conn->ConstructDynamicBody();
            conn->dynamic_response->set( http::field::content_type, "application/json; charset=utf-8" );
            nlohmann::json json_rsp; // 响应体为 JSON

            std::string str_req = beast::buffers_to_string( conn->request.body().data() );
            std::cout << "/api/v1/login收到数据：" << str_req << std::endl;

            // 为了保证能够写入内容，使用 Defer
            Defer defer_write(
                [ conn, &json_rsp ] ()
                {
                    conn->dynamic_response->result( http::status::ok );
                    conn->WriteRspBody( json_rsp.dump() );
                } );

            // 解析请求体
            nlohmann::json json_req = HttpLogicMgr::ParseJsonHelper( str_req );
            if ( json_req.is_null() )
            {
                std::cout << "/api/v1/login解析JSON出错" << std::endl;
                json_rsp.emplace( "error", EnumErrorCode::ErrorJson );
                return;
            }

            std::string email = json_req[ "email" ].get<std::string>();
            std::string password = json_req[ "password" ].get<std::string>();

            // 先查看是否有对应 email 被注册
            int uuid = MySqlMgr::GetInstance()->SelectUserUuid( email );
            if ( uuid <= 0 )
            {
                json_rsp.emplace( "error", EnumErrorCode::ErrorEmailInvalid );
                return;
            }

            // 验证密码是否与存储的一致
            std::string pwd = MySqlMgr::GetInstance()->SelectUserPwd( email );
            if ( pwd != password )
            {
                json_rsp.emplace( "error", EnumErrorCode::ErrorPwdWrong );
                return;
            }

            // 最后调用 StatusServer 分配 ChatServer
            auto grpc_rsp
                = StatusGrpcMgr::GetInstance()->GetChatServer( uuid );
            if ( grpc_rsp.error() != ( std::int32_t ) EnumErrorCode::Success )
            {
                json_rsp.emplace( "error", grpc_rsp.error() );
                return;
            }

            // 更新数据库中用户的 updated_at
            if ( !MySqlMgr::GetInstance()->UpdateUserUpdatedAt( uuid ) )
            {
                json_req.emplace( "error", EnumErrorCode::ErrorMySql );
                return;
            }

            // 若一切成功，则返回响应
            json_rsp.emplace( "uuid", uuid );
            json_rsp.emplace( "error", grpc_rsp.error() );

            // 并且最后写入用于免密登录的 cookie
            // 同时更新数据库中用户的 updated_at 时间
            conn->dynamic_response->insert(
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
            conn->dynamic_response->insert(
                http::field::set_cookie,
                CookieProcesser::Serialize(
                    "updated_at",
                    MySqlMgr::GetInstance()->SelectUserLastLoginTime( uuid ),
                    {
                        { "Path", "/" },
                        { "HttpOnly", "" },
                        { "Secure", "" },
                        { "SameSite", "None"},
                        { "Max-Age", "259200" }
                    } ) );
            conn->dynamic_response->insert(
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
        } );

    // 发送验证码
    RegisterPost(
        "/api/v1/send_vrf",
        [] ( std::shared_ptr<HttpConn> conn )
        {
            conn->ConstructDynamicBody();
            conn->dynamic_response->set( http::field::content_type, "application/json; charset=utf-8" );
            nlohmann::json json_rsp; // 响应体为 JSON

            std::string str_req = beast::buffers_to_string( conn->request.body().data() );
            std::cout << "/api/v1/login收到数据：" << str_req << std::endl;

            // 为了保证能够写入内容，使用 Defer
            Defer defer_write(
                [ conn, &json_rsp ] ()
                {
                    conn->dynamic_response->result( http::status::ok );
                    conn->WriteRspBody( json_rsp.dump() );
                } );

            // 解析请求体
            nlohmann::json json_req = HttpLogicMgr::ParseJsonHelper( str_req );
            if ( json_req.is_null() )
            {
                std::cout << "/api/v1/login解析JSON出错" << std::endl;
                json_rsp.emplace( "error", EnumErrorCode::ErrorJson );
                return;
            }

            std::string email = json_req[ "email" ].get<std::string>();

            // 检查 email 是否被注册过
            if ( MySqlMgr::GetInstance()->SelectUserUuid( email ) >= 0 )
            {
                std::cout << email << "已被注册" << std::endl;
                json_rsp.emplace( "error", EnumErrorCode::ErrorEmailConflicts );
                return;
            }

            // 发送验证码
            auto result
                = VerifiGrpcMgr::GetInstance()->GetVerifiCode( email );
            if ( result.error() != ( std::int32_t ) EnumErrorCode::Success )
            {
                json_rsp.emplace( "error", result.error() );
                return;
            }

            // 若是一切正常，则返回响应
            json_rsp.emplace( "target_email", email );
            json_rsp.emplace( "error", result.error() );
        } );
}

bool HttpLogicMgr::HandlePost( std::shared_ptr<HttpConn> conn )
{
    auto iter_handler = post_handlers.find( conn->post_url );

    // 未找到则返回 false
    if ( iter_handler == post_handlers.end() )
    {
        std::cout << "HttpLogicMgr无法处理post url：" << conn->post_url << std::endl;
        return false;
    }

    std::cout << "HttpLogicMgr处理post请求，其url：" << conn->post_url << std::endl;
    iter_handler->second( conn );
    return true;
}

void HttpLogicMgr::AutoRegDir(
    const std::string& prefix_offset, const std::string& url_dir,
    EnumCheckCookie safe )
{
    if ( url_dir.empty() || url_dir.back() != '/' )
        return;

    std::string full_dir = prefix_offset + "/" + url_dir; // 完整静态文件相对路径
    bool is_dir = fs::exists( full_dir ) && fs::is_directory( full_dir );
    if ( !is_dir )
        return;

    std::string root = url_dir; // 裸路径（无最后斜杠）
    root.pop_back();

    // 先手动注册 dir 本身的重定向
    RegisterGet(
        "/" + root, // 去除尾部的斜杠
        [ full_dir, url_dir ] ( std::shared_ptr<HttpConn> conn )
        {
            conn->ConstructDynamicBody();

            // 控制进行永久重定向
            conn->dynamic_response->result( http::status::moved_permanently );
            conn->dynamic_response->set( http::field::location, "/" + url_dir + "index.html" );
        } );
    RegisterGet(
        "/" + url_dir, // 根目录重定向到 index.html
        [ full_dir, url_dir ] ( std::shared_ptr<HttpConn> conn )
        {
            conn->ConstructDynamicBody();

            // 控制进行永久重定向
            conn->dynamic_response->result( http::status::moved_permanently );
            conn->dynamic_response->set( http::field::location, "/" + url_dir + "index.html" );
        } );

    // 在逐个注册每个文件的获得
    std::vector<std::string> files = GetAllFilesHelper( full_dir );
    for ( const auto& rel_path : files )
    {
        std::string url = rel_path; // 后端获取文件的相对路径与实际上的前端 url 并不相通
        url.erase( 0, FRONTEND_STATIC_DIR.size() );
        RegisterGet(
            url,
            [ rel_path, safe ] ( std::shared_ptr<HttpConn> conn )
            {
                // 假设是安全模式，则需要检查 cookie
                // 当 cookie 无效时，跳转到 /login 页面
                if ( safe == EnumCheckCookie::Safe )
                {
                    // 先构建动态响应体
                    conn->ConstructDynamicBody();

                    // 如果没有找到 cookie，指示重定向到 /login 页面
                    auto iter = conn->request.find( http::field::cookie );
                    if ( iter == conn->request.end() )
                    {
                        conn->dynamic_response->set( http::field::location, "/login" );
                        conn->dynamic_response->result( http::status::temporary_redirect );
                        return;
                    }

                    // 找到 cookie 则解析
                    std::string str_cookie = iter->value();
                    auto map_cookies
                        = std::move( CookieProcesser::Parse( str_cookie ) );
                    // 如果解析不成功,，也是重定向到 /login 页面
                    if ( !CheckCookieValid( map_cookies ) )
                    {
                        conn->dynamic_response->set( http::field::location, "/login" );
                        conn->dynamic_response->result( http::status::temporary_redirect );
                        return;
                    }

                    // 通过了检测之后，不需再用动态响应体了，故销毁
                    conn->DestructDynamicBody();
                }

                // 分配文件响应体
                conn->ConstructFileBody();

                auto filebody
                    = std::move( HttpLogicMgr::PrepareFileBodyHelper( rel_path ) );

                // 设置好 header 和 body
                conn->file_response->set(
                    http::field::content_type, HttpLogicMgr::GetMimeHelper( rel_path ) );
                conn->WriteRspBody( std::move( filebody ) );

                // 最后设定状态
                conn->file_response->keep_alive( false );
                conn->file_response->result( http::status::ok );
            } );
    }
}

HttpLogicMgr::HttpLogicMgr()
{
    InitGet();
    InitPost();

    std::cout << "HttpLogicMgr构造" << std::endl;
}

std::vector<std::string> HttpLogicMgr::GetAllFilesHelper( const std::string& dir )
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
                auto nestedFiles = GetAllFilesHelper( entry.path().string() );
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

http::file_body::value_type HttpLogicMgr::PrepareFileBodyHelper( const std::string& file )
{
    // 尝试打开文件
    beast::error_code err;
    http::file_body::value_type body;
    body.open( file.c_str(), beast::file_mode::scan, err );

    if ( err )
    {
        std::cout << "创建file_body::value_type失败：" << err.message() << std::endl;
        return {};
    }

    return std::move( body );
}

std::string HttpLogicMgr::GetMimeHelper( const std::string& file )
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

nlohmann::json HttpLogicMgr::ParseJsonHelper( const std::string& str )
{
    // 将请求体转化为 json 格式
    nlohmann::json json;
    try
    {
        json = nlohmann::json::parse( str );
    }
    catch ( std::exception& exp )
    {
        std::cout << "HttpLogicMgr ParseJsonHelper处异常：" << exp.what() << std::endl;
        return {};
    }

    return json;
}

bool HttpLogicMgr::CheckCookieValid( std::map<std::string, std::string>& map_cookies )
{
    // 检查是否有 updated_at 键值对
    auto iter = map_cookies.find( "updated_at" );
    if ( iter == map_cookies.end() )
        return false;
    std::string updated_at = iter->second;

    // 检查是否有 uuid 键值对
    iter = map_cookies.find( "uuid" );
    if ( iter == map_cookies.end() )
        return false;
    int uuid = std::stoi( iter->second );

    // 检查 token 中携带的时间（也就是上次登录时间）是否与数据库一致
    std::string db_updated_at
        = MySqlMgr::GetInstance()->SelectUserLastLoginTime( uuid );
    if ( db_updated_at != updated_at )
        return false;

    // 最后检查传来的时间是否是三天内登录产生
    if ( !DateProcesser::CheckWithinDays( updated_at, 3 ) )
        return false;

    return true;
}
