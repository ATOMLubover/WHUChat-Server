#include "websock_mgr.hpp"

#include "websock_conn.hpp"
#include "http_conn.hpp"
#include "websock_msg_pipe.hpp"

#include <iostream>
#include <regex>

WebsockMgr::~WebsockMgr()
{
    std::cout << "WebsockMgr被析构" << std::endl;
}

void WebsockMgr::AddConn( std::shared_ptr<WebsockConn> conn )
{
    std::lock_guard<std::mutex> guard( mtx_mapconn );

    map_conn.insert( std::make_pair( conn->GetUid(), conn ) );
    std::cout << "WebsockConn No." << conn->GetUid() << " 被保留" << std::endl;
}

void WebsockMgr::RmvConn( const std::string& uuid )
{
    std::lock_guard<std::mutex> guard( mtx_mapconn );

    auto iter = map_conn.find( uuid );
    if ( iter != map_conn.end() )
    {
        // 如果删除的链接是属于管道的，则检查删除这个链接会不会导致有的管道悬空
        if ( iter->second->mission == WebsockMsgPipe::PIPE_URI )
            TryDelPipe( iter->second );

        // 先关闭连接
        iter->second->Close();
        map_conn.erase( iter );

        std::cout << "WebsockConn No." << uuid << " 停止保留" << std::endl;
    }
}

bool WebsockMgr::CheckValidUpgrade( std::shared_ptr<HttpConn> http_conn )
{
    // 如果 request 格式不符合升级要求，返回 false
    if ( !websocket::is_upgrade( http_conn->GetRequest() ) )
        return false;

    // 如果是不被支持的 WebSocket 升级接口，返回 false
    auto iter = set_uri.find( http_conn->GetUri() );
    if ( iter == set_uri.end() )
        return false;

    // 如果是管道升级接口，则保证其格式合理，否则返回 false
    if ( *iter == WebsockMsgPipe::PIPE_URI
        && !CheckPipeFormat( http_conn->GetRawParams() ) )
        return false;

    std::cout << "收到Websocket升级请求："
        << http_conn->GetUri() << "?" << http_conn->GetRawParams() << std::endl;
    return true;
}

std::shared_ptr<WebsockConn> WebsockMgr::CreateConn( std::shared_ptr<HttpConn> http_conn )
{
    std::shared_ptr<WebsockConn> ws_conn;
    try
    {
        // 接管 HttpConn 的 socket
        ws_conn = std::make_shared<WebsockConn>( http_conn );
        // 进行协议升级
        ws_conn->SyncAccept( http_conn->GetRequest() );

        // 检查是否需要建立管道
        TryBuildPipe( ws_conn );
    }
    catch ( std::exception& exp )
    {
        std::cout << "WebsockMgr CreateConn函数中出现异常：" << exp.what() << std::endl;
        return std::shared_ptr<WebsockConn>();
    }

    return ws_conn;
}

WebsockMgr::WebsockMgr()
{
    set_uri = { "/trans_ans" };

    std::cout << "WebsockMgr构造" << std::endl;
}

void WebsockMgr::TryBuildPipe( std::shared_ptr<WebsockConn> conn )
{
    std::string uri = WebsockMsgPipe::PIPE_URI;

    if ( conn->mission != uri )
        return;

    try
    {
        // 由于 WebsockConn 的建立是客户端自行决定时机的
        // 无法确定 WebsockMsgPipe 先有谁建立，故先使用 session_id 验证存在性
        int session_id = std::stoi( conn->get_params[ "session_id" ] );

        // 防止多次建立同一个管道，锁定
        std::lock_guard<std::mutex> guard( mtx_mappipe );

        auto iter = map_pipe.find( session_id );
        // 如果不存在先前的，则先创建一个
        if ( iter == map_pipe.end() )
        {
            iter = map_pipe.insert( std::make_pair(
                session_id,
                std::make_shared<WebsockMsgPipe>( session_id ) ) )
                .first;
            std::cout << "WebsockMsgPipe（session_id：" << session_id << "）"
                "被保留" << std::endl;
        }

        // 然后再检查来源
        // 如果来自 ApiServer，则绑定 input
        // TODO: 可能要防止重复连接
        if ( conn->get_params[ "from" ] == "api_server" )
        {
            iter->second->BindInput( conn );
            std::cout << "WebsockMsgPipe（session_id：" << session_id << "）"
                "已接入input" << std::endl;
            return;
        }
        // 如果来自 web client，则绑定 output
        if ( conn->get_params[ "from" ] == "client" )
        {
            iter->second->BindOutput( conn );
            std::cout << "WebsockMsgPipe（session_id：" << session_id << "）"
                "已接入output" << std::endl;
            return;
        }
    }
    catch ( std::exception& exp )
    {
        std::cout << "WebsockMgr TryBuildPipe函数处异常：" << exp.what() << std::endl;
        return;
    }
}

bool WebsockMgr::CheckPipeFormat( const std::string& get_raw_params )
{
    // 定义正则表达式，匹配参数字符串应该要有 from 和 session_id
    std::regex pattern( R"(^from=(api_server|client)&session_id=(\d+)$)" );
    return std::regex_match( get_raw_params, pattern );
}

void WebsockMgr::TryDelPipe( std::shared_ptr<WebsockConn> conn )
{
    int session_id = std::stoi( conn->get_params[ "session_id" ] );

    std::lock_guard<std::mutex> guard( mtx_mappipe );

    auto iter = map_pipe.find( session_id );
    if ( iter != map_pipe.end() )
    {
        // 删除对应管道中的 input 或者 output
        auto pipe = iter->second;
        if ( pipe->GetInput() == conn )
            pipe->UnloadInput();
        else if ( pipe->GetOutput() == conn )
            pipe->UnloadOutput();

        // 最后检查是否管道已经空了，为空则删除
        if ( pipe->IsUnloaded() )
            map_pipe.erase( iter );
    }
}
