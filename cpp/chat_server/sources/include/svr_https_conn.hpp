#pragma once

#include "aliases.h"

#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>

#include <memory>
#include <string>
#include <atomic>
#include <functional>
#include <variant>
#include <map>

// 为了保证接口一致性的封装
using ResponseVar = std::variant<
    std::shared_ptr<http::response<http::string_body>>,
    std::shared_ptr<http::response<http::file_body>>>;

class SvrHttpsConn;

using ReqType = std::shared_ptr<http::request<http::dynamic_body>>; // shared_ptr 类型
using LogicGetterType = std::function<void( std::shared_ptr<SvrHttpsConn> )>; // functional 类型

using ReadHandlerType = std::shared_ptr<std::function<ResponseVar( std::shared_ptr<SvrHttpsConn> )>>; //shared_ptr 类型
using ReadFuncType = std::function<ResponseVar( std::shared_ptr<SvrHttpsConn> )>; // functional 类型

// 服务器型 HTTPS 连接
class SvrHttpsConn
    : public std::enable_shared_from_this<SvrHttpsConn>
{
public:
    SvrHttpsConn( tcp::socket&& socket, ssl::context& ctx );
    virtual ~SvrHttpsConn();

    // target URI
    std::string GetUri() const { return m_uri; }
    // GET 请求的参数
    std::map<std::string, std::string>& GetParamsOfGet() { return m_par_get; }

    ReqType GetRequest() { return m_req; }

    // 设置是否延迟发送响应
    void SetDelay( bool flag ) { m_is_delay = flag; }

    // 注册获得 handler 的回调
    void SetLogicGetter( LogicGetterType getter );
    // 注册读取成功时的回调
    void SetReadHandler( ReadHandlerType handler );

    // 异步 SSL 挥手
    // 开始运行的异步链的 Run 函数
    void DoHandshake();
    // 异步写响应（public 是因为存在延迟发送的场景）
    // 这里的 response 是裸露的响应，没有智能指针包装
    void DoWrite( auto&& response );

private:
    // 处理挥手的回调
    void OnHandShake( beast::error_code ec );

    // 异步读请求
    void DoRead();
    // 处理读请求的回调
    void OnRead( beast::error_code ec );

    // // 异步写响应
    // // 这里的 response 是裸露的响应，没有智能指针包装
    // void DoWrite( auto&& response );
    // 处理写响应的回调
    void OnWrite( beast::error_code ec );

    // 预处理请求并存储
    void Prepare();

    // 简单失败写响应函数
    void ResponseFailure( const ReqType& req, http::status status, std::string body );

private:
    // 连接编号计数器
    static std::atomic<int> total_cnt;

    // 连接编号
    int m_id;

    // 使用 SSL 流封装 TCP 套接字
    ssl::stream<tcp::socket> m_stream;
    // 由于存在延迟响应，因此 request 要存储
    ReqType m_req;

    // target URI
    std::string m_uri;
    // GET 请求的字符串形式参数
    std::string m_raw_params;
    // GET 请求的参数
    std::map<std::string, std::string> m_par_get;

    // 外部注入，告诉 HTTPS 连接去哪里找到逻辑函数
    LogicGetterType m_logic_getter;
    // 外部注入，读取成功时的回调
    ReadHandlerType m_read_handler;
    // 外部注入，设置是否延迟到外界手动发送
    bool m_is_delay;
};
