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
    ~MySqlMgr() = default;

    // 直接返回 email 对应的 uuid
    int SelectUserUuid( const std::string& email );
    // 返回 email 对应的 password
    std::string SelectUserPwd( const std::string& email );

    // 更新 uuid 对应的 updated_at
    bool UpdateUserUpdatedAt( int uuid );

    /// @brief 尝试注册用户
    /// @param user_info 
    /// @return -3 email 已存在， -2 用户名已存在，-1 异常，
    /// 0 为未定义值，大于 0 的值即为注册成功的 uuid
    int RegisterUser( const UserInfo& user_info );

    // 获取用户的上次登录时间
    std::string SelectUserLastLoginTime( int uuid );

private:
    MySqlMgr() = default;

private:
    MySqlDao dao;
};