#include "svr_wss_mgr.hpp"

#include "svr_wss_conn.hpp"
#include "svr_https_conn.hpp"
#include "svr_wss_pipe.hpp"
#include "cookie_processer.hpp"
#include "mysql_mgr.hpp"
#include "redis_mgr.hpp"
#include "config_mgr.hpp"

#include <json/json.hpp>

#include <iostream>
#include <regex>

SvrWssMgr::~SvrWssMgr()
{
    std::clog << "SvrWssMgr被析构" << std::endl;
}

bool SvrWssMgr::CheckHttpsUpgradable( std::shared_ptr<SvrHttpsConn> conn )
{
    // 如果 request 格式不符合升级要求，返回 false
    if ( !websocket::is_upgrade( *conn->GetRequest() ) )
        return false;

    // 如果是不被支持的 WebSocket 升级接口，返回 false
    auto iter = m_set_upgradable_uri.find( conn->GetUri() );
    if ( iter == m_set_upgradable_uri.end() )
        return false;

    // 如果是管道升级接口，则保证其格式合理，否则返回 false
    if ( *iter == SvrWssPipe::PIPE_URI_OUTPUT
        && !CheckPipeCliFormat( conn ) )
        return false;
    if ( *iter == SvrWssPipe::PIPE_URI_INPUT
        && !CheckPipeSvrFormat( conn ) )
        return false;

    std::cout << "收到Websocket升级请求：(ID: " << conn->GetId() << "), " << conn->GetUri() << std::endl;
    return true;
}

void SvrWssMgr::UpgradeConn( std::shared_ptr<SvrHttpsConn> http_conn )
{
    try
    {
        auto conn = std::make_shared<SvrWssConn>( http_conn );
        // 注入注册和销毁回调
        conn->SetHolder(
            [ self = shared_from_this() ] ( std::shared_ptr<SvrWssConn> conn )
            {
                self->AddConn( conn );
            } );
        conn->SetRemover(
            [ self = shared_from_this() ] ( std::shared_ptr<SvrWssConn> conn )
            {
                self->RmvConn( conn->GetId() );
            } );

        // 检查是否是管道的 WSS 连接
        if ( m_set_pipe_uri.find( conn->GetUri() ) != m_set_pipe_uri.end() )
            TryBuildPipe( conn );

        // 最后 WSS 挥手，开始监听
        conn->DoAccept( http_conn->GetRequest() );
    }
    catch ( std::exception& exp )
    {
        std::cerr << "SvrWssMgr升级HTTPS连接处异常：" << exp.what() << std::endl;
        return;
    }
}

SvrWssMgr::SvrWssMgr()
    : m_set_upgradable_uri( { "/api/v1/ws/trans_ans", "/api/v1/ws/send_ans" } )
    , m_set_pipe_uri( { "/api/v1/ws/trans_ans", "/api/v1/ws/send_ans" } )
{
    std::clog << "SvrWssMgr构造" << std::endl;
}

void SvrWssMgr::AddConn( std::shared_ptr<SvrWssConn> conn )
{
    std::lock_guard<std::mutex> lock( m_mtx_map_conn );

    m_map_conn.emplace( conn->GetId(), conn );
    std::clog << "SvrWssMgr已保存连接：" << conn->GetId() << std::endl;
}

void SvrWssMgr::RmvConn( int uuid )
{
    try
    {
        auto iter = m_map_conn.find( uuid );
        if ( iter == m_map_conn.end() )
            return;

        // 如果要删除的链接属于管道，则防止管道悬空
        if ( m_set_pipe_uri.find( iter->second->GetUri() ) != m_set_pipe_uri.end() )
            TryDelPipe( iter->second );

        // 最后删除连接
        iter->second->Close();
        m_map_conn.erase( iter );
    }
    catch ( std::exception& exp )
    {
        std::clog << "SvrWssMgr RmvConn函数处异常：" << exp.what() << std::endl;
        return;
    }

    std::clog << "SvrWssMgr已解除保存：" << uuid << std::endl;
}

bool SvrWssMgr::CheckCookieWithUuid( std::shared_ptr<SvrHttpsConn> conn, int uuid )
{
    try
    {    // uuid == 0 时 ApiServer 的请求
        if ( uuid == 0 )
            return true;

        auto req = *conn->GetRequest();

        // 如果不存在 Cookie 字段，则直接返回 false
        auto iter_cookie = req.find( "Cookie" );
        if ( iter_cookie == req.end() )
            return false;

        // 反序列化 cookie，检查其中的 uuid 是否匹配
        // uuid == -1 则跳过后面的检查直接返回 true
        auto cookies
            = CookieProcesser::Parse( iter_cookie->value() );
        if ( uuid == -1 )
            return true;

        auto iter_uuid = cookies.find( "uuid" );
        if ( iter_uuid == cookies.end()
            || iter_uuid->second != std::to_string( uuid ) )
            return false;

        // 如果 token 失效/不存在，也返回 false
        if ( !RedisMgr::GetInstance()->QueryChatServerToken( uuid, cookies[ "token" ] ) )
            return false;
    }
    catch ( std::exception& exp )
    {
        std::cerr << "CheckCookieWithUuid处发生异常：" << exp.what() << std::endl;
        // 发生任何解析错误都返回 false
        return false;
    }

    return true;
}

bool SvrWssMgr::CheckPipeCliFormat( std::shared_ptr<SvrHttpsConn> conn )
{
    try
    {
        auto params = conn->GetParamsOfGet();
        // 如果不存在 uuid 则返回 false
        auto iter_uuid = params.find( "uuid" );
        if ( iter_uuid == params.end() )
            return false;

        // 检查 cookie
        if ( !CheckCookieWithUuid( conn, std::stoi( iter_uuid->second ) ) )
            return false;

        // 不存在 seesion_id 则返回 false
        auto iter_ssn_id = params.find( "session_id" );
        if ( iter_ssn_id == params.end() )
            return false;

        // 如果当前 session_id 不是这个用户的，则返回 false
        auto sessions = MySqlMgr::GetInstance()->SelectSessions(
            std::stoi( iter_uuid->second ) );
        if ( std::find_if(
            sessions.begin(),
            sessions.end(),
            [ & ] ( const SessionInfo& session )
            {
                return session.m_id == std::stoi( iter_ssn_id->second );
            } )
            == sessions.end() )
        {
            return false;
        }
    }
    catch ( std::exception& exp )
    {
        std::cout << "WebsockMgr检查客户端升级参数处异常：" << exp.what() << std::endl;
        return false;
    }

    return true;
}

bool SvrWssMgr::CheckPipeSvrFormat( std::shared_ptr<SvrHttpsConn> conn )
{
    // 定义正则表达式，匹配参数字符串应该要有 token 和 session_id
    std::regex pattern( R"((?:session_id=(\d+)&uuid=(\d+)|uuid=(\d+)&session_id=(\d+)))" );
    std::string raw_params = conn->GetRawParamsOfGet();
    if ( !std::regex_match( raw_params, pattern ) )
        return false;

    try
    {
        auto params = conn->GetParamsOfGet();

        // 获取参数
        std::string token = params[ "token" ];
        int session_id = std::stoi( params[ "session_id" ] );

        // 确定是否存在对应会话
        if ( !MySqlMgr::GetInstance()->CheckSessionExisting( session_id ) )
            return false;
    }
    catch ( std::exception& exp )
    {
        std::cout << "WebsockMgr检查ApiServer端升级参数处异常：" << exp.what() << std::endl;
        return false;
    }

    return true;
}

void SvrWssMgr::TryBuildPipe( std::shared_ptr<SvrWssConn> conn )
{
    auto iter = conn->GetParGet().find( "session_id" );
    if ( iter == conn->GetParGet().end() )
        return;

    try
    {
        // 由于 WebsockConn 的建立是客户端自行决定时机的
        // 无法确定 WebsockMsgPipe 先有谁建立，故先使用 session_id 验证存在性
        int session_id = std::stoi( iter->second );
        int uuid = std::stoi( conn->GetParGet()[ "uuid" ] );

        // 防止多次建立同一个管道，锁定
        std::lock_guard<std::mutex> guard( m_mtx_pipe );

        auto iter = m_map_pipe.find( session_id );
        // 如果不存在先前的，则先创建一个
        if ( iter == m_map_pipe.end() )
        {
            iter = m_map_pipe.insert( std::make_pair(
                session_id,
                std::make_shared<SvrWssPipe>( session_id ) ) )
                .first;
            std::cout << "WebsockMsgPipe（session_id：" << session_id << "）"
                "被保留" << std::endl;
        }

        // 然后再检查来源
        // 如果来自 ApiServer，则绑定 input
        if ( conn->GetUri() == SvrWssPipe::PIPE_URI_INPUT )
        {
            iter->second->BindInput( conn );
            std::cout << "WebsockMsgPipe（session_id：" << session_id << "）"
                "已接入input" << std::endl;
            return;
        }
        // 如果来自 web client，则绑定 output
        if ( conn->GetUri() == SvrWssPipe::PIPE_URI_OUTPUT )
        {
            iter->second->BindOutput( conn );
            std::cout << "WebsockMsgPipe（session_id：" << session_id << "）"
                "已接入output" << std::endl;
            return;
        }
    }
    catch ( std::exception& exp )
    {
        std::cout << "SvrWssMgr TryBuildPipe函数处异常：" << exp.what() << std::endl;
        return;
    }
}

void SvrWssMgr::TryDelPipe( std::shared_ptr<SvrWssConn> conn )
{
    int session_id = std::stoi( conn->GetParGet()[ "session_id" ] );

    std::lock_guard<std::mutex> guard( m_mtx_pipe );

    auto iter = m_map_pipe.find( session_id );
    if ( iter != m_map_pipe.end() )
    {
        // 删除对应管道中的 input 或者 output
        auto pipe = iter->second;
        if ( pipe->GetInput() == conn )
            pipe->UnbindInput();
        else if ( pipe->GetOutput() == conn )
            pipe->UnbindOutput();

        // 最后检查是否管道已经空了，为空则删除
        if ( pipe->IsUnloaded() )
            m_map_pipe.erase( iter );
    }
}
