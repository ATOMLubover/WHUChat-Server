#pragma once

#include "singleton.hpp"

#include "cli_https_conn.hpp"

#include <memory>

// HTTP 管理类
class HttpMgr
    : public Singleton<HttpMgr>
{
    friend class Singleton<HttpMgr>;

public:
    ~HttpMgr();

    // // 字符串化
    // std::string ToString();

    // 创建 HTTP 客户端型连接
    std::shared_ptr<CliHttpsConn> CreateCliHttpConn();

private:
    HttpMgr();
    HttpMgr( const HttpMgr& ) = delete;
    HttpMgr& operator=( const HttpMgr& ) = delete;
};