#include "include/redis_conn_pool.hpp"

#include "include/redis_reply.hpp"

#include <hiredis/hiredis.h>

#include <iostream>

RedisConnPool::RedisConnPool(
    std::size_t size, const std::string& host, const std::string& port, const std::string& pwd )
    : size( size ), host( host ), port( port )
    , is_stopped( false )
{
    for ( std::size_t i = 0; i < size; i++ )
    {
        RedisContext context(
            redisConnect( host.c_str(), std::stoi( port ) ) );
        if ( !context || context->err != 0 )
            continue;

        RedisReply reply( reinterpret_cast< RedisReply::Raw* >( // 由于 hiredis 是一个 C 语言库，所以需要强制转换
            redisCommand( context.GetContext(), "AUTH %s", pwd.c_str() ) ) );
        if ( reply->type == REDIS_REPLY_ERROR )
        {
            std::cout << "redis认证失败，服务器：" << host << ":" << port << std::endl;
            return;
        }
        std::cout << "redis认证成功，服务器：" << host << ":" << port << std::endl;

        que_conn.emplace( context.Release() ); // 由于 RedisContext 移动构造有暂未可知的 bug，所以使用 Release
    }
}

RedisConnPool::~RedisConnPool()
{
    std::lock_guard<std::mutex> guard( mtx_queconn );

    ClosePool();
    while ( !que_conn.empty() )
        que_conn.pop();
}

RedisContext::Raw* RedisConnPool::TakeConn()
{
    std::unique_lock<std::mutex> lock( mtx_queconn );

    cv_queconn.wait(
        lock,
        [ this ] ()
        {
            if ( this->is_stopped )
                return true;

            return !this->que_conn.empty();
        } );

    if ( is_stopped )
        return nullptr;

    RedisContext::Raw* context = que_conn.front().Release();
    que_conn.pop();
    return context;
}

void RedisConnPool::ReturnConn( RedisContext::Raw* context )
{
    std::lock_guard<std::mutex> guard( mtx_queconn );

    if ( is_stopped )
        return;

    que_conn.emplace( context );
    cv_queconn.notify_one();
}

void RedisConnPool::ClosePool()
{
    is_stopped = true;
    cv_queconn.notify_all();
}