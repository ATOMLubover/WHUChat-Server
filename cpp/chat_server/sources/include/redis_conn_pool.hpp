#pragma once

#include "redis_context.hpp"

#include <string>
#include <atomic>
#include <queue>
#include <memory>
#include <condition_variable>

// RAII 类
// 作为 RedisMgr 的成员，用于管理 redis 连接
// 需要手动获得和还回链接（其中内置互斥锁和条件变量）
class RedisConnPool
{
public:
    RedisConnPool( std::size_t size, const std::string& host, const std::string& port, const std::string& pwd );
    RedisConnPool( const RedisConnPool& ) = delete;
    RedisConnPool& operator=( const  RedisConnPool& ) = delete;
    ~RedisConnPool();

    // 获取一个 redis 连接（条件变量同步）
    RedisContext::Raw* TakeConn();
    // 还回一个 redis 连接（锁同步）
    void ReturnConn( RedisContext::Raw* context );

    void ClosePool();

private:
    std::size_t size;
    std::queue<RedisContext> que_conn;

    std::atomic<bool> is_stopped;
    std::condition_variable cv_queconn;
    std::mutex mtx_queconn;

    std::string host;
    std::string port;
};