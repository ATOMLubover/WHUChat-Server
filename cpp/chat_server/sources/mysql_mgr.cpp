#include "mysql_mgr.hpp"

#include "sync_logger.hpp"

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
    switch ( dao.CheckSessionExisting( ssn_id ) )
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

// TODO
int MySqlMgr::CreateSession( int uuid )
{
    SyncLogger::GetInstance()->Log( LogLevel::Info, "MySqlMgr::CreateSession" );
    return 0;
}

std::string MySqlMgr::SelectUserLastLoginTime( int uuid )
{
    return dao.SelectUserUpdatedAt( uuid );
}
