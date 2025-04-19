#pragma once

#include "singleton.hpp"
#include "redis_dao.hpp"

#include <string>

// Redis 控制类
class RedisMgr
    : public Singleton<RedisMgr>
{
    friend class Singleton<RedisMgr>;

public:
    RedisMgr( const RedisMgr& ) = delete;
    RedisMgr& operator=( const RedisMgr& ) = delete;
    ~RedisMgr() = default;

    // 删除指定的键
    bool DelVrfEmail( const std::string& email );
    // 查询验证码是否存在
    bool CheckVrfValid( const std::string& email, const std::string vrf );

    // 获取用户对应的聊天服务器地址
    std::string GetChatServerAddr( int uuid );

    // 检查 token 是否过期
    bool CheckToken( int uuid, const std::string& token );

private:
    RedisMgr() = default;

private:
    RedisDao dao;
};