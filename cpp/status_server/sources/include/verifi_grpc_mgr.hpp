#pragma once

#include "singleton.hpp"
#include "grpc_conn_pool.hpp"

#include <grpcpp/grpcpp.h>
#include "message.grpc.pb.h"

using grpc::Channel;
using grpc::Status;
using grpc::ClientContext;

using message::GetVerifiRequest;
using message::GetVerifiResponse;
using message::VerifiService;

// 验证码发送微服务调用器
// 注意验证码发送的微服务器必须部署在 127.0.0.1
class VerifiGrpcMgr
    : public Singleton<VerifiGrpcMgr>
{
    friend class Singleton<VerifiGrpcMgr>;

public:
    GetVerifiResponse GetVerificationCode( const std::string& email );

private:
    VerifiGrpcMgr();

private:
    GrpcConnPool<message::VerifiService> conn_pool;
};