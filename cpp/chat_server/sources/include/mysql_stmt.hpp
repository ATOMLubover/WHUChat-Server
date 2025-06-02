#pragma once

#include "mysql_conn.hpp"

// #include <mysql_driver.h>
// #include <mysql_connection.h>
#include <cppconn/exception.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>

#include <memory>

// RAII 类
// 封装 MySQL 的 prepared statement
class MySqlStmt
{
public:
    MySqlStmt( std::unique_ptr<MySqlConn>& conn )
        : ref_conn( conn )
    { }

    // 设置普通语句
    void SetNormalStatement()
    {
        try
        {
            stmt.reset( ref_conn->GetRawConn().createStatement() );
        }
        catch ( sql::SQLException& e )
        {
            std::stringstream ss;
            ss << "MySqlStmt::SetNormalStatement: " << e.what();
        }
    }
    // 执行 NonQuery
    int Execute( const std::string& query )
    {
        int result = 0;
        try
        {
            if ( stmt )
                result = stmt->executeUpdate( query );
        }
        catch ( std::exception& exp )
        {
            std::cout << "MySqlStmt执行statement语句出现异常：" << exp.what() << std::endl;
            return -1;
        }
        return result;
    }

    // 设置预编译语句
    void SetStatement( const std::string& pstmt )
    {
        prestmt.reset( ref_conn->GetRawConn().prepareStatement( pstmt ) );
    }
    // 阻塞地返回运行结果
    std::unique_ptr<sql::ResultSet> Commit( const std::string& query_res = "" )
    {
        try
        {
            if ( prestmt )
                prestmt->execute();
        }
        catch ( std::exception& exp )
        {
            std::cout << "MySqlStmt执行预编译语句出现异常：" << exp.what() << std::endl;
            return nullptr;
        }

        // 获取结果集
        if ( query_res != "" )
        {
            std::unique_ptr<sql::Statement> stmt_res( ref_conn->GetRawConn().createStatement() );
            std::unique_ptr<sql::ResultSet> result( stmt_res->executeQuery( query_res ) );
            // 返回要执行的查询语句的结果
            return std::move( result );
        }

        // 如果不需要返回结果，则直接返回 nullptr
        return nullptr;
    }

private:
    std::unique_ptr<MySqlConn>& ref_conn;
    std::unique_ptr<sql::PreparedStatement> prestmt;
    std::unique_ptr<sql::Statement> stmt;
};