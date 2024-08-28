#pragma once

#include <cstddef>
#include <cstdint>

namespace frame {
template <typename Socket>
class Connection
{
public:
    int fileDiscription();
    void setNonBlocking();
    size_t peerRecvBufferSize();
    size_t peerSendBufferSize();
    int recv(uint8_t* info, size_t len);
    int send(uint8_t* info, size_t len);
};

template <typename Socket>
class Acceptor
{
public:
    int fileDiscription();
    Connection<Socket> accept();
    template <typename... Args>
    void open(const Args&... address);
};

}  // namespace frame
