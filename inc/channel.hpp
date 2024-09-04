#pragma once

#include <sys/ioctl.h>
#include <sys/socket.h>

#include <algorithm>
#include <cstddef>
#include <functional>
#include <memory>
#include <queue>
#include <vector>

#include "abstract.hpp"
#include "valve.hpp"

namespace frame {
template <typename Socket>
class Channel
{
public:
    using Buffer = std::queue<std::vector<uint8_t>>;
    using ReadInfo = std::function<void(int, Buffer&)>;

    Channel(Connection<Socket>&&);
    Channel(Channel&&);

    bool trySendRest();
    bool trySend(std::string_view);
    void recv(const ReadInfo&);
    bool tryReadyRecv();
    bool tryReadySend();

private:
    Valve recvValve, sendValve;
    Connection<Socket> connection;
    std::unique_ptr<Buffer> recvBuffer, sendBuffer;
    size_t nextSendStart;
};

template <typename Socket>
Channel<Socket>::Channel(Connection<Socket>&& connection)
    : connection{std::move(connection)},
      nextSendStart{0},
      recvValve{Valve::State::open},
      sendValve{Valve::State::shut},
      sendBuffer{new Buffer},
      recvBuffer{new Buffer} {}

template <typename Socket>
Channel<Socket>::Channel(Channel&& channel)
    : connection{std::move(channel.connection)},
      nextSendStart{0},
      recvValve{Valve::State::open},
      sendValve{Valve::State::shut},
      sendBuffer{new Buffer},
      recvBuffer{new Buffer} {}

template <typename Socket>
void Channel<Socket>::recv(const ReadInfo& readInfo) {
    size_t infoSize = 0;
    int fd = connection.fileDiscription();
    while (recvValve.tryStartUpWithAgain()) {
        ioctl(fd, FIONREAD, &infoSize);
        if (infoSize <= 0) {
            continue;
        }
        if (recvBuffer->empty() || not recvBuffer->back().empty()) {
            recvBuffer->emplace();
        }
        auto& buffer = recvBuffer->back();
        buffer.resize(infoSize);
        connection.recv(buffer.data(), infoSize);
        readInfo(fd, *recvBuffer);
    }
}

template <typename Socket>
bool Channel<Socket>::trySendRest() {
    const auto trySendWithAgain = [&]() -> bool {
        if (sendBuffer->empty()) {
            return false;
        }
        auto& buffer = sendBuffer->front();
        uint8_t* info = buffer.data() + nextSendStart;
        size_t infoSize = buffer.size() - nextSendStart;
        size_t maxSize = connection.peerSendBufferSize();
        int writtenSize = connection.send(info, std::min(maxSize, infoSize));
        writtenSize = std::max(0, writtenSize);
        if (writtenSize < infoSize) {
            nextSendStart += writtenSize;
            return false;
        }
        nextSendStart = 0;
        sendBuffer->pop();
        return true;
    };
    bool res;
    while (sendValve.tryStartUpWithAgain()) {
        while (trySendWithAgain()) {
        }
        res = sendValve.tryDisable([&]() -> bool { return sendBuffer->empty(); });
    }
    return res;
}

template <typename Socket>
bool Channel<Socket>::trySend(std::string_view reply) {
    size_t maxSize = connection.peerSendBufferSize();
    uint8_t* info = (uint8_t*)reply.data();
    size_t infoSize = reply.size();
    int writtenSize = 0;
    if (sendValve.peerState() == Valve::State::shut) {
        writtenSize = connection.send(info, std::min(maxSize, infoSize));
        writtenSize = std::max(0, writtenSize);
    }
    if (writtenSize >= infoSize) {
        return true;
    }
    info += writtenSize;
    infoSize -= writtenSize;
    sendBuffer->emplace(info, info + infoSize);
    if (sendValve.tryEnable([&]() { return sendBuffer->empty(); })) {
        return false;
    }
    return true;
}

template <typename Socket>
bool Channel<Socket>::tryReadyRecv() {
    return recvValve.tryTurnOnWithAgain();
}

template <typename Socket>
bool Channel<Socket>::tryReadySend() {
    return sendValve.tryTurnOnWithAgain();
}

}  // namespace frame
