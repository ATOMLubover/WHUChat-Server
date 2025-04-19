#pragma once

#include "aliases.h"

#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>

#include <memory>
#include <functional>

// 管理 HTTPS 连接创建时机的监听器
class HttpsListener
    : public std::enable_shared_from_this<HttpsListener>
{
    using AcceptHandlerType = std::function<void( tcp::socket&&, ssl::context& )>;

public:
    HttpsListener(
        net::io_context& ioc, ssl::context& ssl_ctx,
        tcp::endpoint endpoint );

    // 开始持续监听
    void Run();
    // 停止监听
    void Stop();

    // 设置复杂链接建立成功的回调
    void SetAcceptHandler( AcceptHandlerType handler );

private:
    // 异步等待连接
    // 使用了 AsioIoContextPool
    void DoAccept();
    // 处理连接到达时的回调
    void OnAccept( boost::system::error_code ec, tcp::socket socket );

private:
    // 异步监听连接
    tcp::acceptor m_acceptor;
    // 注入的 SSL 上下文
    ssl::context& m_ssl_ctx;

    // 用于调用非 accept 操作的 strand
    net::strand<net::io_context::executor_type> m_strand;

    // 外部注入，用来处理复杂的链接建立成功的回调
    AcceptHandlerType m_accept_handler;
};