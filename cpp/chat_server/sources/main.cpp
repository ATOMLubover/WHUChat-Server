#define BOOST_BEAST_DEBUG

#include "include/chat_server.hpp"

#include <iostream>

int main( int argc, const char** argv )
{
    // std::make_shared<ChatServer>()->Run();

    try
    {
        std::make_shared<ChatServer>()->Run();
    }
    catch ( std::exception& exp )
    {
        std::cout << "main函数处异常：" << exp.what() << std::endl;
        return -1;
    }

    return 0;
}