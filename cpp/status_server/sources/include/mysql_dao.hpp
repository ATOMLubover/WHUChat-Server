#pragma once

#include "mysql_conn_pool.hpp"

#include <memory>

// DAO 层，封装 MySQL 数据库的直接操作
// 接受多线程访问
class MySqlDao
{
public:
    MySqlDao();

    /**
     * 事例过程，用于 注册新用户，如果用户存在则会返回错误码
     * @returns 错误码：-2 未能找到结果，-1 MySQL 事务执行异常，
     * 0 成功执行，1 用户已存在，2 邮箱重复
     */
    int ProcRegisterUser( const MySqlUsersElem& new_user );

private:
    const int SIZE_CONN_POOL = 4; // 连接池的大小
    std::unique_ptr<MySqlConnPool> conn_pool;
};