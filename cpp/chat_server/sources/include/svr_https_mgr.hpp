#pragma once

#include "aliases.h"
#include "singleton.hpp"
#include "https_listener.hpp"
#include "svr_https_conn.hpp"

#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>

#include <memory>
#include <functional>
#include <map>

// 服务器型 HTTPS 连接管理器
class SvrHttpsMgr
    : public Singleton<SvrHttpsMgr>
{
    friend class Singleton<SvrHttpsMgr>;

public:
    ~SvrHttpsMgr();

    // 注入 ioc，开始运行
    void Run( net::io_context& ioc );
    // 停止监听器
    void Stop();

private:
    SvrHttpsMgr();

    // 初始化 SSL 上下文
    void InitSslContext();
    // 初始化 listener
    void InitListener( net::io_context& ioc );

    // 使用 listener 传导的数据创建服务器型 HTTPS 连接
    // 隐式调用 HttpsLogicSystem 进行逻辑注入
    std::shared_ptr<SvrHttpsConn> CreateConn( net::ip::tcp::socket&& socket, ssl::context& ctx );

private:
    // 用于监听的 listener
    std::shared_ptr<HttpsListener> m_listener;

    // SSL 上下文
    std::unique_ptr<ssl::context> m_ctx;
};