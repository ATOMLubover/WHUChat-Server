#include "redis_mgr.hpp"

#include <iostream>

RedisMgr::~RedisMgr()
{
    std::cout << "RedisMgr被析构" << std::endl;
}

bool RedisMgr::QueryChatServerToken( int uuid, const std::string& token )
{
    const std::string PREFIX = "token_";
    std::string result = "";
    dao.Get( PREFIX + std::to_string( uuid ), &result );
    return result == token;
}

RedisMgr::RedisMgr()
{
    std::cout << "RedisMgr构造" << std::endl;
}