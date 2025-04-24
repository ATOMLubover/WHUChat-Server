#include "http_mgr.hpp"

std::shared_ptr<CliHttpsConn> HttpMgr::CreateCliHttpConn()
{
    return std::shared_ptr<CliHttpsConn>();
}