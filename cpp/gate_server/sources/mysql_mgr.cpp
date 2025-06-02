#include "mysql_mgr.hpp"

int MySqlMgr::SelectUserUuid( const std::string& email )
{
    return dao.SelectUserUuid( email );
}

std::string MySqlMgr::SelectUserPwd( const std::string& email )
{
    return dao.SelectUserPwd( email );
}

bool MySqlMgr::UpdateUserUpdatedAt( int uuid )
{
    try
    {
        dao.UpdateUserUpdatedAt( uuid );
    }
    catch ( std::exception& exp )
    {
        std::cout << "MySqlMgr UpdateUserUpdatedAt处异常：" << exp.what() << std::endl;
        return false;
    }

    return true;
}

int MySqlMgr::RegisterUser( const UserInfo& user_info )
{
    MySqlUsersElem user_elem{
        user_info.username,
        user_info.email,
        user_info.password };
    int result = dao.ProcRegisterUser( user_elem );
    switch ( result )
    {
        case 0:
            return dao.SelectUserUuid( user_info.email );

        case -2:
        case -1:
            return -1; // 产生了异常

        case 1:
            return -2; // 用户名已存在

        case 2:
            return -3; // email 已存在

        default:
            return 0; // 未定义错误
    }
}

std::string MySqlMgr::SelectUserLastLoginTime( int uuid )
{
    return dao.SelectUserUpdatedAt( uuid );
}

UserInfo MySqlMgr::SelectUserById( int id )
{
    UserInfo user_info = dao.SelectUserById( id );
    if ( user_info.id <= 0 )
    {
        std::cerr << "MySqlMgr SelectUserById: 未找到用户 ID " << id << std::endl;
        return {};
    }
    return user_info;
}
