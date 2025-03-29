#pragma once

#include "aliases.h"

#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/beast/http.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <memory>
#include <iostream>
#include <queue>
#include <mutex>
#include <string>

class WebsockMgr;
class NetLogicSystem;

// websocket 连接类
// http 连接的升级连接
class WebsockConn
    : public std::enable_shared_from_this<WebsockConn>
{
    friend class WebsockMgr;
    friend class NetLogicSystem;

    using Socket = websocket::stream<beast::tcp_stream>;

public:
    // WebsockConn 的 socket 是 http::request 对应的 socket
    WebsockConn( tcp::socket& socket );
    ~WebsockConn();

    // 获取唯一识别码
    std::string GetUid() const { return uuid; }
    // 获取底层流的 TCP socket
    tcp::socket& GetSocket();

    // 升级 accept

    // 异步 accept（启动时机由外部掌握）
    void SyncAccept( http::request<http::string_body>& request );

private:
    // 异步 read
    void AsyncRead();
    // 异步发送
    void AsyncSend( std::string msg );
    // 异步写入
    void AsyncWrite( std::string msg );

private:
    std::unique_ptr<Socket> websock; // 被封装的 socket
    std::string uuid; // 该链接的 UID

    beast::flat_buffer buf_recv; // 流式接受时的缓冲区
    std::queue<std::string> que_msg; // 信息队列
    std::mutex mtx_quemsg;
};