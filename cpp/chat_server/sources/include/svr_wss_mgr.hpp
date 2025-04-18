#pragma once

#include "aliases.h"
#include "singleton.hpp"

#include <memory>
#include <map>
#include <set>
#include <mutex>

class SvrHttpsConn;
class SvrWssConn;
class SvrWssPipe;

// 服务器型 WSS 连接管理器
class SvrWssMgr
    : public Singleton<SvrWssMgr>
{
    friend class Singleton<SvrWssMgr>;

public:
    ~SvrWssMgr();
    SvrWssMgr( const SvrWssMgr& ) = delete;
    SvrWssMgr& operator=( const SvrWssMgr& ) = delete;

    // 检查 HTTPS 连接是否包含 WSS 升级请求
    bool CheckHttpsUpgradable( std::shared_ptr<SvrHttpsConn> conn );

    // 创建连接
    void UpgradeConn( std::shared_ptr<SvrHttpsConn> conn );

private:
    SvrWssMgr();

    // 添加连接，防止析构
    void AddConn( std::shared_ptr<SvrWssConn> conn );
    // 去除连接
    void RmvConn( int uuid );

    ////////////////////////
    // 以下是实际的逻辑部分 //
    ////////////////////////

    // 检查 cookie
    bool CheckCookieWithUuid( std::shared_ptr<SvrHttpsConn> conn, int uuid );

    // 检查一个连接是否符合 WSS 管道客户端的格式
    bool CheckPipeCliFormat( std::shared_ptr<SvrHttpsConn> conn );
    // 检查一个连接是否符合 WSS 管道服务器的格式
    bool CheckPipeSvrFormat( std::shared_ptr<SvrHttpsConn> conn );

    // 尝试建立管道
    void TryBuildPipe( std::shared_ptr<SvrWssConn> conn );
    // 尝试删除管道
    void TryDelPipe( std::shared_ptr<SvrWssConn> conn );

private:
    // 所有 SvrWssConn 的 map
    std::map<int, std::shared_ptr<SvrWssConn>> m_map_conn;
    // 对应连接 map 的互斥锁
    std::mutex m_mtx_map_conn;

    // 可升级接口的集合
    std::set<std::string> m_set_upgradable_uri;
    // 管道接口的集合
    std::set<std::string> m_set_pipe_uri;

    // 所有管道的 map
    std::map<int, std::shared_ptr<SvrWssPipe>> m_map_pipe;
    // 管道变量的互斥锁
    std::mutex m_mtx_pipe;
};