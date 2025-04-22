#include "mysql_mgr.hpp"

#include "sync_logger.hpp"

#include <json/json.hpp>

#include <list>
#include <algorithm>

bool MySqlMgr::CheckUuidExisting( int uuid )
{
    switch ( dao.SelectUuid( uuid ) )
    {
        case -1:
        case 1:
            return false;

        case 0: // 只有返回 0 才是正常找到
            return true;

        default:
            return false;
    }
}

bool MySqlMgr::CheckSessionExisting( int ssn_id )
{
    switch ( dao.CheckSessionExisting( ssn_id ) )
    {
        case -1:
        case 1:
            return false;

        case 0: // 只有返回 0 才是正常找到
            return true;

        default:
            return false;
    }
}

std::list<ModelInfo> MySqlMgr::SelectModels()
{
    try
    {
        return dao.SelectModels();
    }
    catch ( std::exception& exp )
    {
        SyncLogger::GetInstance()->Log(
            LogLevel::Error,
            "MySqlMgr::SelectModels处异常: {}", exp.what() );
        return {};
    }
}

std::list<SessionInfo> MySqlMgr::SelectSessions( int uuid )
{
    try
    {
        return dao.SelectSessions( uuid );
    }
    catch ( const std::exception& exp )
    {
        SyncLogger::GetInstance()->Log(
            LogLevel::Error,
            "MySqlMgr::SelectSessions处异常: {}",
            exp.what() );
        return {};
    }
}

std::list<MessageInfo> MySqlMgr::SelectMessagesInSession( int uuid, int ssn_id )
{
    try
    {
        auto sessions = dao.SelectSessions( uuid );
        if ( std::find_if(
            sessions.begin(),
            sessions.end(),
            [ ssn_id ] ( const SessionInfo& info ) { return info.m_id == ssn_id; }
        )
            == sessions.end() )
        {
            return {};
        }

        return dao.SelectMessages( ssn_id );;
    }
    catch ( const std::exception& exp )
    {
        SyncLogger::GetInstance()->Log(
            LogLevel::Error,
            "MySqlMgr::SelectMessages处异常: {}",
            exp.what() );
        return {};
    }
}

int MySqlMgr::CreateSession( int uuid )
{
    try
    {
        int result = dao.CreateSession( uuid );
        switch ( result )
        {
            case 0:
            case -1:
            case -2:
            case -3:
                return -1;

            default:
                return result;
        }
    }
    catch ( const std::exception& exp )
    {
        SyncLogger::GetInstance()->Log(
            LogLevel::Error,
            "MySqlMgr::CreateSession处异常: {}",
            exp.what() );
    }
    return 0;
}

bool MySqlMgr::UpdateSessionTitle( int ssn_id, const std::string& title )
{
    try
    {
        int result = dao.UpdateSessionTitle( ssn_id, title );
        switch ( result )
        {
            case 0:
            case -1:
                return false;

            default:
                return true;
        }
    }
    catch ( const std::exception& exp )
    {
        SyncLogger::GetInstance()->Log(
            LogLevel::Error,
            "MySqlMgr::UpdateSessionTitle处异常: {}",
            exp.what() );
        return false;
    }
}

int MySqlMgr::CreateMessage(
    int ssn_id, int uuid, int model_id,
    const std::string& content, const std::string& sender,
    std::string raw )
{
    // // 如果 content 为空，则不创建消息
    // if ( content.empty() )
    //     return -1;

    try
    {
        // 先确定创建的消息是否是该用户已创建会话中的
        // 不等于 0 的是用户发送的消息
        if ( uuid != 0 )
        {
            // 当是用户的信息试图插入时，要检查 raw 中的 prompt 对象 content 字段是否为空
            nlohmann::json json_raw = nlohmann::json::parse( raw );
            if ( json_raw[ "prompt" ][ "content" ].is_null()
                || json_raw[ "prompt" ][ "content" ].get<std::string>().empty() )
                return -1;

            auto sessions_of_user = SelectSessions( uuid );
            if ( std::find_if(
                sessions_of_user.begin(),
                sessions_of_user.end(),
                [ ssn_id ] ( const SessionInfo& info ) { return info.m_id == ssn_id; } )
                == sessions_of_user.end() )
            {
                return -1;
            }
        }
        // 如果是 AI 的回答，则需要处理生成 raw 数据
        else
        {
            // 当是 AI 的回答时，要检查 content 是否为空
            if ( content.empty() )
                return -1;

            nlohmann::json json_prompt;
            json_prompt[ "role" ] = "assistant";
            json_prompt[ "content" ] = content;

            nlohmann::json json_raw;
            json_raw[ "prompt" ] = json_prompt;
            json_raw[ "model_id" ] = model_id;
            json_raw[ "session_id" ] = ssn_id;
            json_raw[ "uuid" ] = uuid;

            raw = json_raw.dump();
        }

        int result = dao.CreateMessage(
            uuid, ssn_id, model_id,
            sender,
            raw );
        switch ( result )
        {
            case 0:
            case -1:
            case -2:
            case -3:
                return -1;

            default:
                return 0;
        }
    }
    catch ( const std::exception& exp )
    {
        SyncLogger::GetInstance()->Log(
            LogLevel::Error,
            "MySqlMgr::CreateMessage处异常: {}",
            exp.what() );
        return -1;
    }
}

std::string MySqlMgr::SelectUserLastLoginTime( int uuid )
{
    return dao.SelectUserUpdatedAt( uuid );
}
