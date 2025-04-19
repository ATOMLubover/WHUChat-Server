#pragma once

#include "mysql_conn_pool.hpp"

#include <memory>
#include <list>
#include <string>

// models 表的结构
struct ModelInfo
{
    int m_id;
    std::string m_name;
    std::string m_class;
    std::string m_desc;
};

// message 表的结构
struct MessageInfo
{
    int m_id;
    std::string m_raw;
};

// sessions 表的结构
struct SessionInfo
{
    int m_id;
    int m_uuid;
    std::string m_title;
    std::string m_updated_at;
};

// DAO 层，封装 MySQL 数据库的直接操作
// 接受多线程访问
class MySqlDao
{
public:
    MySqlDao();

    /// @brief 确定用户 uuid 是否存在
    /// @return 错误码：-1 异常，0 查找成功，1 未找到
    int SelectUuid( int uuid );
    /// @brief 确定 sesion_id 是否存在
    /// @return 错误码：-1 异常，0 查找成功，1 未找到
    int CheckSessionExisting( int ssn_id );

    /// @brief 获取所有 model 的信息
    std::list<ModelInfo> SelectModels();

    /// @brief 获取指定用户所有的 session 的基本信息
    std::list<SessionInfo> SelectSessions( int uuid );
    /// @brief 获取指定 session 的信息
    std::list<MessageInfo> SelectMessages( int uuid, int ssn_id );

    /// @brief 创建新会话
    /// @return 错误码：-3 model_id 不存在，-2 uuid 不存在，-1 异常，0 未定义，大于 0 创建成功
    /// @note 创建成功后，会返回 session_id
    int CreateSession( int uuid );
    /// @brief 更新 session 的标题
    /// @return 错误码：-1 异常，0 未存在结果，1 session_id 不存在
    int UpdateSessionTitle( int ssn_id, const std::string& title );

    /// @brief 创建新 message
    /// @return 错误码：-3 model_id 不存在, -2 session_id 不存在，-1 异常，0 未定义，大于 0 创建成功
    /// @note 创建成功后，会返回 session_id
    int CreateMessage(
        int uuid, int ssn_id, int model_id,
        const std::string& content, const std::string& sender,
        const std::string& raw );

    // 获取用户的 updated_at
    std::string SelectUserUpdatedAt( int uuid );

private:
    const int SIZE_CONN_POOL = 4; // 连接池的大小
    std::unique_ptr<MySqlConnPool> conn_pool;
};