#include "svr_https_conn.hpp"

#include "url_processer.hpp"
#include "config_mgr.hpp"

#include <fmt/format.h>

#include <iostream>

std::atomic<int> SvrHttpsConn::total_cnt = 0;

SvrHttpsConn::SvrHttpsConn( tcp::socket&& socket, ssl::context& ctx )
    : m_ssl_stream( std::make_unique<ssl::stream<tcp::socket>>( std::move( socket ), ctx ) )
    , m_id( total_cnt.fetch_add( 1 ) )
    , m_is_delay( false )
    // , m_is_ssl_handshaked( false )
{
    std::clog << fmt::format( "SvrHttpsConn(ID: {})构造", m_id ) << std::endl;
}

SvrHttpsConn::~SvrHttpsConn()
{
    if ( m_ssl_stream )
    {
        // if ( m_is_ssl_handshaked )
        //     m_ssl_stream->shutdown();
        m_ssl_stream->lowest_layer().close();
    }

    std::clog << fmt::format( "SvrHttpsConn(ID: {})被析构", m_id ) << std::endl;
}

void SvrHttpsConn::SetLogicGetter( HttpsLogicGetter getter )
{
    m_logic_getter = std::move( getter );
}

void SvrHttpsConn::SetReadHandler( HttpsReadHandler handler )
{
    m_read_handler = handler;
}

void SvrHttpsConn::DoHandshake()
{
    m_ssl_stream->async_handshake(
        ssl::stream_base::server, // 以服务器身份进行挥手
        [ self = shared_from_this() ] ( beast::error_code ec )
        {
            self->OnHandShake( ec );
        } );
}

std::unique_ptr<ssl::stream<tcp::socket>> SvrHttpsConn::ReleaseSslStream()
{
    std::unique_ptr<ssl::stream<tcp::socket>> ori( m_ssl_stream.release() );
    m_ssl_stream.reset( nullptr );
    std::clog << fmt::format( "SvrHttpsConn(ID: {})转让SSL流\n", m_id ) << std::endl;
    return ori;
}

void SvrHttpsConn::OnHandShake( beast::error_code ec )
{
    if ( ec )
    {
        std::clog << fmt::format(
            "SvrHttpsConn(ID: {})的SSL握手接受到错误码: {}",
            m_id, ec.message() ) << std::endl;
        return;
    }

    // m_is_ssl_handshaked = true;
    // 如果显示的挥手成功，则开始读取 HTTPS 请求
    DoRead();
}

void SvrHttpsConn::DoRead()
{
    // 为了支持 chunked 传输，使用 multi_buffer 作为接收缓冲区
    auto buf_recv
        = std::make_shared<beast::multi_buffer>();
    // 为 m_req 分配内存
    m_req = std::make_shared<http::request<http::dynamic_body>>();
    http::async_read(
        *m_ssl_stream,
        *buf_recv,
        *m_req,
        [ self = shared_from_this(), buf_recv ] // 防止异步操作被销毁
        ( beast::error_code ec, std::size_t )
        {
            self->OnRead( ec );
        } );
}

void SvrHttpsConn::OnRead( beast::error_code ec )
{
    if ( ec )
    {
        std::cerr << fmt::format(
            "SvrHttpsConn(ID: {})读取请求接受到错误码: {}",
            m_id, ec.message() ) << std::endl;
        return;
    }

    std::clog << fmt::format( "SvrHttpsConn(ID: {})读取请求成功于：{}\n",
        m_id, m_req->target() );

    // 如果没有获得 logic 的途径，则该 URL 无法被处理
    if ( !m_logic_getter )
    {
        // 这可能是设计错误
        if ( m_ssl_stream != nullptr )
            ResponseFailure( m_req, http::status::internal_server_error, "URI invalid" );
        return;
    }

    // 先预处理 request，得到重要数据
    Prepare();
    // 延迟到此时（得到 URI 后）获取逻辑
    // 在这里可能被升级为 WSS 连接
    // 如果此时升级为 WSS 连接了之后，则不会获得 HTTPS 的逻辑函数，会得到 false
    if ( !m_logic_getter( shared_from_this() ) )
    {
        if ( m_ssl_stream != nullptr )
            ResponseFailure( m_req, http::status::not_found, "URI invalid" );
        return;
    }

    // 如果不存在 handler，说明该 URL 是无效的
    // 返回 404
    if ( !m_read_handler )
    {
        ResponseFailure( m_req, http::status::not_found, "URI invalid" );
        return;
    }

    // 先执行注入执行逻辑
    auto res = ( *m_read_handler )( shared_from_this() );
    // 如果是延迟发送，则跳过下面的发送过程
    if ( m_is_delay )
        return;

    // 然后根据返回值的类型决定如何发送
    std::visit(
        [ self = shared_from_this() ] ( auto&& res )
        {
            using BodyType = std::decay_t<decltype( res )>;

            // 假设 res_body 是一个 std::string
            if constexpr ( std::is_same_v<BodyType,
                std::shared_ptr<http::response<http::string_body>>> )
            {
                res->set( http::field::server, "ChatServer" );
                // 发送 string_body 的函数
                self->DoWrite( std::move( *res ) );
            }
            else
            {
                res->set( http::field::server, "ChatServer" );
                // 发送 file_body 或者 dynamic_body 的响应
                self->DoWrite( std::move( *res ) );
            }
        },
        res );
}

void SvrHttpsConn::DoWrite( auto&& response )
{
    // 发送之前检查是否需要设置 Content-Length
    // 没有设置，且是 string_body 的话，则设置
    if ( ( response.find( http::field::content_length ) == response.end() )
        && std::is_same_v<std::decay_t<decltype( response )>, http::response<http::string_body>> )
    {
        response.content_length( response.body().size() );
    }

    // 使用完美转发构造 shared_ptr
    auto res_ptr = std::make_shared<std::decay_t<decltype( response )>>(
        std::forward<decltype( response )>( response ) );
    http::async_write(
        *m_ssl_stream,
        *res_ptr,
        [ self = shared_from_this(), res_ptr ] ( beast::error_code ec, std::size_t bytes_trans )
        {
            self->OnWrite( ec );
        } );
}

void SvrHttpsConn::OnWrite( beast::error_code ec )
{
    if ( ec )
    {
        std::cerr << fmt::format(
            "SvrHttpsConn(ID: {})写入响应接受到错误码: {}",
            m_id, ec.message() ) << std::endl;
        return;
    }
}

void SvrHttpsConn::Prepare()
{
    // 如果是 GET 请求，则解析出 URI 和参数
    if ( m_req->method() == http::verb::get )
        UrlProcesser::ParseGet(
            m_req->target(),
            &m_uri, &m_raw_params, &m_par_get );
    // 其他请求则直接用 target 赋值 uri
    else
        m_uri = UrlProcesser::UrlDecode( m_req->target() );
}

void SvrHttpsConn::ResponseFailure(
    const HttpsReq& req,
    http::status status, std::string body )
{
    http::response<http::string_body> res{};
    res.version( req->version() );
    res.result( status );
    res.set( http::field::server, "ChatServer" );
    res.set( http::field::content_type, "text/html" );
    res.body() = body;

    // 跨域处理
    std::string addr_gate_server
        = ConfigMgr::GetInstance()[ "gate_server_local" ][ "host" ]
        + ":" + ConfigMgr::GetInstance()[ "gate_server_local" ][ "port" ];
    res.set( http::field::access_control_allow_origin, addr_gate_server );
    res.set( http::field::access_control_allow_methods, "GET, POST, DEL, OPTIONS" );
    res.set( http::field::access_control_allow_headers, "Content-Type, Accept, Authorization" );
    // 允许携带凭证
    res.set( http::field::access_control_allow_credentials, "true" );

    DoWrite( std::move( res ) );
}
