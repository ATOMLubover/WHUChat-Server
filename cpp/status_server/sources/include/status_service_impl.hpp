#pragma once

#include <grpcpp/grpcpp.h>
#include "message.grpc.pb.h"

#include <string>
#include <vector>
#include <mutex>

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

using message::GetChatServerRequest;
using message::GetChatServerResponse;
using message::StatusService;

struct ChatServerInfo
{
    std::string host;
    std::string port;

    static std::string GenUuidHelper();
};

// 对 StatusService 的实现（Node.js 中是自动实现的）
class StatusServiceImpl final
    : public StatusService::Service
{
public:
    StatusServiceImpl();
    Status GetChatServer(
        ServerContext* context,
        const GetChatServerRequest* request,
        GetChatServerResponse* reply ) override;

private:
    void CacheToken( int uuid, const std::string& token );

private:
    std::vector<ChatServerInfo> list_chatsrv_info; // 存储了 ChatServer 的信息
    std::mutex mtx_servers;
};