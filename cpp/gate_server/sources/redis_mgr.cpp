#include "redis_mgr.hpp"

bool RedisMgr::DelVrfEmail( const std::string& email )
{
    const std::string PREFIX = "code_";
    return dao.Delete( PREFIX + email );
}

bool RedisMgr::CheckVrfValid( const std::string& email, const std::string vrf )
{
    const std::string PREFIX = "code_";
    std::string val;
    // 如果没找到，直接返回 false
    if ( dao.Get( PREFIX + email, &val ) )
        return false;
    // 如果验证码不匹配，也返回 false
    if ( val != vrf )
        return false;

    return true;
}

std::string RedisMgr::GetChatServerAddr( int uuid )
{
    const std::string PREFIX = "server_user_";

    std::string addr = "";
    if ( !dao.Get( PREFIX + std::to_string( uuid ), &addr ) )
        return "";

    return addr;
}

bool RedisMgr::CheckToken( int uuid, const std::string& token )
{
    const std::string PREFIX = "token_user_";
    std::string result = "";
    dao.Get( PREFIX + std::to_string( uuid ), &result );
    return result == token;
}
