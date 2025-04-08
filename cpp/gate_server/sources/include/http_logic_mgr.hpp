#pragma once

#include "singleton.hpp"
#include "aliases.h"

#include <boost/beast/http.hpp>
#include <json/json.hpp>

#include <functional>
#include <map>
#include <memory>
#include <vector>
#include <string>

class HttpConn;
using HttpHandler = std::function<void( std::shared_ptr<HttpConn> )>;

class HttpLogicMgr
    : public Singleton<HttpLogicMgr>
{
    friend class Singleton<HttpLogicMgr>;

    enum class EnumCheckCookie
    {
        Safe,   // 检查 cookie
        Unsafe  // 不检查 cookie
    };

public:
    ~HttpLogicMgr();

    bool HandleGet( std::shared_ptr<HttpConn> conn );
    bool HandlePost( std::shared_ptr<HttpConn> conn );

private:
    HttpLogicMgr();

    // 注册 get 请求处理函数（url, handler）
    void RegisterGet( const std::string& url, HttpHandler handler );
    // 注册 post 请求处理函数（url, handler）
    void RegisterPost( const std::string& url, HttpHandler handler );

    // 注册所有 GET 请求
    void InitGet();
    // 注册所有 POST 请求
    void InitPost();

    // 自动注册前端文件夹下，整个文件夹的文件
    // dir 需要以 “/” 结尾
    // safe 参数用来表示是否需要 cookie 验证（默认不检查）
    void AutoRegDir(
        const std::string& prefix_offset, const std::string& url_dir,
        EnumCheckCookie safe = EnumCheckCookie::Unsafe );

    // 得到某个文件夹下所有文件的名称（注意不包含 dir 文件夹）
    static std::vector<std::string> GetAllFilesHelper( const std::string& dir );
    // 获得用于 file_body 的响应体的内容
    static http::file_body::value_type PrepareFileBodyHelper( const std::string& file );
    // 获得某个文件的 content-type
    static std::string GetMimeHelper( const std::string& file );

    // 辅助转换 string 为 JSON
    static nlohmann::json ParseJsonHelper( const std::string& str );

    // 检查 cookie 是否有效（本质就是检查记录的时间是否是在最近三天内）
    static bool CheckCookieValid( std::map<std::string, std::string>& map_cookie );

private:
    static const std::string FRONTEND_STATIC_DIR; // 前端静态资源目录

    std::map<std::string, HttpHandler> post_handlers; // 键：url，值：处理函数
    std::map<std::string, HttpHandler> get_handlers; // 键：url，值：处理函数
};
