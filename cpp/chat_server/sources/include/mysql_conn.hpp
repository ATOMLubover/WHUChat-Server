#pragma once

#include "mysql_elems.h"

#include <mysql_driver.h>
#include <mysql_connection.h>
#include <cppconn/exception.h>

#include <memory>

// 记录如何连接 MySQL 的数据
struct MySqlConnInfo
{
    std::string addr;       // MySQL 服务器的地址
    std::string user;       // 登录所用用户
    std::string password;   // 用户对应密码
    std::string schema;     // 要连接的架构
};

// RAII 类
// 封装对 MySQL 数据库的链接
class MySqlConn
{
public:
    MySqlConn( const MySqlConnInfo& info );

    // 重新建立一个链接
    void Rebuild();

    // 返回其存储的裸链接
    sql::Connection& GetRawConn();

public:
    MySqlConnInfo info;
    std::unique_ptr<sql::Connection> conn;
};
