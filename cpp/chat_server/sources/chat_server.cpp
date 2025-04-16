#include "include/chat_server.hpp"

#include "asio_iocontext_pool.hpp"
#include "http_conn.hpp"
#include "aliases.h"
// #include "config_mgr.hpp"
#include "websock_conn.hpp"

#include "svr_https_mgr.hpp"

#include <boost/asio.hpp>

ChatServer::ChatServer()
    : ioc_server( IOC_THREAD_NUM )
    // , acceptor( ioc_server/*, tcp::endpoint( net::ip::address_v4::from_string( "127.0.0.1" ), 8081 )*/ )
{
    std::cout << "ChatServer构造" << std::endl;
}

ChatServer::~ChatServer()
{
    std::cout << "ChatServer被析构退出" << std::endl;
}

void ChatServer::Run()
{
    auto self = shared_from_this(); // 防止该实例自身被析构

    // 注册用于退出的信号处理回调函数
    boost::asio::signal_set signals( ioc_server, SIGINT, SIGTERM );
    signals.async_wait(
        [ self ] ( beast::error_code err, int signal )
        {
            if ( err )
            {
                std::cout << "ChatServer signals注册回调获取错误码：" << err << std::endl;
                return; // 直接返回，不再继续执行 ioc_main 的 stop 函数
            }

            std::cout << "GateServer收到信号：" << signal << std::endl;
            self->ioc_server.stop();
        } );

    // // 开始监听
    // self->AsyncListen();
    SvrHttpsMgr::GetInstance()->Run( ioc_server );

    ioc_server.run(); // 阻塞，事件循环直到所有任务完成
}

// void ChatServer::AsyncListen()
// {
//     auto self = shared_from_this();

//     auto& ioc_conn = AsioIoContextPool::GetInstance()->GetIoService();
//     std::shared_ptr<HttpConn> conn = std::make_shared<HttpConn>( ioc_conn );
//     acceptor.async_accept(
//         conn->GetSocket(),
//         [ self, conn ] ( beast::error_code err )
//         {
//             // 注意必须要传入 conn（一个 shared_ptr 的复制）
//             // 以防止 conn 在这个 AsyncListen 函数结束后被析构
//             try
//             {
//                 // 如果产生错误，略过 conn 的 AsyncRead 处理函数重新开始监听新的连接
//                 if ( err )
//                 {
//                     self->AsyncListen();
//                     return;
//                 }

//                 conn->AsyncRead();

//                 // 仍然是开始监听新的连接
//                 self->AsyncListen();
//             }
//             catch ( std::exception& exp )
//             {
//                 std::cout << "ChatServer AsyncListen中出现异常：" << exp.what() << std::endl;
//             }
//         } );
// }