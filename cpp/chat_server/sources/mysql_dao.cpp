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

int MySqlDao::SelectUuid( int uuid )
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
            "SELECT EXISTS(SELECT NULL FROM users WHERE id = {})", uuid ) );
    if ( resultset->next() )
    {
        int result = resultset->getInt( 1 );
        if ( result == 1 )
            return 0;
    }

    return 1;
}

int MySqlDao::CheckSessionExisting( int ssn_id )
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
            "SELECT EXISTS(SELECT NULL FROM sessions WHERE id = {})", ssn_id ) );
    if ( resultset->next() )
    {
        int result = resultset->getInt( 1 );
        if ( result == 1 )
            return 0;
    }

    return 1;
}
std::list<ModelInfo> MySqlDao::SelectModels()
{
    std::unique_ptr<MySqlConn> conn = conn_pool->TakeConn();
    if ( conn == nullptr )
    {
        std::cout << "MySqlDao无法获取正常连接" << std::endl;
        return {};
    }

    // 如果获得的链接非空，则需要在最后返回连接
    Defer defer(
        [ this, &conn ] ()
        {
            this->conn_pool->ReturnConn( std::move( conn ) );
        } );

    MySqlStmt stmt( conn );
    std::unique_ptr<sql::ResultSet> resultset
        = stmt.Commit(
            "SELECT `id`, `name`, `reasonable`, `online`, `description` FROM `models`" );
    std::list<ModelInfo> result;
    while ( resultset->next() )
    {
        ModelInfo info{
            resultset->getInt( 1 ),
            resultset->getString( 2 ),
            resultset->getInt( 3 ),
            resultset->getInt( 4 ),
            resultset->getString( 5 ) };
        result.push_back( std::move( info ) );
    }

    return result;
}

std::list<SessionInfo> MySqlDao::SelectSessions( int uuid )
{
    std::unique_ptr<MySqlConn> conn = conn_pool->TakeConn();
    if ( conn == nullptr )
    {
        std::cout << "MySqlDao无法获取正常连接" << std::endl;
        return {};
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
            "SELECT `id`, `user_id`, `title`, `updated_at` "
            "FROM `sessions` WHERE `status` = 'active' AND `user_id` = {}", uuid ) );
    std::list<SessionInfo> result;
    while ( resultset->next() )
    {
        SessionInfo info{
            resultset->getInt( 1 ),
            resultset->getInt( 2 ),
            resultset->getString( 3 ),
            resultset->getString( 4 ) };
        result.push_back( std::move( info ) );
    }

    return result;
}

std::list<MessageInfo> MySqlDao::SelectMessages( int ssn_id )
{
    std::unique_ptr<MySqlConn> conn = conn_pool->TakeConn();
    if ( conn == nullptr )
    {
        std::cout << "MySqlDao无法获取正常连接" << std::endl;
        return {};
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
            "SELECT `id`, `sender`, `raw` FROM `messages` WHERE `session_id` = {}",
            ssn_id ) );
    std::list<MessageInfo> result;
    while ( resultset->next() )
    {
        result.push_back( MessageInfo{
            resultset->getInt( 1 ),
            resultset->getString( 2 ),
            resultset->getString( 3 ) } );
    }

    return std::move( result );
}

int MySqlDao::CreateSession( int uuid )
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
    stmt.SetStatement( fmt::format(
        "CALL CreateSession( {}, @result )",
        uuid ) );
    std::unique_ptr<sql::ResultSet> resultset
        = stmt.Commit( "SELECT @result" );
    if ( resultset->next() )
    {
        int result = resultset->getInt( 1 );
        return result;
    }

    return 0;
}

int MySqlDao::UpdateSessionTitle( int ssn_id, const std::string& title )
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
    stmt.SetNormalStatement();
    int result = stmt.Execute( fmt::format( "UPDATE `sessions` SET `title` = '{}' WHERE `id` = {}",
        title, ssn_id ) );

    return result;
}

int MySqlDao::CreateMessage(
    int uuid, int ssn_id, int model_id,
    const std::string& sender,
    const std::string& raw )
{
    // 当 raw 为空，直接返回
    if ( raw == "" )
        return -1;

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

    // 1. 使用预处理语句调用存储过程
    std::unique_ptr<sql::PreparedStatement> pstmt(
        ( conn->GetRawConn() ).prepareStatement( "CALL CreateMessage(?, ?, ?, ?, ?, @result)" )
    );

    // 2. 绑定参数（自动防注入）
    pstmt->setInt( 1, uuid );
    pstmt->setInt( 2, ssn_id );
    pstmt->setInt( 3, model_id );
    pstmt->setString( 4, sender );
    pstmt->setString( 5, raw );

    // 3. 执行存储过程
    pstmt->execute();

    // 4. 获取存储过程的输出参数
    std::unique_ptr<sql::Statement> stmt( ( conn->GetRawConn() ).createStatement() );
    std::unique_ptr<sql::ResultSet> res(
        stmt->executeQuery( "SELECT @result" )
    );

    if ( res->next() )
    {
        int result = res->getInt( 1 );
        std::cout << "CreateMessage执行完毕: " << result << std::endl;
        return result;
    }

    return 0;
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

// int MySqlDao::SelectUserUuid( const std::string& email )
// {
//     std::unique_ptr<MySqlConn> conn = conn_pool->TakeConn();
//     if ( conn == nullptr )
//     {
//         std::cout << "MySqlDao无法获取正常连接" << std::endl;
//         return -1;
//     }

//     // 如果获得的链接非空，则需要在最后返回连接
//     Defer defer(
//         [ this, &conn ] ()
//         {
//             this->conn_pool->ReturnConn( std::move( conn ) );
//         } );

//     MySqlStmt stmt( conn );
//     std::unique_ptr<sql::ResultSet> resultset
//         = stmt.Commit( fmt::format(
//             "SELECT id FROM users WHERE email = '{}'", email ) );
//     if ( resultset->next() )
//     {
//         int result = resultset->getInt( "id" );
//         return result;
//     }

//     return -1;
// }

// std::string MySqlDao::SelectUserPwd( const std::string& email )
// {
//     std::unique_ptr<MySqlConn> conn = conn_pool->TakeConn();
//     if ( conn == nullptr )
//     {
//         std::cout << "MySqlDao无法获取正常连接" << std::endl;
//         return "";
//     }

//     // 如果获得的链接非空，则需要在最后返回连接
//     Defer defer(
//         [ this, &conn ] ()
//         {
//             this->conn_pool->ReturnConn( std::move( conn ) );
//         } );

//     MySqlStmt stmt( conn );
//     std::unique_ptr<sql::ResultSet> resultset
//         = stmt.Commit( fmt::format(
//             "SELECT password FROM users WHERE email = '{}'", email ) );
//     if ( resultset->next() )
//     {
//         std::string result = resultset->getString( "password" );
//         return result;
//     }

//     return "";
// }