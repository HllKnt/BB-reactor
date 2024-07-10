#include "client.hpp"

#include <sockpp/socket.h>

#include <chrono>
#include <iostream>

using namespace nlohmann;
using namespace localSocket;
using namespace protocol;

int main(int argc, char *argv[]) {
    localSocket::Client client("\0server.sock");
    using namespace std::chrono;
    client.request(RequestGet{.addresses = {{"test", {}}}});
    auto start = system_clock::now();
    auto res = client.getResponse();
    auto end = system_clock::now();
    auto duration = duration_cast<microseconds>(end - start);
    std::cout << "Spent"
              << 1000 * double(duration.count()) * microseconds::period::num /
                     microseconds::period::den
              << " microseconds." << std::endl;
    return 0;
}
