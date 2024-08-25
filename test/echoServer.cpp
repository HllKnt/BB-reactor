#include <sockpp/unix_acceptor.h>
#include <sockpp/unix_stream_socket.h>

#include <chrono>
#include <queue>
#include <string_view>
#include <thread>

#include "abstract.hpp"
#include "server.hpp"

using namespace frame;
using namespace sockpp;

template <>
class Connection<sockpp::unix_socket>
{
public:
    Connection(sockpp::unix_socket&& socket) : socket(std::move(socket)) {
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
    sockpp::unix_socket socket;
};

template <>
class Acceptor<sockpp::unix_socket>
{
public:
    void open(const std::string& address) { acceptor.open(address); }

    int fileDiscription() { return acceptor.handle(); }

    Connection<sockpp::unix_socket> accept() { return acceptor.accept().release(); }

private:
    sockpp::unix_acceptor acceptor;
};

using EchoServer = Server<sockpp::unix_socket>;

template <>
void EchoServer::readInfo(int fd, std::queue<std::vector<uint8_t>>& requests) {
    uint8_t* info = requests.front().data();
    size_t infoSize = requests.front().size();
    writeInfo(fd, {(char*)info, infoSize});
    requests.pop();
}

int main(int argc, char* argv[]) {
    sockpp::initialize();
    EchoServer echoServer("\0sock");
    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
    return 0;
}
