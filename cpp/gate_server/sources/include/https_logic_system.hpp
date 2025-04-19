#pragma once

#include "singleton.hpp"
#include "svr_https_conn.hpp"

#include <json/json.hpp>

#include <vector>

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

    // 代理整个文件夹文件的 GET 请求
    void RegisterDir( const std::string& prefix, const std::string& url_dir, bool need_cookie = false );
    // 获取一个文件夹所有子文件（夹）的函数
    std::vector<std::string> GetDirFiles( const std::string& dir );
    // 获取 MIME 类型
    static std::string GetMimeType( const std::string& file );

    // 辅助获得 file_body
    static http::file_body::value_type PrepareFileBody( const std::string& file );

    // 检查 cookie 是否有效
    // 当 uuid 为 -1 时，则不检查 uuid；为 0 时则不检查整个 cookie
    static bool CheckCookie( const http::request<http::dynamic_body>& req );
    // 将请求体反序列化为 json
    static nlohmann::json ParseJson( const http::request<http::dynamic_body>& req );

private:
    // 前段代理文件所在的根目录
    static const std::string FRONTEND_STATIC_DIR;

    // GET 请求的处理回调
    std::map<std::string, HttpsReadHandler> m_get_handlers;
    // POST 请求的处理回调
    std::map<std::string, HttpsReadHandler> m_post_handlers;
};