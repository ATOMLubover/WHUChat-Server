#pragma once

#include "aliases.h"

#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast.hpp>
#include <boost/beast/http.hpp>

#include <memory>
#include <cctype>

// 聊天的后台服务器
class ChatServer
    : public std::enable_shared_from_this<ChatServer> // 由于使用异步回调，故使用CRTP
{
public:
    ChatServer();
    ~ChatServer();

    // 开始运行服务器
    void Run();

private:
    // // 创建一个 HttpConn 连接，异步等待客户端的连接
    // void AsyncListen();

private:
    const std::size_t IOC_THREAD_NUM = 1; // 用于 AsyncListen 的 io_context 的线程数
    net::io_context ioc_server; // 用于 AsyncListen 的 io_context

    // tcp::acceptor acceptor;
};