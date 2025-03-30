#include "status_server.hpp"

#include <memory>

int main( int argc, char* argv[] )
{
    std::make_shared<StatusServer>()->Run();

    return 0;
}