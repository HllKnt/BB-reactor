#include "server.hpp"

#include <chrono>
#include <thread>

using namespace nlohmann;

int main(int argc, char *argv[]) {
    json resource = {
        {"pi", 3.14},
        {"happy", true},
        {"name", "Niels"},
        {"nothing", nullptr},
        {"answer", {{"everything", 42}}},
        {"list", {1, 0, 2}},
        {"object", {{"currency", "USD"}, {"value", 42.99}}}
    };
    localSocket::Server server("\0server.sock");
    server.addResource("test", resource);
    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
    return 0;
}
