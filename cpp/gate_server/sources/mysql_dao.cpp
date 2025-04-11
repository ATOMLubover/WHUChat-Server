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

void MySqlDao::UpdateUserUpdatedAt( int uuid )
{
    std::unique_ptr<MySqlConn> conn = conn_pool->TakeConn();
    if ( conn == nullptr )
    {
        std::cout << "MySqlDao无法获取正常连接" << std::endl;
        return;
    }

    // 如果获得的链接非空，则需要在最后返回连接
    Defer defer(
        [ this, &conn ] ()
        {
            this->conn_pool->ReturnConn( std::move( conn ) );
        } );

    MySqlStmt stmt( conn );
    stmt.SetStatement( fmt::format(
        "UPDATE users SET updated_at = NOW() WHERE id = {}", uuid ) );
    stmt.Commit();
}

std::string MySqlDao::SelectUserUpdatedAt( int uuid )
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
            "SELECT updated_at FROM users WHERE id = '{}'", uuid ) );
    if ( resultset->next() )
    {
        std::string result = resultset->getString( 1 );
        return result;
    }

    return "";
}

int MySqlDao::ProcRegisterUser( const MySqlUsersElem& new_user )
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

    MySqlStmt pstmt( conn );
    // 调用过程，尝试插入新用户
    std::string str_pstmt = fmt::format(
        "CALL RegisterUser( '{}', '{}', '{}', @result )",
        new_user.username, new_user.email, new_user.password );
    pstmt.SetStatement( str_pstmt );
    // 查询结果
    std::unique_ptr<sql::ResultSet> resultset
        = pstmt.Commit( "SELECT @result AS result" );
    if ( resultset->next() )
    {
        int result = resultset->getInt( "result" );
        std::cout << "注册新用户查询结果为：" << result << std::endl;
        return result;
    }

    // 没有查询到结果时返回 -2（与 MySQL 异常的 -1 进行区分）
    return -2;
}
