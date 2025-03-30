#pragma once

#include <grpcpp/grpcpp.h>
#include "message.grpc.pb.h"

#include <string>
#include <atomic>
#include <queue>
#include <memory>
#include <condition_variable>
#include <concepts>

// 定义 concept
template<typename T>
concept HasStub = requires {
    typename T::Stub;
};
template<class T>
concept GrpcSrv = (
    std::is_same_v<T, message::VerifiService>
    || std::is_same_v<T, message::StatusService> )
    && HasStub<T>;

// RAII 类
// 作为成员变量使用，而非管理类
// 同样需要手动获得和换回链接（内置锁）
template<GrpcSrv Srv>
class GrpcConnPool
{
    using GrpcStub = Srv::Stub;

public:
    GrpcConnPool( std::size_t size, std::string host, std::string port )
        : size( size ), host( host ), port( port )
        , is_stopped( false )
    {
        std::string target = host + ":" + port;
        for ( std::size_t i = 0; i < size; i++ )
        {
            // 创建 channel（对长连接的抽象）
            std::shared_ptr<grpc::Channel> channel
                = grpc::CreateChannel( target, grpc::InsecureChannelCredentials() );
            std::cout << "新gRPC channel已创建在: " << target << std::endl;
            // 将新的 stub 创建在 该 channel 底下
            que_stub.emplace( Srv::NewStub( channel ) );
        }
    }
    GrpcConnPool( const GrpcConnPool& ) = delete;
    GrpcConnPool& operator=( const  GrpcConnPool& ) = delete;
    ~GrpcConnPool()
    {
        std::lock_guard<std::mutex> guard( mtx_questub );

        ClosePool();
        if ( que_stub.empty() )
            que_stub.pop(); // 由于是智能指针，所以弹出后不用手动 delete
    }

    // 由于 stub 不可复制，所以直接移动指向其的 unique_ptr
    std::unique_ptr<GrpcStub> TakeConn()
    {
        std::unique_lock<std::mutex> lock( mtx_questub );

        // 此处决定线程是否挂起并释放锁：若连接池不空或者连接池已经停止
        cv_questub.wait(
            lock,
            [ this ] ()
            {
                if ( this->is_stopped )
                    return true;

                return !this->que_stub.empty();
            } );

        if ( is_stopped )
            return nullptr;

        auto stub = std::move( que_stub.front() );
        que_stub.pop();
        return std::move( stub );
    }
    // 还回 grpc 连接（锁同步）
    void ReturnConn( std::unique_ptr<GrpcStub>&& connection )
    {
        std::lock_guard<std::mutex> guard( mtx_questub );

        // 当连接池已经停止，则没必要重新再回收到 que_stub 中
        if ( is_stopped )
            return;

        que_stub.emplace( std::move( connection ) );
        cv_questub.notify_one(); // 如果有线程在等待取用，则有一个幸运儿可以获得
    }

private:
    void ClosePool()
    {
        is_stopped = true;
        cv_questub.notify_all();
    }

private:
    std::size_t size;
    std::queue<std::unique_ptr<GrpcStub>> que_stub;

    std::atomic<bool> is_stopped;
    std::condition_variable cv_questub;
    std::mutex mtx_questub;

    std::string host;
    std::string port;
};
