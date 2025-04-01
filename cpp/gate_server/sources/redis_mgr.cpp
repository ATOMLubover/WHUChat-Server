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