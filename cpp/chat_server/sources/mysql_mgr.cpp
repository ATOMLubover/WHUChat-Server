#include "mysql_mgr.hpp"

bool MySqlMgr::CheckUuidExisting( int uuid )
{
    switch ( dao.SelectUuid( uuid ) )
    {
        case -1:
        case 1:
            return false;

        case 0: // 只有返回 0 才是正常找到
            return true;

        default:
            return false;
    }
}

bool MySqlMgr::CheckSessionExisting( int ssn_id )
{
    switch ( dao.SelectSsnId( ssn_id ) )
    {
        case -1:
        case 1:
            return false;

        case 0: // 只有返回 0 才是正常找到
            return true;

        default:
            return false;
    }
}
