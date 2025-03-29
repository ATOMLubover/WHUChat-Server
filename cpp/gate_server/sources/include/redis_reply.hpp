#pragma once

#include <hiredis/hiredis.h>

// RAII 类
// 用于封装 redisReply*，在析构时自动释放资源
class RedisReply
{
public:
    using Raw = redisReply;

public:
    // 考虑到安全原因，构造函数选择不隐式地自动转换指针类型
    RedisReply( Raw* reply )
        : reply( reply )
    { }
    RedisReply( RedisReply&& rhs )
    {
        reply = rhs.reply;
        rhs.reply = nullptr;
    }
    ~RedisReply()
    {
        if ( reply )
            freeReplyObject( reply );
    }

    Raw* GetReply()
    {
        return reply;
    }

    explicit operator bool()
    {
        return reply != nullptr;
    }

    Raw* operator->()
    {
        return reply;
    }

private:
    Raw* reply;
};