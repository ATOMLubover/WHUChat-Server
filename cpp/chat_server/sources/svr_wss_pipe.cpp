#include "svr_wss_pipe.hpp"

#include "mysql_mgr.hpp"

#include <fmt/format.h>

#include <iostream>

std::string SvrWssPipe::PIPE_URI_INPUT = "/api/v1/ws/send_ans";
std::string SvrWssPipe::PIPE_URI_OUTPUT = "/api/v1/ws/trans_ans";

std::atomic<int> SvrWssPipe::pipe_id_cnt = 0;

SvrWssPipe::SvrWssPipe( int session_id )
    : m_id( pipe_id_cnt.fetch_add( 1 ) )
    , m_session_id( session_id )
{
    std::clog << fmt::format(
        "SvrWssPipe(ID: {})构造，其session_id为{}", m_id, session_id )
        << std::endl;
}

SvrWssPipe::~SvrWssPipe()
{
    // 尝试存储消息，如果不成功打印日志
    if ( MySqlMgr::GetInstance()->CreateMessage(
        m_session_id, 0, 100,
        m_message.str(), "assistant",
        "" )
        != 0 )
    {
        std::cerr << fmt::format(
            "SvrWssPipe(ID: {}, sessiond_id: {})析构时存储消息失败\n",
            m_id, m_session_id );
    }

    std::cout << "WebsockMsgPipe被析构，其session_id：" << m_session_id
        << "\n所有接受到内容为：" << m_message.str() << std::endl;
}

void SvrWssPipe::BindInput( std::shared_ptr<SvrWssConn> conn )
{
    std::lock_guard<std::mutex> lock( m_mtx );

    m_input = conn;

    conn->SetReadHandler(
        [ self = shared_from_this() ] ( std::shared_ptr<SvrWssConn> conn )->bool
        {
            // 先读取缓冲区中读到的内容
            auto& buf_recv = conn->GetBufRecv();
            std::string msg = std::move( beast::buffers_to_string( buf_recv.data() ) );
            buf_recv.consume( buf_recv.size() );

            try
            {
                {
                    std::lock_guard<std::mutex> lock( self->m_mtx );

                    // 若是管道的 output 已绑定，则直接发送
                    if ( self->m_output )
                    {
                        self->MakeOutputSend( std::move( msg ) );
                        return true;
                    }
                }

                // 如果 output 未接入，则先暂存
                {
                    std::lock_guard<std::mutex> lock( self->m_mtx );

                    std::clog << fmt::format(
                        "SvrWssPipe(ID: {})暂存输入信息：{}",
                        self->m_id, msg )
                        << std::endl;

                    self->m_que_buf.push( msg );

                    return true;
                }
            }
            catch ( std::exception& exp )
            {
                std::cerr << fmt::format(
                    "SvrWssPipe(ID: {}) input(ID: {})处理read处异常：{}\n",
                    self->m_id, conn->GetId(), exp.what() );
                return false;
            }
        } );

    std::clog << fmt::format(
        "SvrWssPipe(ID: {})绑定输入conn(ID: {})，其session_id为 {}\n",
        m_id, conn->GetId(), m_session_id );

}

void SvrWssPipe::BindOutput( std::shared_ptr<SvrWssConn> conn )
{
    std::lock_guard<std::mutex> lock( m_mtx );

    m_output = conn;

    // 一但接入 output，先检查是否需要发送积存信息
    MakeOutputSend( "\0" );

    std::clog << fmt::format(
        "SvrWssPipe(ID: {})绑定输出conn(ID: {})，其session_id为 {}\n",
        m_id, conn->GetId(), m_session_id );
}

bool SvrWssPipe::IsUnloaded() const
{
    if ( m_input || m_output )
        return false;

    return true;
}

void SvrWssPipe::MakeOutputSend( std::string msg )
{
    // 先发送积存的信息
    while ( !m_que_buf.empty() )
    {
        m_message << m_que_buf.front();

        m_output->DoSend( std::move( m_que_buf.front() ) );
        m_que_buf.pop();
    }

    // 然后再发送当前信息
    if ( msg != "\0" )
    {
        m_message << msg;
        m_output->DoSend( msg );
    }
}
