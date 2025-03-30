#pragma once

#include "singleton.hpp"
#include "redis_dao.hpp"

// Redis 控制类
class RedisMgr
    : public Singleton<RedisMgr>
{
    friend class Singleton<RedisMgr>;

public:
    RedisMgr( const RedisMgr& ) = delete;
    RedisMgr& operator=( const RedisMgr& ) = delete;
    ~RedisMgr();

private:
    RedisMgr();

private:
    RedisDao dao;
};