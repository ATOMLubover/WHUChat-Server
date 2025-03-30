#pragma once

#include "singleton.hpp"
#include "grpc_conn_pool.hpp"

#include <grpcpp/grpcpp.h>
#include "message.grpc.pb.h"

#include <cctype>

using grpc::Channel;
using grpc::Status;
using grpc::ClientContext;

using message::GetChatServerRequest;
using message::GetChatServerResponse;
using message::StatusService;

class StatusGrpcMgr
    : public Singleton<StatusGrpcMgr>
{
    friend class Singleton<StatusGrpcMgr>;

public:
    GetChatServerResponse GetChatServer( std::int32_t uuid );

private:
    StatusGrpcMgr();

private:
    GrpcConnPool<message::StatusService> conn_pool;
};