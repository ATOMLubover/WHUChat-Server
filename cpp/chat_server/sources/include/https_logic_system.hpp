#pragma once

#include "singleton.hpp"
#include "svr_https_conn.hpp"

#include <json/json.hpp>

// 独立的 HTTPS 连接的逻辑处理
class HttpsLogicSystem
    : public Singleton<HttpsLogicSystem>
{
    friend class Singleton<HttpsLogicSystem>;

public:
    ~HttpsLogicSystem();

    // 进行初始化，注册所有回调函数
    void Init();

    // 向 SvrHttpsConn 中注册逻辑找寻函数
    void RegisterGetter( std::shared_ptr<SvrHttpsConn> conn );

private:
    HttpsLogicSystem();

    // 初始化所有 GET handler
    void InitGetHandlers();
    // 初始化所有 POST handler
    void InitPostHandlers();

    // 为 SvrHttpsConn 提供 GET 请求的处理回调
    HttpsReadHandler FindGetHandler( const std::string& uri );
    // 为 SvrHttpsConn 提供 POST 请求的处理回调
    HttpsReadHandler FindPostHandler( const std::string& uri );

    // 注册 GET 请求的处理回调
    void RegisterGetHandler( const std::string& uri, HttpsReadHandler handler );
    // 注册 POST 请求的处理回调
    void RegisterPostHandler( const std::string& uri, HttpsReadHandler handler );

    // 检查 cookie 是否有效
    // 当 uuid 为 -1 时，则不检查 uuid；为 0 时则不检查整个 cookie
    bool CheckCookieWithUuid( const http::request<http::dynamic_body>& req, int uuid );
    // 将请求体反序列化为 json
    nlohmann::json ParseJson( const http::request<http::dynamic_body>& req );

    /////////////////////////////////
    /// 以下为一些处理具体逻辑的函数 ///
    /////////////////////////////////

    // /api/v1/chat/send_message 中，转发请求到 ApiSerevr
    // 并异步等待其回答的逻辑
    void TransferMsgToApiServer( std::shared_ptr<SvrHttpsConn> conn );

private:
    // GET 请求的处理回调
    std::map<std::string, HttpsReadHandler> m_get_handlers;
    // POST 请求的处理回调
    std::map<std::string, HttpsReadHandler> m_post_handlers;
};