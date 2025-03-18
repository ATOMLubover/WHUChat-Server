#pragma once

#include <hiredis/hiredis.h>

// RAII 类
// 用于封装 redisContext*，在析构时自动释放资源
class RedisContext
{
public:
    using Raw = redisContext;

public:
    // 考虑到安全原因，构造函数选择不隐式地自动转换指针类型
    RedisContext( Raw* context )
        : context( context )
    { }
    RedisContext( RedisContext&& rhs )
        = delete;
    RedisContext& operator=( RedisContext&& rhs )
        = delete;
    ~RedisContext()
    {
        if ( context )
            redisFree( context );
    }

    Raw* GetContext()
    {
        return context;
    }

    // 释放 redisContext* 所有权，并将自己的 context 置空
    Raw* Release()
    {
        Raw* con = context;
        context = nullptr;
        return con;
    }

    explicit operator bool()
    {
        return context != nullptr;
    }

    Raw* operator->()
    {
        return context;
    }

private:
    Raw* context;
};