#include "websock_conn.hpp"

#include "websock_mgr.hpp"

#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/beast/websocket.hpp>

WebsockConn::WebsockConn( tcp::socket& socket )
{
    // 在原 socket 基础上添加 strand
    {
        // 创建 strand
        auto strand = net::make_strand( socket.get_executor() );

        // 释放原始 socket 的句柄
        auto native_handle = socket.release();
        // 建立新 socket
        tcp::socket new_socket( strand, tcp::v4(), native_handle );
        // 将新 socket 转移到 tcp_stream
        beast::tcp_stream tcp_stream( std::move( new_socket ) );

        // 最后构造 WebSocket Stream
        websock.reset( new Socket( std::move( tcp_stream ) ) );
    }

    // 生成对应的 uuid
    {
        boost::uuids::random_generator randgen;
        boost::uuids::uuid tmp_uuid = randgen();
        // 获得一个 uuid
        uuid = boost::uuids::to_string( tmp_uuid );
    }

    std::cout << "WebsockConn构造：" << uuid << std::endl;
}

WebsockConn::~WebsockConn()
{
    std::cout << "WebsockConn被析构：" << uuid << std::endl;
}

tcp::socket& WebsockConn::GetSocket()
{
    return beast::get_lowest_layer( *websock ).socket();
}

void WebsockConn::AsyncRead()
{
    auto self = shared_from_this();
    websock->async_read(
        buf_recv,
        [ self ] ( beast::error_code err, std::size_t bytes )
        {
            if ( err )
            {
                std::cout << "WebsockConn异步read出现错误：" << err.message() << std::endl;
                // 删除 WebsockMgr 中持有的复制的 shard_ptr
                WebsockMgr::GetInstance()->RmvConn( self->GetUid() );

                return;
            }

            try
            {
                // 让发送的内容与接受到的内容类型相同
                self->websock->text( self->websock->got_text() );
                std::string data_recv = beast::buffers_to_string( self->buf_recv.data() );
                // 清空缓冲区，以供下次使用
                self->buf_recv.consume( self->buf_recv.size() );
                std::cout << "WebsockConn接收到数据：" << data_recv << std::endl;

                // 发送数据
                self->AsyncSend( std::move( data_recv ) );

                // 该次回调函数结束后在启用一个回调
                self->AsyncRead();
            }
            catch ( std::exception& exp )
            {
                std::cout << "WebsockConn AsyncRead处异常：" << exp.what() << std::endl;
                WebsockMgr::GetInstance()->RmvConn( self->GetUid() );
                return;
            }
        } );
}

void WebsockConn::SyncAccept( http::request<http::string_body>& request )
{
    // try
    // {
    //     websock->accept( request );
    // }
    // catch ( const std::exception& e )
    // {
    //     std::cout << "WebsockConn同步accept升级协议出错误：" << e.what() << std::endl;
    //     return;
    // }

    // // 如果一切正常则开始准备读取
    // WebsockMgr::GetInstance()->AddConn( shared_from_this() );
    // AsyncRead();

    auto self = shared_from_this();
    websock->async_accept(
        request,
        [ self ] ( beast::error_code err )
        {
            if ( err )
            {
                std::cout << "WebsockConn异步accept出现错误：" << err.message() << std::endl;
                return;
            }

            WebsockMgr::GetInstance()->AddConn( self );
            // 若未发生问题，则开始读入
            self->AsyncRead();
        } );
}

void WebsockConn::AsyncSend( std::string msg )
{
    {
        // 锁定 que_msg 消息队列
        std::lock_guard<std::mutex> guard( mtx_quemsg );

        std::size_t len_que = que_msg.size();
        que_msg.push( msg );
        if ( len_que > 0 )
            return;
    }

    AsyncWrite( std::move( msg ) );
}

void WebsockConn::AsyncWrite( std::string msg )
{
    auto self = shared_from_this();
    websock->async_write(
        net::buffer( msg.c_str(), msg.size() ),
        [ self ] ( beast::error_code err, std::size_t bytes )
        {
            if ( err )
            {
                std::cout << "WebsockConn异步write出现错误：" << err.message() << std::endl;
                // 删除 WebsockMgr 中持有的复制的 shard_ptr
                WebsockMgr::GetInstance()->RmvConn( self->GetUid() );

                return;
            }

            try
            {
                std::string msg_to_send;
                {
                    std::lock_guard<std::mutex> guard( self->mtx_quemsg );

                    self->que_msg.pop(); // 将上次发送的消息弹出
                    if ( self->que_msg.empty() )
                        return;

                    // 如果还有没发送的消息
                    msg_to_send = self->que_msg.front();
                }

                self->AsyncWrite( std::move( msg_to_send ) );
            }
            catch ( std::exception& exp )
            {
                std::cout << "WebsockConn AsyncWrite处异常：" << exp.what() << std::endl;
                WebsockMgr::GetInstance()->RmvConn( self->GetUid() );
                return;
            }
        } );
}
