#include <sockpp/unix_acceptor.h>
#include <sockpp/unix_stream_socket.h>

#include <chrono>
#include <string_view>
#include <thread>

#include "abstract.hpp"
#include "server.hpp"

using namespace frame;
using namespace sockpp;

template <>
class SocketAbstract<sockpp::unix_socket>
{
public:
    SocketAbstract(sockpp::unix_socket&& socket) : socket(std::move(socket)) {}

    SocketAbstract(SocketAbstract<sockpp::unix_socket>&& socketAbstract)
        : socket(std::move(socketAbstract.socket)) {
        recvBufferSize = socket.recv_buffer_size().value();
        sendBufferSize = socket.send_buffer_size().value();
    }

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
class AcceptorAbstract<sockpp::unix_socket>
{
public:
    void open(const std::string& address) { acceptor.open(address); }

    int fileDiscription() { return acceptor.handle(); }

    SocketAbstract<sockpp::unix_socket> accept() { return acceptor.accept().release(); }

private:
    sockpp::unix_acceptor acceptor;
};

using EchoServer = Server<sockpp::unix_socket>;

template <>
void EchoServer::readInfo(int fd, std::string_view query, std::vector<uint8_t>& buffer) {
    writeInfo(fd, query);
    buffer.clear();
}

int main(int argc, char* argv[]) {
    sockpp::initialize();
    EchoServer echoServer("\0sock");
    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
    return 0;
}
