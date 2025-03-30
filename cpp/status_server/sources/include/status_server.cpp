#include "status_server.hpp"

#include "aliases.h"
#include "config_mgr.hpp"
#include "status_service_impl.hpp"

#include <grpcpp/grpcpp.h> 
#include <boost/asio.hpp>  
#include <boost/asio/signal_set.hpp>

#include <thread>           
#include <memory>           
#include <iostream>         
#include <string>

StatusServer::StatusServer()
    : server_addr(
        ConfigMgr::GetInstance()[ "status_server" ][ "host" ]
        + ConfigMgr::GetInstance()[ "status_server" ][ "port" ] )
{ }

void StatusServer::Run()
{
    try
    {  // 创建服务
        StatusServiceImpl status_srv_impl;
        // 创建服务器构建起
        grpc::ServerBuilder builder;
        builder.AddListeningPort(
            server_addr, grpc::InsecureServerCredentials() );
        builder.RegisterService( &status_srv_impl );

        // 构建并启动 gRPC 服务器
        std::unique_ptr<grpc::Server> server( builder.BuildAndStart() );
        std::cout << "gRPC服务器监听于：" << server_addr << std::endl;

        // 为了在容器中运行和退出，采取 signal_set
        net::io_context ioc{ 1 };
        net::signal_set signals( ioc, SIGINT, SIGTERM );
        // 设置异步停止
        signals.async_wait(
            [ &server ] ( boost::system::error_code err, int signal_num )
            {
                if ( !err )
                {
                    server->Shutdown();
                    std::cout << "StatusServer关闭" << std::endl;
                }
            } );
        // 监听停止信号的 ioc 在另外的线程中运行
        std::thread( [ &ioc ] () { ioc.run(); } ).detach();

        server->Wait(); // 阻塞监听请求
        ioc.stop();
    }
    catch ( std::exception& exp )
    {
        std::cout << "StatusServer运行中异常：" << exp.what() << std::endl;
        return;
    }
}
