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

class VerifiGrpcMgr
    : public Singleton<VerifiGrpcMgr>
{
    friend class Singleton<VerifiGrpcMgr>;

public:
    GetVerifiResponse GetVerificationCode( const std::string& email );

private:
    VerifiGrpcMgr();

private:
    GrpcConnectionPool conn_pool;
};