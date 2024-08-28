#include <sys/socket.h>

#include <chrono>
#include <future>
#include <iostream>
#include <vector>

#include "sockpp/tcp_connector.h"

using namespace sockpp;

size_t pingpang(size_t times) {
    tcp_connector conn;
    conn.connect({"127.0.0.1", 1234});
    constexpr size_t size = 1e1;
    auto buffer = new std::vector<uint8_t>(size);
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < times; i++) {
        conn.write(buffer->data(), size);
        conn.read_n(buffer->data(), size);
    }
    auto end = std::chrono::high_resolution_clock::now();
    delete buffer;
    auto duration = end - start;
    return std::chrono::duration_cast<milliseconds>(duration).count();
}

int main(int argc, char* argv[]) {
    sockpp::initialize();
    constexpr size_t clientSize = 10;
    constexpr size_t pingpangTimes = 1000;
    std::vector<std::future<size_t>> v;
    for (int i = 0; i < clientSize; i++) {
        v.push_back(std::async(pingpang, pingpangTimes));
    }
    size_t sum = 0;
    for (auto& i : v) {
        sum += i.get();
    }
    std::cout << sum / clientSize << "ms" << std::endl;
    return 0;
}
