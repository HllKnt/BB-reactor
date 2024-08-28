#include <netinet/in.h>
#include <sys/socket.h>

#include <chrono>
#include <queue>
#include <string_view>
#include <thread>

#include "abstract.hpp"
#include "server.hpp"
#include "sockpp/tcp_acceptor.h"
#include "sockpp/tcp_socket.h"

using namespace frame;
using namespace sockpp;

template <>
class Connection<tcp_socket>
{
public:
    Connection(tcp_socket&& socket) : socket(std::move(socket)) {
        recvBufferSize = this->socket.recv_buffer_size().value();
        sendBufferSize = this->socket.send_buffer_size().value();
    }

    Connection(Connection&& connection)
        : socket(std::move(connection.socket)),
          recvBufferSize{connection.recvBufferSize},
          sendBufferSize{connection.sendBufferSize} {}

    void setNonBlocking() { socket.set_non_blocking(); }

    int fileDiscription() { return socket.handle(); }

    size_t peerRecvBufferSize() { return recvBufferSize; }

    size_t peerSendBufferSize() { return sendBufferSize; }

    int recv(uint8_t* info, size_t len) { return socket.recv(info, len).value(); }

    int send(uint8_t* info, size_t len) { return socket.send(info, len).value(); }

private:
    size_t recvBufferSize;
    size_t sendBufferSize;
    tcp_socket socket;
};

template <>
class Acceptor<tcp_socket>
{
public:
    void open(const string& address, uint16_t port) { acceptor.open({address, port}); }

    int fileDiscription() { return acceptor.handle(); }

    Connection<tcp_socket> accept() { return acceptor.accept().release(); }

private:
    tcp_acceptor acceptor;
};

using Base = Server<tcp_socket>;

class EchoServer : public Base
{
public:
    void readInfo(int fd, std::queue<std::vector<uint8_t>>& requests);
};

void EchoServer::readInfo(int fd, std::queue<std::vector<uint8_t>>& requests) {
    auto& request = requests.back();
    uint8_t* info = request.data();
    size_t infoSize = request.size();
    writeInfo(fd, {(const char*)info, infoSize});
    request.clear();
}

int main(int argc, char* argv[]) {
    sockpp::initialize();
    EchoServer echoServer;
    echoServer.open(std::string{"127.0.0.1"}, 1234);
    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
    return 0;
}
