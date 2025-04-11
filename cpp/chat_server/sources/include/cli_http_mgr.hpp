#pragma once

#include "singleton.hpp"
#include "cli_http_conn.hpp"

// 仅管理客户端型 HTTP 连接的管理类
class CliHttpMgr
    : public Singleton<CliHttpMgr>
{
    friend class Singleton<CliHttpMgr>;

public:
    ~CliHttpMgr();

    // 字符串化函数
    std::string ToString() const;

    // 创建一个客户端型 HTTP 连接
    std::shared_ptr<CliHttpConn> CreateConn();

    // 向特定服务器发送请求
    void AsyncRequest(
        const std::string& host, const std::string& port,
        http::request<http::string_body>&& req,
        CliRspHandler rsp_handler,
        CliTimeoutHandler timeout_handler );

private:
    CliHttpMgr();
    CliHttpMgr( const CliHttpMgr& ) = delete;
    CliHttpMgr& operator=( const CliHttpMgr& ) = delete;

private:

};