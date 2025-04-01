#include "http_logic_mgr.hpp"

#include "include/http_conn.hpp"
#include "verifi_grpc_mgr.hpp"
#include "defer.hpp"
#include "redis_mgr.hpp"
#include "mysql_mgr.hpp"
#include "status_grpc_mgr.hpp"
// #include "RedisManager.h"
// #include "ConfigManager.h"
// #include "MySqlManager.h"
// #include "StatusGrpcClient.h"

#include <fmt/core.h>
#include <json/json.hpp>

#include <iostream>
#include <filesystem>

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

void HttpLogicMgr::AutoRegDir( const std::string& prefix_offset, const std::string& url_dir )
{
    if ( url_dir.empty() || url_dir.back() != '/' )
        return;

    std::string full_dir = prefix_offset + "/" + url_dir; // 完整静态文件相对路径
    std::string root = url_dir; // 裸路径（无最后斜杠）
    root.pop_back();

    // 先手动注册 dir 本身的重定向
    RegisterGet(
        "/" + root, // 去除尾部的斜杠
        [ full_dir, url_dir ] ( std::shared_ptr<HttpConn> conn )
        {
            conn->ConstructDynamicBody();

            // 控制进行永久重定向
            conn->dynamic_response->result( 301 );
            conn->dynamic_response->set( http::field::location, "/" + url_dir + "index.html" );
        } );
    RegisterGet(
        "/" + url_dir, // 根目录重定向到 index.html
        [ full_dir, url_dir ] ( std::shared_ptr<HttpConn> conn )
        {
            conn->ConstructDynamicBody();

            // 控制进行永久重定向
            conn->dynamic_response->result( 301 );
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
            [ rel_path ] ( std::shared_ptr<HttpConn> conn )
            {
                auto filebody
                    = std::move( HttpLogicMgr::PrepareFileBodyHelper( rel_path ) );

                // 分配文件响应体
                conn->ConstructFileBody();

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
    // TODO: for test
    {
        // 注册一个测试用的 get 请求
        RegisterGet(
            "/get_test",
            [] ( std::shared_ptr<HttpConn> conn )
            {
                // 分配动态响应体
                conn->ConstructDynamicBody();

                conn->dynamic_response->set(
                    http::field::content_type, "text/plain; charset=utf-8" );
                // 随便写入一些内容
                conn->WriteRspBody( "recieved /get_test request\n" );
                int i = 0;
                for ( auto& elem : conn->get_params )
                {
                    i++;
                    conn->WriteRspBody( fmt::format( "param {}: key=\"{}\", val=\"{}\"\n",
                        i, elem.first, elem.second ) );
                }

                // 最后设置状态
                conn->dynamic_response->keep_alive( false );
                conn->dynamic_response->result( http::status::ok );
            } );

        // 注册整个 login-test 页面
        AutoRegDir( FRONTEND_STATIC_DIR, "login-test/" );

        // 注册整个 binary-test 页面
        AutoRegDir( FRONTEND_STATIC_DIR, "binary-test/" );

        // POST 处理发送验证码的请求
        RegisterPost(
            "/login-test/post_verification",
            [] ( std::shared_ptr<HttpConn> conn )
            {
                std::string str_req_body = beast::buffers_to_string( conn->request.body().data() );
                std::cout << "获得POST请求体：" << str_req_body << std::endl;

                // 由于是传输 json 返回，所以分配动态响应体
                conn->ConstructDynamicBody();
                nlohmann::json json_rsp_body;

                // 将请求体转化为 json 格式
                nlohmann::json json_req_body;
                try
                {
                    json_req_body = nlohmann::json::parse( str_req_body );
                }
                catch ( std::exception& exp )
                {
                    std::cout << "HttpLogicMgr 解析发送email的json处异常：" << exp.what() << std::endl;

                    // 返回 ErrorJson
                    json_rsp_body.emplace( "error", ( int ) EnumErrorCode::ErrorJson );
                    conn->WriteRspBody( json_rsp_body.dump() );

                    return;
                }

                // 成功转换之后，提取 email
                std::string email = json_req_body[ "email" ].get<std::string>();

                // 调用 VerifiGrpcMgr 处理发送验证码的工作
                message::GetVerifiResponse verifi_rsp;
                try
                {
                    verifi_rsp = VerifiGrpcMgr::GetInstance()->GetVerifiCode( email );
                }
                catch ( std::exception& exp )
                {
                    std::cout << "VerifiGrpcMgr处理发送邮件处异常：" << exp.what() << std::endl;

                    // 返回 ErrorGrpc
                    json_rsp_body.emplace( "error", ( int ) EnumErrorCode::ErrorGrpc );
                    conn->WriteRspBody( json_rsp_body.dump() );

                    return;
                }

                // 如果一切正常
                json_rsp_body.emplace( "email", email );
                json_rsp_body.emplace( "error", verifi_rsp.error() );

                conn->WriteRspBody( json_rsp_body.dump() );

                return;
            } );


    }

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

    // POST 部分
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

                // 若分配成功，则返回响应
                json_rsp.emplace( "uuid", uuid );
                json_rsp.emplace( "error", grpc_rsp.error() );
                json_rsp.emplace( "token", grpc_rsp.token() );
                json_rsp.emplace( "host", grpc_rsp.host() );
                json_rsp.emplace( "port", grpc_rsp.port() );
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

    std::cout << "HttpLogicMgr构造" << std::endl;
}

std::vector<std::string> HttpLogicMgr::GetAllFilesHelper( const std::string& dir )
{
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
