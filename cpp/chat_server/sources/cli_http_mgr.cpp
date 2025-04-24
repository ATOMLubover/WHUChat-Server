#include "cli_http_mgr.hpp"

#include "asio_iocontext_pool.hpp"
#include "sync_logger.hpp"

CliHttpMgr::~CliHttpMgr()
{
    SyncLogger::GetInstance()->Log(
        LogLevel::Info,
        "{}被析构", ToString() );
}

CliHttpMgr::CliHttpMgr()
{
    SyncLogger::GetInstance()->Log(
        LogLevel::Info,
        "{}构造", ToString() );
}

std::string CliHttpMgr::ToString() const
{
    return "CliHttpMgr";
}

void CliHttpMgr::Init( ssl::context* ssl_ctx )
{
    m_ssl_ctx = ssl_ctx;
}

std::shared_ptr<CliHttpConn> CliHttpMgr::CreateConn()
{
    auto& ioc
        = AsioIoContextPool::GetInstance()->GetIoService();
    return std::make_shared<CliHttpConn>( ioc, *m_ssl_ctx );
}

void CliHttpMgr::AsyncRequest(
    const std::string& host, const std::string& port,
    http::request<http::string_body>&& req,
    CliRspHandler rsp_handler,
    CliTimeoutHandler timeout_handler )
{
    auto conn = CreateConn();

    // 绑定回调，设定请求，最后发送
    conn->SetRspHandler( std::move( rsp_handler ) );
    conn->SetTimeoutHandler( std::move( timeout_handler ) );
    conn->SetRequest( std::move( req ) );
    conn->AsyncSendTo( host, port );

    SyncLogger::GetInstance()->Log(
        LogLevel::Info,
        "CliHttpMgr启动发送请求向: {}:{}",
        host, port );
}
