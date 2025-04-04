#pragma once

#include "singleton.hpp"
#include "aliases.h"

#include <boost/beast/http.hpp>

#include <functional>
#include <map>
#include <memory>
#include <vector>
#include <string>
#include <set>

class HttpConn;
using HttpHandler = std::function<void( std::shared_ptr<HttpConn> )>;

class WebsockConn;
using WsHandler = std::function<void( std::shared_ptr<WebsockConn> )>;

// http 和 Websocket 的逻辑独立形成的类
class HttpLogicSystem
    : public Singleton<HttpLogicSystem>
{
    friend class Singleton<HttpLogicSystem>;

public:
    ~HttpLogicSystem();

    bool HandleGet( std::shared_ptr<HttpConn> conn );
    bool HandlePost( std::shared_ptr<HttpConn> conn );

    // 将 HttpConn 升级为 WebsockConn（原链接应当被废弃）
    bool HandleUpgrade( std::shared_ptr<HttpConn> conn );

private:
    HttpLogicSystem();

    // 注册 get 请求处理函数（url, handler）
    void RegisterGet( const std::string& url, HttpHandler handler );
    // 注册 post 请求处理函数（url, handler）
    void RegisterPost( const std::string& url, HttpHandler handler );

private:
    static const std::string FRONTEND_STATIC_DIR; // 前端静态资源目录

    std::map<std::string, HttpHandler> post_handlers; // 键：url，值：处理函数
    std::map<std::string, HttpHandler> get_handlers; // 键：url，值：处理函数
};
