#include "gate_server.hpp"

#include "asio_iocontext_pool.hpp"
#include "http_conn.hpp"
#include "config_mgr.hpp"

GateServer::GateServer()
    : ioc_server( IOC_THREAD_NUM )
    , acceptor( ioc_server, tcp::endpoint( tcp::v4() /* 这里是默认监听 0.0.0.0 */,
        static_cast< std::uint16_t >( std::stoi( ConfigMgr::GetInstance()[ "gate_server" ][ "port" ] ) ) ) )
{
    std::cout << "GateServer构造，监听于：0.0.0.0:"
        << std::stoi( ConfigMgr::GetInstance()[ "gate_server" ][ "port" ] ) << std::endl;
}

GateServer::~GateServer()
{
    std::cout << "GateServer被析构退出" << std::endl;
}

void GateServer::Run()
{
    auto self = shared_from_this(); // 防止该实例自身被析构

    // 注册用于退出的信号处理回调函数
    boost::asio::signal_set signals( ioc_server, SIGINT, SIGTERM );
    signals.async_wait(
        [ self ] ( beast::error_code err, int signal )
        {
            if ( err )
            {
                std::cout << "GateServer signals注册回调获取错误码：" << err << std::endl;
                return; // 直接返回，不再继续执行 ioc_main 的 stop 函数
            }

            std::cout << "GateServer收到信号：" << signal << std::endl;
            self->ioc_server.stop();
        } );

    // 开始监听
    self->AsyncListen();

    ioc_server.run(); // 阻塞，事件循环直到所有任务完成
}

void GateServer::AsyncListen()
{
    auto self = shared_from_this(); // keep self from being destructed

    auto& ioc_conn = AsioIoContextPool::GetInstance()->GetIoService();
    std::shared_ptr<HttpConn> conn = std::make_shared<HttpConn>( ioc_conn );
    acceptor.async_accept(
        conn->GetSocket(),
        [ self, conn ] ( beast::error_code err )
        {
            // 注意必须要传入 conn（一个 shared_ptr 的复制）
            // 以防止 conn 在这个 AsyncListen 函数结束后被析构
            try
            {
                // 如果产生错误，略过 conn 的 AsyncReadAndHandle 处理函数重新开始监听新的连接
                if ( err )
                {
                    self->AsyncListen();
                    return;
                }

                conn->AsyncReadAndHandle();

                // 仍然是开始监听新的连接
                self->AsyncListen();
            }
            catch ( std::exception& exp )
            {
                std::cout << "GateServer AsyncListen中出现异常：" << exp.what() << std::endl;
            }
        } );
}