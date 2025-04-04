#include "mysql_dao.hpp"

#include "config_mgr.hpp"
#include "mysql_stmt.hpp"
#include "defer.hpp"

#include <fmt/format.h>

MySqlDao::MySqlDao()
{
    SubSection section_mysql = ConfigMgr::GetInstance()[ "mysql" ];
    MySqlConnInfo info =
    {
        section_mysql[ "host" ] + ":" + section_mysql[ "post" ],
        section_mysql[ "user" ],
        section_mysql[ "password" ],
        section_mysql[ "schema" ]
    };
    conn_pool.reset( new MySqlConnPool( info, SIZE_CONN_POOL ) );
}

int MySqlDao::SelectUserUuid( const std::string& email )
{
    std::unique_ptr<MySqlConn> conn = conn_pool->TakeConn();
    if ( conn == nullptr )
    {
        std::cout << "MySqlDao无法获取正常连接" << std::endl;
        return -1;
    }

    // 如果获得的链接非空，则需要在最后返回连接
    Defer defer(
        [ this, &conn ] ()
        {
            this->conn_pool->ReturnConn( std::move( conn ) );
        } );

    MySqlStmt stmt( conn );
    std::unique_ptr<sql::ResultSet> resultset
        = stmt.Commit( fmt::format(
            "SELECT id FROM users WHERE email = '{}'", email ) );
    if ( resultset->next() )
    {
        int result = resultset->getInt( "id" );
        return result;
    }

    return -1;
}

std::string MySqlDao::SelectUserPwd( const std::string& email )
{
    std::unique_ptr<MySqlConn> conn = conn_pool->TakeConn();
    if ( conn == nullptr )
    {
        std::cout << "MySqlDao无法获取正常连接" << std::endl;
        return "";
    }

    // 如果获得的链接非空，则需要在最后返回连接
    Defer defer(
        [ this, &conn ] ()
        {
            this->conn_pool->ReturnConn( std::move( conn ) );
        } );

    MySqlStmt stmt( conn );
    std::unique_ptr<sql::ResultSet> resultset
        = stmt.Commit( fmt::format(
            "SELECT password FROM users WHERE email = '{}'", email ) );
    if ( resultset->next() )
    {
        std::string result = resultset->getString( "password" );
        return result;
    }

    return "";
}