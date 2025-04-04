#include "mysql_mgr.hpp"

int MySqlMgr::SelectUserUuid( const std::string& email )
{
    return dao.SelectUserUuid( email );
}

std::string MySqlMgr::SelectUserPwd( const std::string& email )
{
    return dao.SelectUserPwd( email );
}
