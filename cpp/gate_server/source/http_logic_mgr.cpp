#include "http_logic_mgr.hpp"

#include "http_conn.hpp"
// #include "VerifyGrpcClient.h"
// #include "RedisManager.h"
// #include "ConfigManager.h"
// #include "MySqlManager.h"
// #include "StatusGrpcClient.h"

#include <fmt/core.h>

#include <iostream>


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
        return false;

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
        return false;

    std::cout << "HttpLogicMgr处理post请求的HttpConn，其url：" << conn->post_url << std::endl;
    iter_handler->second( conn );
    return true;
}

HttpLogicMgr::HttpLogicMgr()
{
    // TODO: for test
    RegisterGet( "/get_test",
        [] ( std::shared_ptr<HttpConn> conn )
        {
            conn->response.set( http::field::content_type, "text/plain; charset=utf-8" );

            // beast::ostream( conn->response.body() )
            //     << "recieved /get_test request"
            //     << std::endl;
            conn->WriteRspBodyHelper( "recieved /get_test request\n" );

            int i = 0;
            for ( auto& elem : conn->get_params )
            {
                i++;
                // beast::ostream( conn->response.body() )
                //     << fmt::format( "param {}: key=\"{}\", val=\"{}\"",
                //         i, elem.first, elem.second )
                //     << std::endl;
                conn->WriteRspBodyHelper( fmt::format( "param {}: key=\"{}\", val=\"{}\"\n",
                    i, elem.first, elem.second ) );
            }
        } );

    std::cout << "HttpLogicMgr构造" << std::endl;
}
