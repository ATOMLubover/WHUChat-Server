#pragma once

#include "singleton.hpp"
#include "mysql_dao.hpp"
#include "user_info.h"

// MySQL 管理类
class MySqlMgr
    : public Singleton<MySqlMgr>
{
    friend Singleton<MySqlMgr>;

public:
    MySqlMgr( const MySqlMgr& ) = delete;
    MySqlMgr& operator=( const MySqlMgr& ) = delete;
    ~MySqlMgr();

    int RegisterUser( const UserInfo& user_info );

private:
    MySqlMgr();
};