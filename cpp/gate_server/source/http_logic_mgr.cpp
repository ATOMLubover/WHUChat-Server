#include "http_logic_mgr.hpp"

#include "FileMgr.hpp"
#include "http_conn.hpp"
// #include "VerifyGrpcClient.h"
// #include "RedisManager.h"
// #include "ConfigManager.h"
// #include "MySqlManager.h"
// #include "StatusGrpcClient.h"

#include <fmt/core.h>

#include <iostream>

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
    // get_handlers.insert( std::make_pair( name_handler, handler ) );
    get_handlers.emplace( url, handler );
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

    std::cout << "HttpLogicMgr处理get请求的HttpConn，其url：" << conn->get_url << std::endl;
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

    std::cout << "HttpLogicMgr处理post请求的HttpConn，其url：" << conn->post_url << std::endl;
    iter_handler->second( conn );
    return true;
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
                conn->response.set( http::field::content_type, "text/plain; charset=utf-8" );

                // 随便写入一些内容
                conn->WriteRspBodyHelper( "recieved /get_test request\n" );
                int i = 0;
                for ( auto& elem : conn->get_params )
                {
                    i++;
                    conn->WriteRspBodyHelper( fmt::format( "param {}: key=\"{}\", val=\"{}\"\n",
                        i, elem.first, elem.second ) );
                }
            } );

        // 注册整个 login-test 页面
        Test_RegisterLoginTest();
    }

    std::cout << "HttpLogicMgr构造" << std::endl;
}

void HttpLogicMgr::Test_RegisterLoginTest()
{
    // 注册返回 login-test 的 index 页面
    RegisterGet(
        "/login-test", // 重定向到 /login-test/index.html
        [] ( std::shared_ptr<HttpConn> conn )
        {
            conn->response.set( http::field::content_type, "text/html; charset=utf-8" );

            // 返回 login-test 的 index 页面
            std::string index_html
                = FileMgr::GetInstance()->GetFileContent( FRONTEND_STATIC_DIR + "/login-test/index.html" ).str();
            conn->WriteRspBodyHelper( index_html );
        } );
    // 注册返回 login-test 的 css 文件
    RegisterGet(
        "/login-test/styles/main.css",
        [] ( std::shared_ptr<HttpConn> conn )
        {
            conn->response.set( http::field::content_type, "text/css; charset=utf-8" );

            // 返回 login-test 的 css 文件
            std::string css
                = FileMgr::GetInstance()->GetFileContent( FRONTEND_STATIC_DIR + "/login-test/styles/main.css" ).str();
            conn->WriteRspBodyHelper( css );
        } );
    // 注册返回 login-test 的 js 文件
    RegisterGet(
        "/login-test/scripts/main.js",
        [] ( std::shared_ptr<HttpConn> conn )
        {
            conn->response.set( http::field::content_type, "text/javascript; charset=utf-8" );

            // 返回 login-test 的 js 文件
            std::string js
                = FileMgr::GetInstance()->GetFileContent( FRONTEND_STATIC_DIR + "/login-test/scripts/main.js" ).str();
            conn->WriteRspBodyHelper( js );
        } );

}
