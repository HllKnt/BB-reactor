#include <sys/socket.h>
#include <chrono>
#include <iostream>

#include "sockpp/tcp_connector.h"

using namespace sockpp;

int main(int argc, char* argv[]) {
    sockpp::initialize();
    tcp_connector conn;
    conn.connect({"127.0.0.1", 1234});
    std::cout << "connected to " << conn.peer_address() << std::endl;
    constexpr size_t size = 1e6;
    auto buffer = new std::vector<uint8_t>(size);
    auto start = std::chrono::high_resolution_clock::now();
    int writeSize = conn.write(buffer->data(), size).value();
    int readSize = conn.read_n(buffer->data(), size).value();
    auto end = std::chrono::high_resolution_clock::now();
    delete buffer;
    conn.shutdown(SHUT_WR);
    auto duration = end - start;
    std::cout << "send: " << writeSize << " Bytes" << std::endl;
    std::cout << "recv: " << readSize << " Bytes" << std::endl;
    std::cout << "time cost: " << std::chrono::duration_cast<milliseconds>(duration).count()
              << "ms" << std::endl;
    return 0;
}
