#include "http_conn.hpp"

#include "http_logic_mgr.hpp"

HttpConn::HttpConn( boost::asio::io_context& ioc )
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

void HttpConn::AsyncReadAndHandle()
{
    auto self = shared_from_this(); // 由于有异步写入，所以需要在 lambda 中传入 shared_ptr

    // 注册异步读取回调函数（由 socket 间接使用 ioc 监听）
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
                // 当正常读取完后，开始处理请求
                self->AsyncHandleRequest();
                self->AsyncCheckTimeout();

                std::cout << "HttpConn读取并处理完成" << std::endl;
            }
            catch ( std::exception& exp )
            {
                std::cout << "HttpConn AsyncReadAndHandle()中出现异常: " << exp.what() << std::endl;
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
            // 如果无错误地调用该回调函数，说明超时
            if ( !err )
            {
                self->socket.close(); // 超时后关闭连接

                std::cout << "HttpConn连接超时" << std::endl;
            }
        } );
}

void HttpConn::AsyncHandleRequest()
{
    response.version( request.version() );
    response.keep_alive( false ); // 默认创建短连接

    // get 请求
    if ( request.method() == http::verb::get )
    {
        PreparseGetParamsHelper();

        bool is_successful
            = HttpLogicMgr::GetInstance()->HandleGet( shared_from_this() );
        // 如果请求的 url 不存在，返回 404
        if ( !is_successful )
        {
            response.result( http::status::not_found );
            response.set( http::field::content_type, "text/plain; charset=utf-8" );
            // beast::ostream( response.body() ) << "url not found\r\n";
            WriteRspBodyHelper( "url not found\r\n" );

            AsyncWriteResponse();
            return;
        }

        // 正常处理时，大部分工作都交给 HandleGet()
        response.result( http::status::ok );
        response.set( http::field::server, "gate_server" );
        AsyncWriteResponse();
        return;
    }

    // post 请求
    if ( request.method() == http::verb::post )
    {
        // 先处理 post 请求的 url，然后再交给 HandlePost
        post_url = request.target();

        bool is_successful
            = HttpLogicMgr::GetInstance()->HandlePost( shared_from_this() );
        // 如果请求的 url 不存在，返回 404
        if ( !is_successful )
        {
            response.result( http::status::not_found );
            response.set( http::field::content_type, "text/plain; charset=utf-8" );
            // beast::ostream( response.body() ) << "url not found\r\n";
            WriteRspBodyHelper( "url not found\r\n" );

            AsyncWriteResponse();
            return;
        }

        // 同上 get 请求的处理
        response.result( http::status::ok );
        response.set( http::field::server, "gate_server" );
        AsyncWriteResponse();
        return;
    }
}

void HttpConn::AsyncWriteResponse()
{
    auto self = shared_from_this();

    response.content_length( response.body().size() );

    http::async_write(
        socket, response,
        [ self ] ( beast::error_code err, std::size_t size_bytes )
        {
            // whether the connection is successful, shutdown
            self->socket.shutdown( tcp::socket::shutdown_send, err ); // shutdown send end only
            self->timer_timeout.cancel();
        } );
}

std::string HttpConn::EncodeUrlHelper( const std::string& raw ) const
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

std::string HttpConn::DecodeUrlHelper( const std::string& url ) const
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

void HttpConn::WriteRspBodyHelper( const std::string& body )
{
    beast::ostream( response.body() ) << body;
}

void HttpConn::PreparseGetParamsHelper()
{
    auto uri = request.target();

    auto query_pos = uri.find( '?' );
    if ( query_pos == std::string::npos )
    {
        get_url = uri;
        return;
    }

    get_url = uri.substr( 0, query_pos );
    std::string query_string = uri.substr( query_pos + 1 );
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