#pragma once

#include "singleton.hpp"

#include <utility>
#include <memory>
#include <unordered_map>

class WebsockConn;

// WebsockConn 管理类
class WebsockMgr
    : public Singleton<WebsockMgr>
{
    friend class Singleton<WebsockMgr>;

public:
    WebsockMgr( const WebsockMgr& ) = delete;
    WebsockMgr& operator=( const WebsockMgr& ) = delete;
    ~WebsockMgr();

    // 添加链接，防止析构
    void AddConn( std::shared_ptr<WebsockConn> conn );
    // 删除链接
    void RmvConn( const std::string& uuid );

private:
    WebsockMgr();

private:
    std::unordered_map<std::string, std::shared_ptr<WebsockConn>> map_conn;
};