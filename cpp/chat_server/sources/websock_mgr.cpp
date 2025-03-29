#include "websock_mgr.hpp"

#include "websock_conn.hpp"

#include <iostream>

WebsockMgr::~WebsockMgr()
{
    std::cout << "WebsockMgr被析构" << std::endl;
}

void WebsockMgr::AddConn( std::shared_ptr<WebsockConn> conn )
{
    map_conn.insert( std::make_pair( conn->GetUid(), conn ) );
    std::cout << "WebsockConn No." << conn->GetUid() << " 被保留" << std::endl;
}

void WebsockMgr::RmvConn( const std::string& uuid )
{
    if ( map_conn.erase( uuid ) != 0 )
        std::cout << "WebsockConn No." << uuid << " 停止保留" << std::endl;
}

WebsockMgr::WebsockMgr()
{
    std::cout << "WebsockMgr构造" << std::endl;
}