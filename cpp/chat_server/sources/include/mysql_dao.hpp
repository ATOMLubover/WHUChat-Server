#pragma once

#include "mysql_conn_pool.hpp"

#include <memory>

// DAO 层，封装 MySQL 数据库的直接操作
// 接受多线程访问
class MySqlDao
{
public:
    MySqlDao();

    /// @brief 确定用户 uuid 是否存在
    /// @return 错误码：-1 异常，0 查找成功，1 未找到
    int SelectUuid( int uuid );
    /// @brief 确定 sesion_id 是否存在
    /// @return 错误码：-1 异常，0 查找成功，1 未找到
    int CheckSessionExisting( int ssn_id );

    

    // 获取用户的 updated_at
    std::string SelectUserUpdatedAt( int uuid );

private:
    const int SIZE_CONN_POOL = 4; // 连接池的大小
    std::unique_ptr<MySqlConnPool> conn_pool;
};