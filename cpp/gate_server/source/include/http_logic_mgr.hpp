#pragma once

#include "singleton.hpp"

#include <functional>
#include <map>
#include <memory>

class HttpConn;
using HttpHandler = std::function<void( std::shared_ptr<HttpConn> )>;

class HttpLogicMgr
    : public Singleton<HttpLogicMgr>
{
    friend class Singleton<HttpLogicMgr>;

public:
    ~HttpLogicMgr();

    // 注册 get 请求处理函数（url, handler）
    void RegisterGet( const std::string& url, HttpHandler handler );
    bool HandleGet( std::shared_ptr<HttpConn> conn );

    // 注册 post 请求处理函数（url, handler）
    void RegisterPost( const std::string& url, HttpHandler handler );
    bool HandlePost( std::shared_ptr<HttpConn> conn );

private:
    HttpLogicMgr();

private:
    std::map<std::string, HttpHandler> post_handlers;
    std::map<std::string, HttpHandler> get_handlers;
};
