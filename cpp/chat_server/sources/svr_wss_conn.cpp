#include "svr_wss_conn.hpp"

#include "svr_https_conn.hpp"
#include "url_processer.hpp"

#include <fmt/format.h>

#include <iostream>

std::atomic<int> SvrWssConn::total_cnt = 0;

// 应当指出，这里 https_conn 的底层 tcp::socket 已经绑定在 starnd 的驱动器上了
// 理论上来说应该是会自动序列化，不需要额外的同步操作
SvrWssConn::SvrWssConn( std::shared_ptr<SvrHttpsConn> https_conn )
    : m_wss_stream( std::move( *https_conn->ReleaseSslStream() ) )
    , m_id( total_cnt.fetch_add( 1 ) )
    , m_uri( https_conn->GetUri() )
    , m_par_get( https_conn->GetParamsOfGet() )
{
    m_wss_stream.set_option(
        websocket::stream_base::timeout::suggested(
            beast::role_type::server ) );

    std::clog << fmt::format( "SvrWssConn(ID: {})构造", m_id ) << std::endl;
}

SvrWssConn::~SvrWssConn()
{
    // Close();

    std::clog << fmt::format( "SvrWssConn(ID: {})被析构", m_id ) << std::endl;
}

void SvrWssConn::DoAccept( std::shared_ptr<http::request<http::dynamic_body>> req )
{
    // 这里在回调中复制 req，防止异步挥手时 req 被析构
    try
    {
        beast::error_code ec;
        m_wss_stream.accept( *req, ec );

        OnAccept( ec, req );
    }
    catch ( std::exception& exp )
    {
        std::cerr << fmt::format(
            "SvrWssConn(ID: {}) DoAccept函数中出现异常：{}\n",
            m_id, exp.what() );
        return;
    }
}

void SvrWssConn::Close()
{
    if ( !m_wss_stream.is_open() )
        return;

    // 先通过 websocket 发送关闭帧
    beast::error_code ec;
    m_wss_stream.close( websocket::close_code::normal, ec );
    if ( ec && ec != boost::asio::error::operation_aborted )
        std::cerr << "SvrWssConn关闭WSS时接受到错误码：" << ec.message() << std::endl;

    // 在 websocket 确认了关闭帧以后，再关闭 SSL
    m_wss_stream.next_layer().shutdown( ec );
    if ( ec )
        std::cerr << "SvrWssConn关闭SSL时接受到错误码：" << ec.message() << std::endl;

    // 在 SSL 流关闭后，再强制关闭 TCP 连接（忽略错误）
    m_wss_stream.next_layer().next_layer().close();
}

void SvrWssConn::OnAccept( beast::error_code ec, HttpsReq req )
{
    if ( ec )
    {
        std::cerr << "SvrWssConn OnHandshake函数接受到错误码：" << ec.message() << std::endl;
        return;
    }

    try
    {
        // 处理 accept 的 request
        UrlProcesser::ParseGet( req->target(), &m_uri, nullptr, &m_par_get );

        // 没有异常时，调用保存函数缓存自身，并开始异步等待读入
        if ( m_holder )
            m_holder( shared_from_this() );

        DoRead();
    }
    catch ( std::exception& exp )
    {
        std::cerr << "SvrWssConn DoRead函数中出现异常：" << exp.what() << std::endl;
        return;
    }
}

void SvrWssConn::DoRead()
{
    m_wss_stream.async_read(
        m_buf_recv,
        [ self = shared_from_this() ] ( beast::error_code ec, std::size_t bytes_trans )
        {
            self->OnRead( ec );
        } );
}

void SvrWssConn::OnRead( beast::error_code ec )
{
    if ( ec )
    {
        std::cerr << "SvrWssConn OnRead函数接受到错误码：" << ec.message() << std::endl;
        if ( m_remover )
            m_remover( shared_from_this() );
        return;
    }

    try
    {
        if ( !m_read_handler )
        {
            std::clog << fmt::format(
                "SvrWssConn(ID: {})已跳过m_read_handler", m_id ) << std::endl;

            // 再开启一次 DoRead
            DoRead();
            return;
        }

        // 如果处理失败，则会不在继续读取
        if ( !m_read_handler( shared_from_this() ) )
        {
            std::cerr << fmt::format(
                "SvrWssConn(ID: {})m_read_handler处理失败", m_id ) << std::endl;
            if ( m_remover )
                m_remover( shared_from_this() );
            return;
        }

        // 再开启一次 DoRead
        DoRead();
    }
    catch ( std::exception& exp )
    {
        std::cerr << fmt::format(
            "SvrWssConn(ID: {}) OnRead函数中出现异常：", m_id )
            << exp.what() << std::endl;
        if ( m_remover )
            m_remover( shared_from_this() );
        return;
    }
}

void SvrWssConn::DoSend( std::string msg )
{
    {
        std::lock_guard<std::mutex> lock( m_mtx_quewait );

        // 将 msg 先加入等待队列，倘若不止它在等待，则不需要他开启写入发送
        m_que_wait.push( msg );
        if ( m_que_wait.size() > 1 )
            return;
    }

    // 如果只有 msg 在等待，则开启一次新的发送
    DoWrite( std::move( msg ) );
}

void SvrWssConn::DoWrite( std::string msg )
{
    m_wss_stream.async_write(
        net::buffer( msg ),
        [ self = shared_from_this(), msg ]
        ( beast::error_code ec, std::size_t bytes_trans )
        {
            self->OnWrite( ec, std::move( msg ) );
        } );
}

void SvrWssConn::OnWrite( beast::error_code ec, std::string msg )
{
    if ( ec )
    {
        std::cerr << "SvrWssConn OnWrite函数接受到错误码：" << ec.message() << std::endl;
        if ( m_remover )
            m_remover( shared_from_this() );
        return;
    }
    // std::clog << fmt::format(
    //     "SvrWssConn(ID: {})OnWrite成功发送：{}", m_id, msg ) << std::endl;

    try
    {
        std::string next_msg{};
        {
            std::lock_guard<std::mutex> lock( m_mtx_quewait );

            // 先弹出上一个已经被 DoWrite 发送的消息
            m_que_wait.pop();
            if ( m_que_wait.empty() )
                return;

            next_msg = std::move( m_que_wait.front() );
        }

        // 如果还有消息，继续发送链
        DoWrite( std::move( next_msg ) );
    }
    catch ( std::exception& exp )
    {
        std::cerr << "SvrWssConn OnWrite函数中出现异常：" << exp.what() << std::endl;
        if ( m_remover )
            m_remover( shared_from_this() );
        return;
    }
}
