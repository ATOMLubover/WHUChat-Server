#include "http_logic_mgr.hpp"

#include "include/file_mgr.hpp"
#include "http_conn.hpp"
// #include "VerifyGrpcClient.h"
// #include "RedisManager.h"
// #include "ConfigManager.h"
// #include "MySqlManager.h"
// #include "StatusGrpcClient.h"

#include <fmt/core.h>

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

    std::cout << "注册url: " << url << std::endl;
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
    // post_handlers.insert( std::make_pair( url, handler ) );
    post_handlers.emplace( url, handler );
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
    if ( url_dir.empty() )
        return;

    std::string full_dir = prefix_offset + "/" + url_dir; // 完整相对路径

    // 先手动注册 dir 本身的重定向
    RegisterGet(
        "/" + url_dir, // 根目录重定向到 index.html
        [ full_dir ] ( std::shared_ptr<HttpConn> conn )
        {
            std::string index = full_dir + "index.html";

            // 连接文件
            auto filebody
                = std::move( HttpLogicMgr::PrepareFileBodyHelper( index ) );

            // 在此处创建好 response
            conn->ConstructFileBody();

            // 设置好 header 和 body
            conn->file_response->keep_alive( false ); // 默认创建短连接
            conn->file_response->set( http::field::content_type, HttpLogicMgr::GetMimeHelper( index ) );
            conn->WriteRspBody( std::move( filebody ) );
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
                conn->file_response->keep_alive( false ); // 默认创建短连接
                conn->file_response->set( http::field::content_type, HttpLogicMgr::GetMimeHelper( rel_path ) );
                conn->WriteRspBody( std::move( filebody ) );
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

                conn->dynamic_response->set( http::field::content_type, "text/plain; charset=utf-8" );
                // 随便写入一些内容
                conn->WriteRspBody( "recieved /get_test request\n" );
                int i = 0;
                for ( auto& elem : conn->get_params )
                {
                    i++;
                    conn->WriteRspBody( fmt::format( "param {}: key=\"{}\", val=\"{}\"\n",
                        i, elem.first, elem.second ) );
                }
            } );

        // 注册整个 login-test 页面
        AutoRegDir( FRONTEND_STATIC_DIR, "login-test/" );

        // 注册整个 binary-test 页面
        AutoRegDir( FRONTEND_STATIC_DIR, "binary-test/" );
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
