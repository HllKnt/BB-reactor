#include "client.hpp"

#include <sockpp/socket.h>

#include <chrono>
#include <iostream>

using namespace nlohmann;

int main(int argc, char *argv[]) {
    localSocket::Client client("\0server.sock");
    using namespace std::chrono;
    auto start = system_clock::now();
    client.requestGet({{"test", {{"answer"}}, "everything"}, {"test", {}, "pi"}});
    client.getResponse();
    client.requestPost({{"test", {{"answer"}}, "everything", 6}, {"test", {}, "pi", 6.28}});
    client.getResponse();
    client.requestGet({{"test", {{"answer"}}, "everything"}, {"test", {}, "pi"}});
    client.getResponse();
    auto end = system_clock::now();
    auto duration = duration_cast<microseconds>(end - start);
    std::cout <<  "Spent" << double(duration.count()) * microseconds::period::num / microseconds::period::den << " seconds." << std::endl;
    return 0;
}
