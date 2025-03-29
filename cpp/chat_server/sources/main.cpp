#include "gate_server.hpp"

#include <iostream>

int main( int argc, const char** argv )
{
    try
    {
        std::make_shared<GateServer>()->Run();
    }
    catch ( std::exception& exp )
    {
        std::cout << "main函数处异常：" << exp.what() << std::endl;
    }

    return 0;
}