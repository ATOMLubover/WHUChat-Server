#pragma once

namespace boost::beast { }
namespace beast = boost::beast;				// from <boost/beast.hpp>
namespace boost::beast::http { }
namespace http = boost::beast::http;		// from <boost/beast/http.hpp>
namespace boost::asio { }
namespace net = boost::asio;				// from <boost/asio.hpp>

namespace boost::asio::ip { class tcp; }
using tcp = boost::asio::ip::tcp;			// from <boost/asio/ip/tcp.hpp>

// 用于标识处理过程中的错误码
enum class EnumErrorCode
{
    Success = 0,                          // 正常处理请求

    ErrorException = 1,                   // GateServer 中产生未定义错误

    ErrorRedis = 101,                     // VrfGrpcServer 调用 Redis 出现错误
    ErrorSend = 102,                      // VrfGrpcServer 未能成功发送验证邮件
    ErrorVrfException = 103,              // VrfGrpcServer 未定义异常

    ErrorServerNotResponding = 1001,      // GateServer 未收到其他服务器的响应
    ErrorGrpc = 1002,                     // GateServer 调用 gRPC 出现错误
    ErrorJson = 1003,                     // GateServer 处理前端传输 JSON 出现错误
    ErrorMySql = 1004,                    // GateServer 调用 MySQL 时发生异常
    ErrorUsernameExists = 1005,           // GateServer 无法注册新用户：用户名已存在
    ErrorEmailConflicts = 1006,           // GateServer 无法注册新用户：email已被注册
    ErrorPwdIncorreponds = 1007,          // GateServer 无法注册新用户：密码不一致
    ErrorVrfInvalid = 1008,               // GateServer 无法注册新用户：验证码不一致
    ErrorPwdWrong = 1009,                 // GateServer 无法登录用户：密码错误
    ErrorEmailInvalid = 1010,             // GateServer 无法登录用户：email 未注册
    ErrorLoginCookieInvalid = 1011,       // GateServer 拒绝访问：cookie 无效
    ErrorCookieNotFound = 1012,           // GateServer 未找到 cookie
    ErrorUnableGetServer = 1013           // GateServer 无法获取 ChatServer 地址
};