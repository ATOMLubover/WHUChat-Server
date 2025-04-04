#pragma once

#include "mysql_conn_pool.hpp"

#include <memory>

// DAO 层，封装 MySQL 数据库的直接操作
// 接受多线程访问
class MySqlDao
{
public:
    MySqlDao();

    /// @brief 通过 email 查找对应 uuid
    /// @returns -1 未找到，0 未定义，大于 0 则是正常的 uuid
    int SelectUserUuid( const std::string& email );
    /// @brief 通过 email 查找对应密码
    /// @returns 空字符串 未找到，非空则为正常的 password
    std::string SelectUserPwd( const std::string& email );

private:
    const int SIZE_CONN_POOL = 4; // 连接池的大小
    std::unique_ptr<MySqlConnPool> conn_pool;
};