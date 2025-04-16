#include "http_conn.hpp"

#include "http_logic_system.hpp"
#include "websock_mgr.hpp"

#include <fmt/format.h>
#include <boost/beast/http.hpp>
#include <json/json.hpp>

#include <format>

HttpConn::HttpConn( net::io_context& ioc )
    : socket( ioc )
    , buf_recv( 8 * 1024 )
    , timer_timeout( socket.get_executor(), std::chrono::seconds( 20 ) )
{
    std::cout << "HttpConn构造" << std::endl;
}

HttpConn::~HttpConn()
{
    std::cout << "HttpConn被析构" << std::endl;
}

void HttpConn::AsyncRead()
{
    auto self = shared_from_this(); // 由于有异步写入，所以需要在 lambda 中传入 shared_ptr

    // 注册异步读取回调函数（由 socket 间接使用 ioc 监听）
    // 虽然 socket 是 tcp 的，但是 http::async_read 会自动处理粘包和边界
    http::async_read(
        socket, buf_recv, request,
        [ self ] ( beast::error_code err, std::size_t size_bytes )
        {
            // http 会自动处理请求头，所以这里不需要处理字节长度
            boost::ignore_unused( size_bytes );

            if ( err )
            {
                std::cout << "HttpConn异步读取回调handler出错：" << err << std::endl;
                return;
            }

            try
            {
                std::cout << "HttpConn等待到请求：" << self->request.target() << std::endl;
                // 当正常读取完后，开始处理请求
                self->SyncHandle();

                std::cout << "HttpConn读取并处理完成" << std::endl;
            }
            catch ( std::exception& exp )
            {
                std::cout << "HttpConn AsyncRead中出现异常: " << exp.what() << std::endl;
            }
        } );
}

void HttpConn::AsyncCheckTimeout()
{
    auto self = shared_from_this();

    // 注册超时回调函数（使用与 socket 相同的 ioc 监听）
    timer_timeout.async_wait(
        [ self ] ( beast::error_code err )
        {
            std::cout << "AsyncCheckTimeout被调用" << err.message() << std::endl;

            // 如果无错误地调用该回调函数，说明超时
            if ( !err )
            {
                self->socket.close(); // 超时后关闭连接

                std::cout << "HttpConn连接超时" << std::endl;
            }
        } );
}

void HttpConn::SyncHandle()
{
    // GET 请求
    if ( request.method() == http::verb::get )
    {
        PreparseGetParamsHelper( request.target() );

        // 首先查看是否是 WebSocket 升级请求
        // 但是注意这这是截断，不进入 http 的逻辑部分，实际上是否创建 Websockt 不在这里决定
        if ( websocket::is_upgrade( request ) )
        {
            // 处理完升级请求后，该 HttpConn 已经没有 socket 可以使用，应该析构
            bool is_successful
                = HttpLogicSystem::GetInstance()->HandleUpgrade( shared_from_this() );
            // 如果不成功，发送失败的响应
            if ( !is_successful )
            {
                response.keep_alive( false );
                response.result( http::status::bad_request );
                response.set( http::field::content_type, "application/json" );

                nlohmann::json json_body;
                json_body.emplace( "error", EnumErrorCode::ErrorWebsocketUpgradeDinied );
                WriteRspBody( json_body.dump() );

                AsyncWriteResponse();
                return;
            }

            return;
        }

        // 如果不是升级请求，那就继续执行正常的 GET 处理逻辑
        bool is_successful
            = HttpLogicSystem::GetInstance()->HandleGet( shared_from_this() );
        // 如果请求的 url 不存在，返回 404
        if ( !is_successful )
        {
            response.keep_alive( false );
            response.result( http::status::not_found );
            response.set( http::field::content_type, "text/plain; charset=utf-8" );
            WriteRspBody( "url not found\r\n" );

            AsyncWriteResponse();
            return;
        }

        // 正常处理时，大部分工作都交给 HandleGet()
        response.set( http::field::server, "ChatServer" );

        // 最后异步写入
        if ( !is_delay )
        {
            AsyncWriteResponse();
            // 设置超时
            AsyncCheckTimeout();
        }

        return;
    }

    // POST 请求
    if ( request.method() == http::verb::post )
    {
        // 先处理 post 请求的 url，然后再交给 HandlePost
        post_url = request.target();

        bool is_successful
            = HttpLogicSystem::GetInstance()->HandlePost( shared_from_this() );
        // 如果请求的 url 不存在，返回 404
        if ( !is_successful )
        {
            response.result( http::status::not_found );
            response.set( http::field::content_type, "text/plain; charset=utf-8" );
            WriteRspBody( "url not found\r\n" );

            AsyncWriteResponse();
            return;
        }

        // 同上 get 请求的处理
        // 正常处理时，大部分工作都交给 HandleGet()
        response.set( http::field::server, "ChatServer" );

        // 最后异步写入
        if ( !is_delay )
        {
            AsyncWriteResponse();
            // 设置超时
            AsyncCheckTimeout();
        }

        return;
    }
}

void HttpConn::AsyncWriteResponse()
{
    auto self = shared_from_this();

    response.content_length( response.body().size() );

    http::async_write(
        socket,
        response,
        [ self ] ( beast::error_code err, std::size_t size_bytes )
        {
            std::cout << "AsyncWriteResponse被调用: " << err.message() << std::endl;

            if ( err )
            {
                std::cout << "HttpConn::AsyncWriteResponse接收到异常: " << err.message() << std::endl;
                return;
            }

            // 关闭连接和计时器
            self->socket.shutdown( tcp::socket::shutdown_both, err );
            self->timer_timeout.cancel();
        } );
}

std::string HttpConn::EncodeUrlHelper( const std::string& raw )
{
    std::ostringstream oss_encoded;

    for ( char ch : raw )
    {
        // only directly input alpha, num, and some common chars
        if ( std::isalnum( ch )
            || ch == '-' || ch == '_' || ch == '.' || ch == '~' )
        {
            oss_encoded << ch;
        }
        else if ( ch == ' ' )
        {
            oss_encoded << '+';
        }
        else
        {
            oss_encoded
                << '%' << std::uppercase << std::hex
                << static_cast< int >( static_cast< std::uint8_t >( ch ) );
        }
    }

    return oss_encoded.str();
}

std::string HttpConn::DecodeUrlHelper( const std::string& url )
{
    std::ostringstream oss_decoded;

    for ( std::size_t i = 0; i < url.size(); i++ )
    {
        char ch{};

        switch ( url[ i ] )
        {
            case '%':
            {
                std::string hex = url.substr( i + 1, 2 );
                ch = static_cast< char >( std::stoul( hex, nullptr, 16 ) );

                i += 2; // skip two chars after '%'

                break;
            }

            case '+':
            {
                ch = ' ';
                break;
            }

            default:
            {
                ch = url[ i ];
                break;
            }
        }

        oss_decoded << ch;
    }

    return oss_decoded.str();
}

void HttpConn::WriteRspBody( const std::string& body )
{
    beast::ostream( response.body() ) << body;
}

void HttpConn::PreparseGetParamsHelper( const std::string uri )
{
    auto query_pos = uri.find( '?' );
    if ( query_pos == std::string::npos )
    {
        get_url = uri;
        return;
    }

    get_url = uri.substr( 0, query_pos );
    std::string query_string = uri.substr( query_pos + 1 );
    get_raw_params = query_string; // 原本的参数部分
    query_string += "&"; // add "&" to simplify the check conditions

    std::string key, val;
    std::size_t pos_equal = 0;
    std::size_t pos_amper = 0;
    std::size_t pos_last_amper = 0;
    while ( pos_amper < query_string.size() )
    {
        if ( query_string[ pos_equal ] != '=' )
            pos_equal++;

        auto Parse
            = [ &key, &val, &query_string, this ] (
                std::size_t begin, std::size_t pos_equal, std::size_t end )
            {
                key = DecodeUrlHelper( query_string.substr( begin, pos_equal - begin ) );
                val = DecodeUrlHelper( query_string.substr( pos_equal + 1, end - pos_equal - 1 ) );
                get_params[ key ] = val;
            };
        if ( query_string[ pos_amper ] == '&' )
        {
            Parse( pos_last_amper, pos_equal, pos_amper );

            pos_equal = pos_amper + 1;
            pos_last_amper = pos_amper + 1;
        }
        pos_amper++;
    }
}
