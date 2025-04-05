#include "websock_msg_pipe.hpp"

const std::string WebsockMsgPipe::PIPE_URI_CLI = "/trans_ans";
const std::string WebsockMsgPipe::PIPE_URI_API = "/send_ans";

WebsockMsgPipe::WebsockMsgPipe( int ssn_id )
    : session_id( ssn_id )
{
    std::cout << "WebsockMsgPipe构造，其session_id：" << session_id << std::endl;
}

WebsockMsgPipe::~WebsockMsgPipe()
{
    std::cout << "WebsockMsgPipe被析构，其session_id：" << session_id
        << "\n所有接受到内容为：" << msg_sent << std::endl;
}

// bool WebsockMsgPipe::IsTarget( std::shared_ptr<WebsockConn> conn )
// {
//     bool is_successful;
//     try
//     {
//         is_successful = conn->GetParams()[ "session_id" ] == input->GetParams()[ "session_id" ];
//     }
//     catch ( std::exception& exp )
//     {
//         std::cout << "WebsockMsgPipe IsTarget函数中出现异常：" << exp.what() << std::endl;
//         return false;
//     }
//     return is_successful;
// }

void WebsockMsgPipe::BindInput( std::shared_ptr<WebsockConn> conn )
{
    input = conn;

    auto self = shared_from_this();
    // 设置回调函数，保证 input 一但有输入，就向 output 输出
    input->SetReadHandler(
        [ self ] ( std::shared_ptr<WebsockConn> input ) -> bool
        {
            // 先提取收到的信息
            std::string data = beast::buffers_to_string( input->buf_recv.data() );
            input->buf_recv.consume( input->buf_recv.size() );

            // 若是 output 还没有绑定，则先缓存
            if ( !self->output )
            {
                std::cout << "WebsockMsgBridge的output未注册，信息暂存：" << data << std::endl;

                std::lock_guard<std::mutex> guard( self->mtx_quebuf );
                self->que_buf.push( data );

                return true;
            }

            // 若是 output 已绑定，则先发送缓存信息
            {
                std::lock_guard<std::mutex> guard( self->mtx_quebuf );

                while ( !self->que_buf.empty() )
                {
                    self->DoOutputSend( self->que_buf.front() );
                    self->que_buf.pop();
                }
            }

            // 在发送这次接受到的当前信息
            self->DoOutputSend( data );

            return true;
        } );
}

void WebsockMsgPipe::BindOutput( std::shared_ptr<WebsockConn> conn )
{
    output = conn;

    // 如果 output 是后于 input 接入的，且有信息积留则迅速发送之前的内容
    std::lock_guard<std::mutex> guard( mtx_quebuf );

    while ( !que_buf.empty() )
    {
        DoOutputSend( que_buf.front() );
        que_buf.pop();
    }
}

bool WebsockMsgPipe::IsUnloaded() const
{
    if ( input )
        return false;
    if ( output )
        return false;
    return true;
}

void WebsockMsgPipe::DoOutputSend( const std::string& msg )
{
    {
        // 存储消息以供数据库存储
        std::lock_guard<std::mutex> guard( mtx_msgsent );
        msg_sent += msg;
    }

    output->AsyncSend( msg );
}
