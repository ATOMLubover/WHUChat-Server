#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast.hpp>
#include <boost/beast/http.hpp>

namespace beast = boost::beast;				// from <boost/beast.hpp>
namespace http = boost::beast::http;		// from <boost/beast/http.hpp>
namespace net = boost::asio;				// from <boost/asio.hpp>

using tcp = boost::asio::ip::tcp;			// from <boost/asio/ip/tcp.hpp>

#include <memory>
#include <string>
#include <map>
#include <chrono>

class HttpLogicMgr;

// http 连接类
// 管理一个 http 短连接
class HttpConn
    : public std::enable_shared_from_this<HttpConn> // 由于有异步写入，所以允许 shared
{
    friend class HttpLogicMgr;

public:
    // HttpConn 所绑定的 ioc 由 HttpLogicMgr 提供
    HttpConn( boost::asio::io_context& ioc );
    ~HttpConn();

    // 异步监听读
    void Start();

    tcp::socket& GetSocket() { return socket; }

private:
    // 异步检查连接是否超时
    void CheckTimeout();
    // 同步调用 HttpLogicMgr 处理请求，但是异步写回 response
    void HandleRequest();
    // 异步写 response，写完后 beast 会自动发送
    void WriteResponse();

    std::string EncodeUrlHelper( const std::string& raw ) const;
    std::string DecodeUrlHelper( const std::string& url ) const;

    // 封装 beast::ostream( response.body() ) << body 的操作
    void WriteRspBodyHelper( const std::string& body );

    // 预解析 get 请求的参数，结果存入 get_url 和 get_params
    void PreparseGetParamsHelper();

private:
    tcp::socket socket; // 与客户端通信的 tcp socket

    beast::flat_buffer buf_recv; // 设置缓冲区大小为 8KB，因为一次通常接受不超过 1500B
    http::request<http::dynamic_body> request;
    http::response<http::dynamic_body> response;

    net::steady_timer timer_timeout; // 设置超时时间为 20s

    std::string get_url; // get 请求的根路由 
    std::map<std::string, std::string> get_params; // get 请求的参数

    std::string post_url; // post 请求的根路由
};