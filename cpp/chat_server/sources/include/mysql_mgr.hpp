#pragma once

#include "singleton.hpp"
#include "mysql_dao.hpp"

// MySQL 管理类
class MySqlMgr
    : public Singleton<MySqlMgr>
{
    friend Singleton<MySqlMgr>;

public:
    MySqlMgr( const MySqlMgr& ) = delete;
    MySqlMgr& operator=( const MySqlMgr& ) = delete;
    ~MySqlMgr() = default;

    // 查询特定 uuid 是否存在
    bool CheckUuidExisting( int uuid );
    // 查询特定 session_id 是否存在
    bool CheckSessionExisting( int ssn_id );

    // 新建特定 session
    bool CreateSession( int uuid );

private:
    MySqlMgr() = default;

private:
    MySqlDao dao;
};