#include <sockpp/socket.h>
#include <sys/epoll.h>
#include <sys/socket.h>

#include <thread>
#include <vector>

#include "sockpp/tcp_connector.h"

using namespace sockpp;

int main(int argc, char *argv[]) {
    initialize();
    std::vector<tcp_connector> connects(100);
    string requery{"hi"};
    string reqly;
    reqly.resize(requery.size());
    std::vector<std::thread> threads;
    for (auto &connect : connects) {
        threads.emplace_back([&]() {
            connect.connect({"127.0.0.1", 1145});
            for (int i = 0; i < 1000; i++) {
                connect.send(requery.data(), requery.size());
                connect.recv(reqly.data(), reqly.size());
            }
        });
    }
    for (auto &thread : threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    return 0;
}
