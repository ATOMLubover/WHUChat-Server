#pragma once

#include "aliases.h"

#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast.hpp>
#include <boost/beast/http.hpp>

#include <memory>
#include <string>
#include <map>
#include <chrono>

class HttpLogicSystem;

// http 连接类
// 封装 tcp::socket，管理一个 http 短连接
// 用 asio 的回调异步处理 http 请求
class HttpConn
    : public std::enable_shared_from_this<HttpConn> // 由于有异步回调，所以允许 CRTP
{
    friend class HttpLogicSystem;

public:
    // HttpConn 所绑定的 ioc 由 HttpLogicSystem 提供
    HttpConn( boost::asio::io_context& ioc );
    ~HttpConn();

    // 异步监听读
    void AsyncRead();

    // 设置是否不自动发送
    void SetDelay( bool flag ) { is_delay = flag; }

    // 供 ChatServer 调用，获取 socket 以异步 async_accept
    tcp::socket& GetSocket() { return socket; }
    // 获取原生 http request
    http::request<http::string_body> GetRequest() const { return request; }

    // 获取 GET 请求参数列表
    std::map<std::string, std::string>& GetParams() { return get_params; }
    // 获取根路由 URI
    std::string GetUri() const { return get_url; }
    // 获取字符串形式的 GET 参数列表
    std::string GetRawParams() const { return get_raw_params; }

private:
    // 异步检查连接是否超时
    void AsyncCheckTimeout();
    // 同步调用 HttpLogicSystem 处理请求，但是随后异步写回 response
    void SyncHandle();
    // 异步写 response，写完后 beast 会自动发送
    void AsyncWriteResponse();

    static std::string EncodeUrlHelper( const std::string& raw );
    static std::string DecodeUrlHelper( const std::string& url );

    // 将简单的数据直接写入 dynamic_body 的响应中
    void WriteRspBody( const std::string& body );

    // 预解析 get 请求的参数
    void PreparseGetParamsHelper( const std::string uri );

private:
    tcp::socket socket; // 与客户端通信的 tcp socket

    beast::flat_buffer buf_recv; // 设置缓冲区大小为 8KB，因为一次通常接受不超过 1500B
    http::request<http::string_body> request;
    http::response<http::dynamic_body> response;

    net::steady_timer timer_timeout; // 设置超时时间为 20s

    std::string get_url; // get 请求的根路由
    std::string get_raw_params; // 用于重定向时的原始路由（仅含参数，不含问号）
    std::map<std::string, std::string> get_params; // get 请求的参数

    std::string post_url; // post 请求的根路由

    bool is_delay;
};