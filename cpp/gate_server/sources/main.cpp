#include "include/config_mgr.hpp"
#include "include/http_logic_mgr.hpp"
#include "include/asio_iocontext_pool.hpp"
#include "include/gate_server.hpp"

#include <iostream>
// #include <filesystem>

int main( int argc, char* argv[] )
{
    try
    {
        std::make_shared<GateServer>()->Run();
    }
    catch ( const std::exception& exp )
    {
        std::cout << "main函数调用gate_server处产生异常：" << exp.what() << std::endl;
    }

    return 0;
}