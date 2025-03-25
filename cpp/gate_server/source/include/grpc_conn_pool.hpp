#pragma once

#include <grpcpp/grpcpp.h>
#include "message.grpc.pb.h"

#include <string>
#include <atomic>
#include <queue>
#include <memory>
#include <condition_variable>

// RAII 类
// 作为成员变量使用，而非管理类
// 同样需要手动获得和换回链接（内置锁）
class GrpcConnectionPool
{
public:
    GrpcConnectionPool( std::size_t size, std::string host, std::string port );
    GrpcConnectionPool( const GrpcConnectionPool& ) = delete;
    GrpcConnectionPool& operator=( const  GrpcConnectionPool& ) = delete;
    ~GrpcConnectionPool();

    // 由于 stub 不可复制，所以直接移动指向其的 unique_ptr
    std::unique_ptr<message::VerifiService::Stub> TakeConnection();
    // 还回 grpc 连接（锁同步）
    void ReturnConnection( std::unique_ptr<message::VerifiService::Stub>&& connection );

private:
    void ClosePool();

private:
    std::size_t size;
    std::queue<std::unique_ptr<message::VerifiService::Stub>> connections;

    std::atomic<bool> is_stopped;
    std::condition_variable cv_queconn;
    std::mutex mtx_queconn;

    std::string host;
    std::string port;
};