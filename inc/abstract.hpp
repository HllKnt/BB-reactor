#pragma once

#include <cstdint>
#include <string>

namespace frame {
template <typename Socket>
class SocketAbstract
{
public:
    virtual void setNonBlocking() = 0;
    virtual int fileDiscription() = 0;
    virtual size_t peerRecvBufferSize() = 0;
    virtual size_t peerSendBufferSize() = 0;
    virtual int recv(uint8_t* info, size_t len) = 0;
    virtual int send(uint8_t* info, size_t len) = 0;
};

template <typename Socket>
class AcceptorAbstract
{
public:
    virtual int fileDiscription() = 0;
    virtual SocketAbstract<Socket> accept() = 0;
    virtual void open(const std::string& address) = 0;
};

}  // namespace frame
