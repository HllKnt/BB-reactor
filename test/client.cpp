#include <chrono>
#include <iostream>

#include "sockpp/unix_connector.h"

using namespace sockpp;

int main(int argc, char* argv[]) {
    sockpp::initialize();
    unix_connector conn;
    conn.connect(unix_address("\0sock"));
    std::cout << "connected to " << conn.peer_address() << std::endl;
    constexpr size_t size = 1e8;
    auto buffer = new std::vector<uint8_t>(size);
    auto start = std::chrono::high_resolution_clock::now();
    int writeSize = conn.write(buffer->data(), size).value();
    int readSize = conn.read_n(buffer->data(), size).value();
    auto end = std::chrono::high_resolution_clock::now();
    delete buffer;
    auto duration = end - start;
    std::cout << "send: " << writeSize << " Bytes" << std::endl;
    std::cout << "recv: " << readSize << " Bytes" << std::endl;
    std::cout << "time cost: " << std::chrono::duration_cast<milliseconds>(duration).count()
              << "ms" << std::endl;
    return 0;
}
