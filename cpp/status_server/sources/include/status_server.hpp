#pragma once

#include <string>

// 基于 gRPC 通信的状态服务器
// 由于只在登录过程中被调用，全程使用同步的处理方式
class StatusServer
{
public:
    StatusServer();

    void Run();

private:
    std::string server_addr;
};