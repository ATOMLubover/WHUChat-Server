#pragma once

#include "mysql_conn.hpp"

#include <string>
#include <queue>
#include <mutex>
#include <atomic>
#include <condition_variable>

class MySqlConnPool
{
public:
    MySqlConnPool( const MySqlConnInfo& info, int poolsize );
    ~MySqlConnPool();

    // // 保证连接不会超时断开
    // void CheckConnections();

    std::unique_ptr<MySqlConn> TakeConn();
    void ReturnConn( std::unique_ptr<MySqlConn>&& conn );

private:
    void ClosePool();

private:
    MySqlConnInfo login_info;

    std::atomic<bool> is_stopped;

    int size;
    std::queue<std::unique_ptr<MySqlConn>> que_conn;
    std::mutex mtx_queconn;
    std::condition_variable cv_queconn;

    // std::thread check_thread; // 保证数据库连接不被自动切断的检查线程
};