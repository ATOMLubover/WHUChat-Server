#include "cli_http_conn.hpp"

#include "sync_logger.hpp"

#include <iostream>
#include <format>

std::atomic<std::uint32_t> CliHttpConn::serial_cnt = 0;

CliHttpConn::CliHttpConn( net::io_context& ioc, ssl::context& ssl_ctx )
    : m_ioc( ioc )
    , m_stream( std::make_unique<ssl::stream<beast::tcp_stream>>(
        net::make_strand( ioc ), ssl_ctx ) ) // 启用串行化
    , m_resolver( net::make_strand( ioc ) )
    , m_serial_num( serial_cnt.fetch_add( 1 ) )
    , m_host( "nil" ), m_port( "nil" )
    , m_timer_timeout( net::make_strand( ioc ) )
{
    // 注册超时时间
    m_timer_timeout.expires_after( std::chrono::seconds( 10 ) );

    SyncLogger::GetInstance()->Log(
        LogLevel::Debug,
        "CliHttpConn（ID: {} ）构造",
        m_serial_num );
}

CliHttpConn::~CliHttpConn()
{
    SyncLogger::GetInstance()->Log(
        LogLevel::Debug,
        "CliHttpConn（ID: {} ）被析构",
        m_serial_num );
}

void CliHttpConn::AsyncSendTo( const std::string& host, const std::string& port )
{
    auto self = shared_from_this();
    // 异步解析（解析成功会自动链式调用到发送函数）
    m_resolver.async_resolve(
        host,
        port,
        [ self ](
            boost::system::error_code err,
            tcp::resolver::results_type results )
        {
            self->OnResolved( err, results );
        } );
}

void CliHttpConn::SetRequest( http::request<http::string_body>&& req )
{
    m_request = std::move( req );
}

void CliHttpConn::SetRspHandler( CliRspHandler rsp_handler )
{
    m_rsp_handler = rsp_handler;
}

void CliHttpConn::SetTimeoutHandler( CliTimeoutHandler timeout_handler )
{
    m_timeout_handler = timeout_handler;
}

std::string CliHttpConn::ToString() const
{
    return std::format(
        "CliHttpConn({}:{}，ID: {})",
        m_host, m_port, m_serial_num );
}

void CliHttpConn::OnResolved(
    boost::system::error_code& err,
    tcp::resolver::results_type results )
{
    if ( err )
    {
        SyncLogger::GetInstance()->Log(
            LogLevel::Error,
            "{} OnResolved接收到err: {}",
            ToString(), err.message().c_str() );
        return;
    }

    try
    {
        auto self = shared_from_this();
        // 成功解析，则尝试连接
        // 这里调用底层的 tcp 连接
        beast::get_lowest_layer( *m_stream ).async_connect(
            results,
            [ self ](
                boost::system::error_code err,
                const tcp::endpoint& endpoint )
            {
                self->OnConnected( err, endpoint );
            } );
    }
    catch ( std::exception& exp )
    {
        SyncLogger::GetInstance()->Log(
            LogLevel::Error,
            "{} OnResolved处异常: {}",
            ToString(), exp.what() );
        return;
    }
}

void CliHttpConn::OnHandshaked( boost::system::error_code err )
{
    try
    {
        if ( err )
        {
            SyncLogger::GetInstance()->Log(
                LogLevel::Error,
                "{} OnHandshaked接收到err: {}",
                ToString(), err.message() );
            // 握手失败，确保 socket 已关闭
            m_stream->next_layer().close();
            return;
        }

        SyncLogger::GetInstance()->Log(
            LogLevel::Info,
            "{} SSL 握手成功", ToString() );

        auto self = shared_from_this();
        // 握手成功，则尝试发送请求（使用 http 库的 async_write 函数）
        // async_write 操作在 m_stream (beast::http::stream) 上进行，它会利用底层的 ssl::stream
        http::async_write(
            *m_stream,
            m_request,
            net::bind_executor(
                m_stream->get_executor(), // 使用 Stream 的 executor ( strand )
                [ self ](
                    boost::system::error_code err,
                    std::size_t bytes_trans )
                {
                    self->OnWritten( err, bytes_trans );
                } ) );

        // 发送请求后开始计时（也可以在连接成功后就开始计时，取决于超时定义的范围）
        // 如果计时器已经在 OnConnected 启动，这里不需要重复启动
        EnableTimeout();
    }
    catch ( std::exception& exp )
    {
        SyncLogger::GetInstance()->Log(
            LogLevel::Error,
            "{} OnHandshaked处异常: {}",
            ToString(), exp.what() );
        m_stream->next_layer().close();
        m_timer_timeout.cancel();
    }
}

void CliHttpConn::OnConnected(
    boost::system::error_code err,
    const tcp::endpoint& endpoint )
{
    try
    {
        if ( err )
        {
            SyncLogger::GetInstance()->Log(
                LogLevel::Error,
                "{} OnConnected接收到err: {}",
                ToString(), err.message().c_str() );

            // 确保关闭 tcp socket
            m_stream->next_layer().close();

            return;
        }

        m_host = endpoint.address().to_string();
        m_port = std::to_string( endpoint.port() );

        SyncLogger::GetInstance()->Log(
            LogLevel::Info,
            "{} 已连接", ToString() );

        m_stream->async_handshake(
            ssl::stream_base::client,
            [ self = shared_from_this() ] ( const boost::system::error_code& ec )
            {
                self->OnHandshaked( ec );
            } );

        // auto self = shared_from_this();
        // // 成功连接，则尝试发送（使用 http 库的 async_write 函数自动处理）
        // http::async_write(
        //     *m_stream,
        //     m_request,
        //     [ self ](
        //         boost::system::error_code err,
        //         std::size_t bytes_trans )
        //     {
        //         self->OnWritten( err, bytes_trans );
        //     } );
    }
    catch ( std::exception& exp )
    {
        SyncLogger::GetInstance()->Log(
            LogLevel::Error,
            "{} OnConnected处异常: {}",
            ToString(), exp.what() );
        return;
    }
}

void CliHttpConn::OnWritten(
    boost::system::error_code err,
    std::size_t bytes_trans )
{
    // 忽略 EOF 错误
    if ( err )
    {
        SyncLogger::GetInstance()->Log(
            LogLevel::Error,
            "{} OnWritten接受到err: {}",
            ToString(), err.message() );
        return;
    }

    try
    {
        auto self = shared_from_this();
        // 发送成功之后，开始等待服务器的响应（同样使用封装好的 http 库的 async_read 函数）
        http::async_read(
            *m_stream,
            m_rsp_buf,
            m_response,
            [ self ](
                boost::system::error_code err,
                std::size_t bytes_trans )
            {
                self->OnRead( err, bytes_trans );
            } );

        // 同时开始计时
        EnableTimeout();
    }
    catch ( std::exception& exp )
    {
        SyncLogger::GetInstance()->Log(
            LogLevel::Error,
            "{} OnWritten处异常: {}",
            ToString(), exp.what() );
        return;
    }
}

void CliHttpConn::OnRead( boost::system::error_code err, std::size_t bytes_trans )
{
    // 忽略 EOF 错误
    if ( err && err != http::error::end_of_stream )
    {
        SyncLogger::GetInstance()->Log(
            LogLevel::Error,
            "{} OnRead接受到err: {}",
            ToString(), err.message() );
        return;
    }

    try
    {
        // 调用外界注册的回调函数
        if ( m_rsp_handler )
            ( *m_rsp_handler )( std::move( m_response ) );

        // 处理完后，关闭连接
        m_stream->next_layer().socket().shutdown( tcp::socket::shutdown_both, err );
        if ( err )
        {
            SyncLogger::GetInstance()->Log(
                LogLevel::Error,
                "{} OnRead关闭连接时接受到err: {}",
                ToString(), err.message() );
            return;
        }
        m_timer_timeout.cancel();
    }
    catch ( std::exception& exp )
    {
        SyncLogger::GetInstance()->Log(
            LogLevel::Error,
            "{} OnRead处异常: {}",
            ToString(), exp.what() );
        return;
    }
}

void CliHttpConn::EnableTimeout()
{
    auto self = shared_from_this();
    // 异步等待
    m_timer_timeout.async_wait(
        [ self ] ( boost::system::error_code err )
        {
            if ( err )
            {
                SyncLogger::GetInstance()->Log(
                    LogLevel::Error,
                    "{} 超时计时器处理回调接收到err: {}",
                    self->ToString(), err.message() );
                return;
            }

            // 超时了，调用外界注册的回调函数
            try
            {
                if ( self->m_timeout_handler )
                    ( *self->m_timeout_handler )( self );
            }
            catch ( std::exception& exp )
            {
                SyncLogger::GetInstance()->Log(
                    LogLevel::Error,
                    "{} 超时计时器回调处理函数处异常: {}",
                    self->ToString(), exp.what() );
                return;
            }

            // 处理完后，关闭连接
            self->m_stream->next_layer().socket().shutdown( tcp::socket::shutdown_both, err );
            if ( err )
            {
                SyncLogger::GetInstance()->Log(
                    LogLevel::Error,
                    "{} 超时计时器处理回调关闭连接时接受到err: {}",
                    self->ToString(), err.message() );
                return;
            }
        }
    );
}
