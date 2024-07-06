#include "client.hpp"

#include <sockpp/socket.h>

#include <iostream>

using namespace nlohmann;

int main(int argc, char *argv[]) {
    localSocket::Client client("\0server.sock");
    client.getResource({{"pi"}});
    std::cout << client.getResponse() << std::endl;
    client.postResource({{"pi"}}, 6.28);
    std::cout << client.getResponse() << std::endl;
    client.getResource({{"pi"}});
    std::cout << client.getResponse() << std::endl;
    return 0;
}
