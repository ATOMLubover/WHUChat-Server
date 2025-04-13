#pragma once

#include "websock_conn.hpp"

#include <memory>
#include <queue>
#include <mutex>

// 在逻辑上关联两个 Websocket
// 将一个 Websocket 中的内容转发到另一个连接中
class WebsockMsgPipe
    : public std::enable_shared_from_this<WebsockMsgPipe>
{
public:
    WebsockMsgPipe( int ssn_id, int model_id );
    WebsockMsgPipe( const WebsockMsgPipe& ) = delete;
    WebsockMsgPipe& operator=( const WebsockMsgPipe& ) = delete;
    ~WebsockMsgPipe();

    // 获取 input
    std::shared_ptr<WebsockConn> GetInput() { return input; }
    // 获取 output
    std::shared_ptr<WebsockConn> GetOutput() { return output; }

    // // 确定该连接是否是对应的 output
    // bool IsTarget( std::shared_ptr<WebsockConn> conn );
    // 绑定 input 连接
    void BindInput( std::shared_ptr<WebsockConn> conn );
    // 绑定 output 连接
    void BindOutput( std::shared_ptr<WebsockConn> conn );

    // 卸载 input
    void UnloadInput() { input = nullptr; }
    // 卸载 output
    void UnloadOutput() { output = nullptr; }

    // 返回是否是空管道（没有输入和输出）
    bool IsUnloaded() const;

private:
    void DoOutputSend( const std::string& msg );

public:
    static const std::string PIPE_URI_CLI;
    static const std::string PIPE_URI_API;

private:
    // int uuid;
    int session_id;
    int model_id;

    std::shared_ptr<WebsockConn> input;
    std::shared_ptr<WebsockConn> output;

    std::queue<std::string> que_buf;
    std::mutex mtx_quebuf;

    // 存储所有传输过的信息
    std::string msg_sent;
    std::mutex mtx_msgsent;
};