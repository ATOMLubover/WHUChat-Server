#include "grpc_conn_pool.hpp"

GrpcConnectionPool::~GrpcConnectionPool()
{
    std::lock_guard<std::mutex> guard( mtx_queconn );

    ClosePool();
    if ( connections.empty() )
        connections.pop(); // 由于是智能指针，所以弹出后不用手动 delete
}

std::unique_ptr<message::VerifiService::Stub> GrpcConnectionPool::TakeConnection()
{
    std::unique_lock<std::mutex> lock( mtx_queconn );

    // 此处决定线程是否挂起并释放锁：若连接池不空或者连接池已经停止
    cv_queconn.wait(
        lock,
        [ this ] ()
        {
            if ( this->is_stopped )
                return true;

            return !this->connections.empty();
        } );

    if ( is_stopped )
        return nullptr;

    auto stub = std::move( connections.front() );
    connections.pop();
    return std::move( stub );
}

void GrpcConnectionPool::ReturnConnection( std::unique_ptr<message::VerifiService::Stub>&& connection )
{
    std::lock_guard<std::mutex> guard( mtx_queconn );

    // 当连接池已经停止，则没必要重新再回收到 connections 中
    if ( is_stopped )
        return;

    connections.emplace( std::move( connection ) );
    cv_queconn.notify_one(); // 如果有线程在等待取用，则有一个幸运儿可以获得
}

void GrpcConnectionPool::ClosePool()
{
    is_stopped = true;
    cv_queconn.notify_all();
}

GrpcConnectionPool::GrpcConnectionPool( std::size_t size, std::string host, std::string port )
    : size( size ), host( host ), port( port )
    , is_stopped( false )
{
    std::string target = host + ":" + port;
    for ( std::size_t i = 0; i < size; i++ )
    {
        // 创建 channel（对长连接的抽象）
        std::shared_ptr<grpc::Channel> channel
            = grpc::CreateChannel( target, grpc::InsecureChannelCredentials() );
        std::cout << "新grpc channel已创建在: " << target << std::endl;
        // 将新的 stub 创建在 该 channel 底下
        connections.emplace( message::VerifiService::NewStub( channel ) );
    }
}