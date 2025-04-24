#pragma once

#include "singleton.hpp"
#include "cli_https_conn.hpp"

#include <boost/asio/ssl.hpp>

// 仅管理客户端型 HTTPS 连接的管理类
class CliHttpsMgr
    : public Singleton<CliHttpsMgr>
{
    friend class Singleton<CliHttpsMgr>;

public:
    ~CliHttpsMgr();

    // 字符串化函数
    std::string ToString() const;

    // 初始化 SSL 上下文
    void Init( ssl::context* ssl_ctx );

    // 创建一个客户端型 HTTP 连接
    std::shared_ptr<CliHttpsConn> CreateConn();

    // 向特定服务器发送请求
    void AsyncRequest(
        const std::string& host, const std::string& port,
        http::request<http::string_body>&& req,
        CliRspHandler rsp_handler,
        CliTimeoutHandler timeout_handler );

private:
    CliHttpsMgr();
    CliHttpsMgr( const CliHttpsMgr& ) = delete;
    CliHttpsMgr& operator=( const CliHttpsMgr& ) = delete;

private:
    // SSL 上下文
    ssl::context* m_ssl_ctx = nullptr;
};