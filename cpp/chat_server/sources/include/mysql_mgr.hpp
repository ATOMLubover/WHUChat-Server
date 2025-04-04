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

    // 直接返回 email 对应的 uuid
    int SelectUserUuid( const std::string& email );
    // 返回 email 对应的 password
    std::string SelectUserPwd( const std::string& email );

private:
    MySqlMgr() = default;

private:
    MySqlDao dao;
};