#include "websock_conn.hpp"

#include "http_conn.hpp"
#include "websock_mgr.hpp"
#include "http_logic_system.hpp"

int WebsockConn::total_count = 0;
std::mutex WebsockConn::mtx_ttlcnt; // 分配空间...

WebsockConn::WebsockConn( std::shared_ptr<HttpConn> ori_conn )
{
    // 复制原 HttpConn 的参数
    get_params = ori_conn->GetParams();
    // 确定任务
    mission = ori_conn->GetUri();

    // 在原 socket 基础上添加 strand
    {
        // 创建 strand
        auto strand = net::make_strand( ori_conn->GetSocket().get_executor() );

        // 释放原始 socket 的句柄
        auto native_handle = ori_conn->GetSocket().release();
        // 建立新 socket
        tcp::socket new_socket( strand, tcp::v4(), native_handle );
        // 将新 socket 转移到 tcp_stream
        beast::tcp_stream tcp_stream( std::move( new_socket ) );

        // 最后构造 WebSocket Stream
        websock.reset( new Socket( std::move( tcp_stream ) ) );
    }

    // 生成对应的 uuid
    {
        std::lock_guard<std::mutex> guard( WebsockConn::mtx_ttlcnt );
        uuid = std::to_string( total_count++ );
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

void WebsockConn::Close()
{
    auto self = shared_from_this();
    // 先通过 WebSocket 协议发送关闭帧
    websock->async_close(
        websocket::close_code::normal,
        [ self ] ( boost::system::error_code err )
        {
            if ( err && err != boost::asio::error::operation_aborted )
            {
                // 处理关闭错误
                std::cerr << "websock异步close出现问题: " << err.message() << std::endl;
            }

            // 关闭成功后，再处理底层 socket
            self->CloseUnderlyingTcp();
        }
    );

}

void WebsockConn::CloseUnderlyingTcp()
{
    boost::system::error_code err;

    // 先关闭 WebSocket（若未完全关闭）
    if ( websock->is_open() )
    {
        websock->close( websocket::close_code::none, err ); // 忽略错误
    }

    // 再关闭底层 TCP
    auto& tcp_layer = websock->next_layer();
    if ( tcp_layer.socket().is_open() )
    {
        tcp_layer.socket().shutdown( tcp::socket::shutdown_both, err );
        tcp_layer.socket().close( err );
    }
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
                // 如果 read_handler 未设定，则跳过该次 read
                if ( !self->read_handler )
                {
                    std::cout << "WebsockConn AsyncRead已跳过handler" << std::endl;

                    self->AsyncRead();
                    return;
                }

                // 如果有 read_handler 则正常开始处理
                bool is_successful
                    = self->read_handler( self->shared_from_this() );
                if ( !is_successful )
                {
                    std::cout << "WebSocket处理任务失败：" << self->mission << std::endl;
                    return;
                }

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

void WebsockConn::SyncAccept( const http::request<http::string_body>& request )
{
    auto self = shared_from_this();
    try
    {
        websock->accept( request );

        WebsockMgr::GetInstance()->AddConn( self );
        // 若未发生问题，则开始读入
        self->AsyncRead();
    }
    catch ( std::exception& exp )
    {
        std::cout << "WebsockConn同步accept处异常：" << exp.what() << std::endl;
        return;
    }
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
