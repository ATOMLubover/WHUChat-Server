#include "svr_https_mgr.hpp"

#include "config_mgr.hpp"
#include "https_logic_system.hpp"
#include "cli_http_mgr.hpp"

#include <iostream>

SvrHttpsMgr::~SvrHttpsMgr()
{
    Stop();

    std::clog << "SvrHttpsMgr被析构" << std::endl;
}

void SvrHttpsMgr::Run( net::io_context& ioc )
{
    // 初始化 m_listener
    InitListener( ioc );
}

void SvrHttpsMgr::Stop()
{
    m_listener->Stop();
}

SvrHttpsMgr::SvrHttpsMgr()
{
    try
    {
        InitSslContext();

        // 初始化子管理器
        HttpsLogicSystem::GetInstance()->Init();
        CliHttpMgr::GetInstance()->Init( &( *m_ctx ) );
    }
    catch ( std::exception& exp )
    {
        std::cerr << "SvrHttpsMgr构造时出现异常: " << exp.what() << std::endl;
        return;
    }

    std::clog << "SvrHttpsMgr构造" << std::endl;
}

void SvrHttpsMgr::InitSslContext()
{
    ssl::context ctx( ssl::context::tlsv12 );
    ctx.set_options(
        ssl::context::default_workarounds |
        ssl::context::no_sslv2 |
        ssl::context::single_dh_use );

    std::string cert_dir = ConfigMgr::GetInstance()[ "chat_server" ][ "cert_dir" ];
    ctx.use_certificate_chain_file( cert_dir + "server.crt" );
    ctx.use_private_key_file( cert_dir + "server.key", ssl::context::pem );

    m_ctx = std::make_unique<ssl::context>( std::move( ctx ) );

    std::clog << "SvrHttpsMgr建立SSL证书完毕" << std::endl;
}

void SvrHttpsMgr::InitListener( net::io_context& ioc )
{
    std::string host = ConfigMgr::GetInstance()[ "chat_server" ][ "host" ];
    std::string port = ConfigMgr::GetInstance()[ "chat_server" ][ "port" ];

    m_listener = std::make_shared<HttpsListener>(
        ioc, *m_ctx,
        tcp::endpoint( net::ip::make_address( host ), std::stoi( port ) ) );

    m_listener->SetAcceptHandler(
        [ self = shared_from_this() ] ( tcp::socket&& socket, ssl::context& ctx )
        {
            auto conn = self->CreateConn( std::move( socket ), ctx );
            conn->DoHandshake();
        } );

    m_listener->Run();

    std::clog << "SvrHttpsMgr创建并运行listener" << std::endl;
}

std::shared_ptr<SvrHttpsConn> SvrHttpsMgr::CreateConn( net::ip::tcp::socket&& socket, ssl::context& ctx )
{
    auto conn
        = std::make_shared<SvrHttpsConn>( std::move( socket ), ctx );
    // 向连接注入延迟获得逻辑的函数（因为要等获得 request 才能确定要执行的逻辑）
    HttpsLogicSystem::GetInstance()->RegisterGetter( conn );

    return conn;
}
