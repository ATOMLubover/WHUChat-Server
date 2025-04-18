#pragma once

#include "aliases.h"
#include "svr_https_conn.hpp"

#include <boost/beast/websocket.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/core.hpp>

#include <memory>
#include <atomic>
#include <queue>
#include <functional>
#include <mutex>

class SvrHttpsConn;
class SvrWssConn;

// 保证自身不被析构的回调函数
using WssHolder = std::function<void( std::shared_ptr<SvrWssConn> )>;
// 去除自身保留的回调函数
using WssRemover = std::function<void( std::shared_ptr<SvrWssConn> )>;
// WSS 读取成功时的回调类型
using WssReadHandler = std::function<bool( std::shared_ptr<SvrWssConn> )>;

// 服务器型 WSS 连接
class SvrWssConn
    : public std::enable_shared_from_this<SvrWssConn>
{
public:
    SvrWssConn( std::shared_ptr<SvrHttpsConn> http_conn );
    ~SvrWssConn();

    // 获取 ID
    int GetId() const { return m_id; }
    // 获取 URI
    std::string GetUri() const { return m_uri; }
    // 获取参数
    std::map<std::string, std::string> GetParGet() const { return m_par_get; }
    // 获取读取内容的缓冲区
    beast::flat_buffer& GetBufRecv() { return m_buf_recv; }
    // 获取发送内容的缓冲队列
    std::queue<std::string>& GetQueWait() { return m_que_wait; }

    // 进行 WSS 的升级握手（开始持续运行异步处理链的 Run 函数）
    void DoAccept( std::shared_ptr<http::request<http::dynamic_body>> req );
    // 预发送
    void DoSend( std::string msg );

    // 注入保存自身不被析构的回调
    void SetHolder( WssHolder holder ) { m_holder = std::move( holder ); }
    // 注入去除自身保留的回调
    void SetRemover( WssRemover remover ) { m_remover = std::move( remover ); }
    // 注入 WSS 读取成功时的回调
    void SetReadHandler( WssReadHandler handler ) { m_read_handler = std::move( handler ); }

    // 同步关闭连接（包括底层 socket）
    // 因为考虑到要在析构函数中调用
    void Close();

private:
    // handshake 的处理回调
    void OnAccept( beast::error_code ec, HttpsReq req );

    // 读取消息
    void DoRead();
    // 读取消息的回调
    void OnRead( beast::error_code ec );

    // 真正写入并发送（会保证发送到没有消息等待）
    void DoWrite( std::string msg );
    // 发送消息的回调
    void OnWrite( beast::error_code ec, std::string msg );

private:
    // 编号用的总计时器
    static std::atomic<int> total_cnt;

    // 连接编号
    int m_id;
    // 升级的 HTTPS 连接的 URI
    std::string m_uri;
    // 升级的 HTTPS 连接的 GET 参数
    std::map<std::string, std::string> m_par_get;

    // 使用底层 SSL 流的 websocket 流
    websocket::stream<ssl::stream<tcp::socket>> m_wss_stream;

    // 发送时使用的缓冲区
    std::queue<std::string> m_buf_send;
    // 读入时的缓冲区，由于要多次读入，所以作为成员函数
    beast::flat_buffer m_buf_recv;

    // 发送时的等待队列
    std::queue<std::string> m_que_wait;
    // 读写锁
    std::mutex m_mtx_quewait;

    // 保存自身不被析构的回调
    WssHolder m_holder;
    // 去除保留自身的回调
    WssRemover m_remover;
    // 读写回调
    WssReadHandler m_read_handler;
};