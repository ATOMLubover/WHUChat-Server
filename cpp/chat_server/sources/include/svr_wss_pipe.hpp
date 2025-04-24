#pragma once

#include "aliases.h"
#include "svr_wss_conn.hpp"

#include <sstream>
#include <mutex>
#include <queue>

// 转发 WSS 信息的管道
class SvrWssPipe
    : public std::enable_shared_from_this<SvrWssPipe>
{
public:
    SvrWssPipe( int session_id );
    ~SvrWssPipe();

    // 获取 input
    std::shared_ptr<SvrWssConn> GetInput() { return m_input; }
    // 获取 output
    std::shared_ptr<SvrWssConn> GetOutput() { return m_output; }

    // 绑定 input
    void BindInput( std::shared_ptr<SvrWssConn> conn );
    // 绑定 output
    void BindOutput( std::shared_ptr<SvrWssConn> conn );

    // 取消绑定 input
    void UnbindInput() { m_input = nullptr; }
    // 取消绑定 output
    void UnbindOutput() { m_output = nullptr; }

    // 确定是否为空管道（无效管道）
    bool IsUnloaded() const;

public:
    // ApiServer 端 input 的 URI
    static std::string PIPE_URI_INPUT;
    // 客户端 output 的 URI
    static std::string PIPE_URI_OUTPUT;

private:
    // 启用 output 发送（线程不安全）
    // 当 msg == "\0" 时不会发送该 msg
    void MakeOutputSend( std::string msg );

    // 保存 message 进入流内
    void SaveMsg( std::string msg );

private:
    // 管道编号计数器
    static std::atomic<int> pipe_id_cnt;

    // 管道编号
    int m_id;
    // 对应的会话 ID
    int m_session_id;

    // 数据控制互斥锁
    std::mutex m_mtx;
    // ApiServer 服务器端的输入
    std::shared_ptr<SvrWssConn> m_input;
    // 客户端的输出
    std::shared_ptr<SvrWssConn> m_output;

    // 记录输入内容的流
    std::stringstream m_message;
    // 当 output 未连接时的缓存队列
    std::queue<std::string> m_que_buf;
};