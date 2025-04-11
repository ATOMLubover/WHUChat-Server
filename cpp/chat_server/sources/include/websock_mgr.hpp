#pragma once

#include "singleton.hpp"

#include <utility>
#include <memory>
#include <map>
#include <cctype>
#include <mutex>
#include <set>

class WebsockConn;
class HttpConn;
class WebsockMsgPipe;

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

    // 检查是否是升级接口
    bool CheckValidUpgrade( std::shared_ptr<HttpConn> http_conn );

    // 创建 WebsockConn
    std::shared_ptr<WebsockConn> CreateConn( std::shared_ptr<HttpConn> http_conn );

private:
    WebsockMgr();

    // 检查并尝试建立 WebsockMsgPipe
    void TryBuildPipe( std::shared_ptr<WebsockConn> conn );
    // 检查一个 Websocket 升级请求的格式是否合适于 WebsockMsgPipe 的 client 建立要求
    bool CheckPipeCliFormat( std::shared_ptr<HttpConn> conn  );
    // 检查一个 Websocket 升级请求的格式是否合适于 WebsockMsgPipe 的 client 建立要求
    bool CheckPipeApiFormat( std::shared_ptr<HttpConn> conn  );
    // 检查并尝试删除管道
    void TryDelPipe( std::shared_ptr<WebsockConn> conn );

    // 检查 cookie 是否有效
    static bool CheckCookieValid( std::map<std::string, std::string>& map_cookies );

private:
    // 由 uuid 到 Conn 的映射
    std::map<std::string, std::shared_ptr<WebsockConn>> map_conn;
    // 对应连接的互斥锁
    std::mutex mtx_mapconn;
    // 保存所有转发管道
    std::map<uint64_t, std::shared_ptr<WebsockMsgPipe>> map_pipe;
    // 对应转发管道的互斥锁
    std::mutex mtx_mappipe;

    // 存储可以升级的 URI
    std::set<std::string> set_uri;
};