#pragma once

#include "singleton.hpp"
#include "mysql_dao.hpp"

#include <list>

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

    // 获取所有模型信息
    std::list<ModelInfo> SelectModels();

    // 获取一个用户的所有 session
    std::list<SessionInfo> SelectSessions( int uuid );
    // 获取一个特定会话的 message
    std::list<MessageInfo> SelectMessagesInSession( int uuid, int ssn_id );

    // 新建特定 session（返回创建的 ssn_id）
    int CreateSession( int uuid );
    // 更新特定 session 的标题
    bool UpdateSessionTitle( int ssn_id, const std::string& title );

    // 创建新消息
    int CreateMessage(
        int ssn_id, int uuid, int model_id,
        const std::string& content, const std::string& sender,
        std::string raw );

    // 获取用户的上次登录时间
    std::string SelectUserLastLoginTime( int uuid );

private:
    MySqlMgr() = default;

private:
    MySqlDao dao;
};