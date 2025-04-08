#include "redis_mgr.hpp"

#include <iostream>

RedisMgr::~RedisMgr()
{
    std::cout << "RedisMgr被析构" << std::endl;
}

bool RedisMgr::SetChatServerToken( int uuid, const std::string& token )
{
    const std::string PREFIX = "token_user_";

    // 先设置 token
    if ( !dao.Set( PREFIX + std::to_string( uuid ), token ) )
        return false;
    // 然后设置过期时限
    const int EXPIRE_TIME = 60 * 60 * 24 * 3; // 总共三天
    if ( !dao.SetExpire( PREFIX + std::to_string( uuid ), EXPIRE_TIME ) )
        return false;

    return true;
}

std::string RedisMgr::QueryChatServerToken( int uuid )
{
    const std::string PREFIX = "token_user_";
    std::string result = "";
    dao.Get( PREFIX + std::to_string( uuid ), &result );
    return result;
}

bool RedisMgr::SetUserChatServer( int uuid, const std::string& host, const std::string& port )
{
    const std::string PREFIX = "server_user_";

    // 先设置 token
    if ( !dao.Set( PREFIX + std::to_string( uuid ), host + ":" + port ) )
        return false;
    // 然后设置过期时限
    const int EXPIRE_TIME = 60 * 60 * 24 * 3; // 总共三天
    if ( !dao.SetExpire( PREFIX + std::to_string( uuid ), EXPIRE_TIME ) )
        return false;

    return true;
}

RedisMgr::RedisMgr()
{
    std::cout << "RedisMgr构造" << std::endl;
}