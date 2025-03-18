#pragma once

namespace boost::beast { }
namespace beast = boost::beast;				// from <boost/beast.hpp>
namespace boost::beast::http { }
namespace http = boost::beast::http;		// from <boost/beast/http.hpp>
namespace boost::asio { }
namespace net = boost::asio;				// from <boost/asio.hpp>

namespace boost::asio::ip { class tcp; }
using tcp = boost::asio::ip::tcp;			// from <boost/asio/ip/tcp.hpp>

// 用于标识 http 处理过程中的错误码
enum class EnumHttpErrorCode
{
    Success = 0,

    ErrorServerNotResponding = 1001,    // 上游服务器无响应
    ErrorUrlNotFound = 1002             // 无效URL
};