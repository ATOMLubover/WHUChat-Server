#include "https_listener.hpp"

#include "svr_https_conn.hpp"
#include "asio_iocontext_pool.hpp"

#include <iostream>

HttpsListener::HttpsListener(
    net::io_context& ioc, ssl::context& ssl_ctx,
    tcp::endpoint endpoint )
    : m_acceptor( net::make_strand( ioc ), endpoint )
    , m_ssl_ctx( ssl_ctx )
    , m_strand( net::make_strand( ioc ) )
{
    // m_acceptor.open( endpoint.protocol() );
    // m_acceptor.set_option( net::socket_base::reuse_address( true ) );
    // m_acceptor.bind( endpoint );
    // m_acceptor.listen( net::socket_base::max_listen_connections );

    std::clog << "HttpsListener构造，监听在: " << endpoint << std::endl;
}

void HttpsListener::Run()
{
    DoAccept();
}

void HttpsListener::Stop()
{
    // 将关闭时间注册到 acceptor 的 strand 上
    if ( m_acceptor.is_open() )
        m_acceptor.close();

    // net::dispatch(
    //     m_acceptor.get_executor(),
    //     [ self = shared_from_this() ] ()
    //     {
    //         if ( self->m_acceptor.is_open() )
    //             self->m_acceptor.close();

    //         std::clog << "HttpsListener停止监听" << std::endl;
    //     } );
}

void HttpsListener::SetAcceptHandler( AcceptHandlerType handler )
{
    m_accept_handler = std::move( handler );
}

void HttpsListener::DoAccept()
{
    auto& ioc
        = AsioIoContextPool::GetInstance()->GetIoService();
    std::shared_ptr<tcp::socket> socket
        = std::make_shared<tcp::socket>( net::make_strand( ioc ) );
    m_acceptor.async_accept(
        *socket,
        [ self = shared_from_this(), socket ]
        ( const boost::system::error_code& ec )
        {
            // 这里生成的 socket 已经自动绑定在上述 strand 上
            self->OnAccept( ec, std::move( *socket ) );
        } );
}

void HttpsListener::OnAccept( boost::system::error_code ec, tcp::socket socket )
{
    if ( ec == net::error::operation_aborted )
    {
        std::cerr << "HttpsListener结束accept: " << ec.message() << std::endl;
        return;
    }

    // 如果有错误，则打印错误
    if ( ec )
    {
        std::cerr << "HttpsListener OnAccept接受到错误: " << ec.message() << std::endl;
    }
    // 如果没有错误，则调用 SvrHttpsMgr 注入的函数创建一个新的连接
    else
    {
        if ( m_accept_handler )
            m_accept_handler( std::move( socket ), m_ssl_ctx );
    }

    DoAccept();
}

