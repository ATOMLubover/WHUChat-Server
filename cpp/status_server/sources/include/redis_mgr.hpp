#pragma once

#include "singleton.hpp"
#include "redis_dao.hpp"

// Redis 控制类
class RedisMgr
    : public Singleton<RedisMgr>
{
    friend class Singleton<RedisMgr>;

public:
    RedisMgr( const RedisMgr& ) = delete;
    RedisMgr& operator=( const RedisMgr& ) = delete;
    ~RedisMgr();

    /// @brief 缓存用于用来访问 ChatServer 的 token，有效时间为 3 天
    /// @returns 是否成功设置
    bool SetChatServerToken( int uuid, const std::string& token );
    /// @brief 查询当前用户是否已经有访问 token
    /// @returns 如果返回空串则是未找到
    std::string QueryChatServerToken( int uuid );
    /// @brief 设置特定用户的 ChatServer 信息
    bool SetUserChatServer( int uuid, const std::string& host, const std::string& port );

private:
    RedisMgr();

private:
    RedisDao dao;
};