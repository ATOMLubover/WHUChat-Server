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

class HttpLogicMgr;

// http 连接类
// 封装 tcp::socket，管理一个 http 短连接
// 用 asio 的回调异步处理 http 请求
class HttpConn
    : public std::enable_shared_from_this<HttpConn> // 由于有异步回调，所以允许 CRTP
{
    friend class HttpLogicMgr;

public:
    // HttpConn 所绑定的 ioc 由 HttpLogicMgr 提供
    HttpConn( boost::asio::io_context& ioc );
    ~HttpConn();

    // 异步监听读
    void AsyncReadAndHandle();

    // 供 GateServer 调用，获取 socket 以异步 async_accept
    tcp::socket& GetSocket() { return socket; }

private:
    // 异步检查连接是否超时
    void AsyncCheckTimeout();
    // 同步调用 HttpLogicMgr 处理请求，但是随后异步写回 response
    void AsyncHandleRequest();
    // 异步写 response，写完后 beast 会自动发送
    void AsyncWriteResponse();

    // 分配动态响应体
    void ConstructDynamicBody();
    // 分配文件响应体
    void ConstructFileBody();

    static std::string EncodeUrlHelper( const std::string& raw );
    static std::string DecodeUrlHelper( const std::string& url );

    // 将简单的数据直接写入 dynamic_body 的响应中
    void WriteRspBody( const std::string& body );
    // 将文件响应体写入 file_response 中
    void WriteRspBody( http::file_body::value_type&& body );

    // 预解析 get 请求的参数，结果存入 get_url 和 get_params
    void PreparseGetParamsHelper();

private:
    tcp::socket socket; // 与客户端通信的 tcp socket

    beast::flat_buffer buf_recv; // 设置缓冲区大小为 8KB，因为一次通常接受不超过 1500B
    http::request<http::dynamic_body> request;
    std::unique_ptr<http::response<http::dynamic_body>> dynamic_response;
    std::unique_ptr<http::response<http::file_body>> file_response;

    net::steady_timer timer_timeout; // 设置超时时间为 20s

    std::string get_url; // get 请求的根路由 
    std::map<std::string, std::string> get_params; // get 请求的参数

    std::string post_url; // post 请求的根路由
};