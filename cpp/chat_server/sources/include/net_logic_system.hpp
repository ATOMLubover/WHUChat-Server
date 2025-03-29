#pragma once

#include "singleton.hpp"
#include "aliases.h"

#include <boost/beast/http.hpp>

#include <functional>
#include <map>
#include <memory>
#include <vector>
#include <string>

class HttpConn;
using HttpHandler = std::function<void( std::shared_ptr<HttpConn> )>;

// http 和 Websocket 的逻辑独立形成的类
class NetLogicSystem
    : public Singleton<NetLogicSystem>
{
    friend class Singleton<NetLogicSystem>;

public:
    ~NetLogicSystem();

    bool HandleGet( std::shared_ptr<HttpConn> conn );
    bool HandlePost( std::shared_ptr<HttpConn> conn );

    // 判断是否是 Websocket 升级请求
    bool IsWebsockUpgrade( std::shared_ptr<HttpConn> conn );
    // 将 HttpConn 升级为 WebsockConn（原链接应当被废弃）
    void HandleUpgrade( std::shared_ptr<HttpConn> conn );

private:
    NetLogicSystem();

    // 注册 get 请求处理函数（url, handler）
    void RegisterGet( const std::string& url, HttpHandler handler );
    // 注册 post 请求处理函数（url, handler）
    void RegisterPost( const std::string& url, HttpHandler handler );

private:
    static const std::string FRONTEND_STATIC_DIR; // 前端静态资源目录

    std::map<std::string, HttpHandler> post_handlers; // 键：url，值：处理函数
    std::map<std::string, HttpHandler> get_handlers; // 键：url，值：处理函数
};
