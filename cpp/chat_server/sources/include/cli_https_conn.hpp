#pragma once

#include "aliases.h"

#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>

#include <memory>
#include <atomic>
#include <cstdint>
#include <functional>

// 在 HTTP 客户端型链接接受到 HTTP 响应后的处理回调函数
using CliRspHandler = std::shared_ptr<
    std::function<void( http::response<http::string_body>&& )>>;

class CliHttpsConn;
// HTTP 客户端型链接超时的处理回调函数
using CliTimeoutHandler = std::shared_ptr<
    std::function<void( std::shared_ptr<CliHttpsConn> )>>;

// 作为客户端的 http 连接
class CliHttpsConn
    : public std::enable_shared_from_this<CliHttpsConn>
{
public:
    // 客户端的 http 连接无须通过 Listener 获得 socket，而是自己创建
    CliHttpsConn( net::io_context& ioc, ssl::context& ssl_ctx );
    ~CliHttpsConn();

    // 绑定要发送的目标服务器（注意当调用这个函数之后就会自动链式调用到发送）
    void AsyncSendTo( const std::string& host, const std::string& port );
    // 设定请求
    void SetRequest( http::request<http::string_body>&& req );
    // 注入获得响应的处理回调函数
    void SetRspHandler( CliRspHandler rsp_handler );

    // 注入超时处理函数
    void SetTimeoutHandler( CliTimeoutHandler timeout_handler );

    // 方便字符串化的辅助函数
    std::string ToString() const;

private:
    // 异步 resolve 的回调函数
    void OnResolved( boost::system::error_code& err, tcp::resolver::results_type results );
    // 异步 handshake 的回调函数
    void OnHandshaked( boost::system::error_code err );
    // 异步 connect 的回调函数
    void OnConnected( boost::system::error_code err, const tcp::endpoint& endpoint );
    // 异步 write 的回调函数
    void OnWritten( boost::system::error_code err, std::size_t bytes_trans );
    // 异步 read 的回调函数
    void OnRead( boost::system::error_code err, std::size_t bytes_trans );

    // 启用超时
    void EnableTimeout();

private:
    // 为 CliHttpsConn 创建唯一 id 的原子变量
    static std::atomic<std::uint32_t> serial_cnt;

    // 对使用的 ioc 的引用
    net::io_context& m_ioc;

    // 当前连接的唯一 id
    std::uint32_t m_serial_num;
    // 内置的 ssl::stream，用于发送 http 请求
    std::unique_ptr<ssl::stream<beast::tcp_stream>> m_stream;

    // 解析器
    tcp::resolver m_resolver;

    // 超时计时器（设置超时时间为 10s）
    net::steady_timer m_timer_timeout;
    // 超时回调处理函数
    CliTimeoutHandler m_timeout_handler;

    // 连接的服务器的 IP
    std::string m_host;
    // 连接的服务器的端口
    std::string m_port;

    // HTTP 请求
    http::request<http::string_body> m_request;
    // HTTP 响应
    http::response<http::string_body> m_response;

    // 处理 http 响应的回调函数（由外界设定）
    CliRspHandler m_rsp_handler;
    // 转化响应的缓冲区
    beast::flat_buffer m_rsp_buf;
};