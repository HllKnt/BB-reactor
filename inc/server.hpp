#pragma once

#include <sys/ioctl.h>

#include <memory>

#include "abstract.hpp"
#include "keeper.hpp"
#include "reactor.hpp"
#include "threadPool.hpp"
#include "valve.hpp"

namespace frame {
template <typename Socket>
class Server
{
public:
    Server(const std::string& address);
    ~Server();

    void writeInfo(int fd, std::string_view reply);
    virtual void readInfo(int fd, std::string_view query, std::vector<uint8_t>& buffer);

private:
    ThreadPool threadPool;
    Reactor mainReactor, subReactor;
    AcceptorAbstract<Socket> acceptor;
    Keeper<int, SocketAbstract<Socket>> connections;
    Keeper<ValveHandle, Valve> valves;
    using Buffer = std::vector<uint8_t>;
    Keeper<int, std::unique_ptr<Buffer>> recvBuffers, sendBuffers;

    void tackleConnect();
    void tackleDisconnect(int fd);
    void tackleReadFeasible(const ValveHandle&);
    void tackleWriteFeasible(const ValveHandle&);
    void tackleNormalIO(const Reactor::ValveHandles&);
};

}  // namespace frame

namespace frame {
template <typename Socket>
Server<Socket>::Server(const std::string& address)
    : threadPool{std::thread::hardware_concurrency()} {
    mainReactor.setNormalIO([this](const Reactor::ValveHandles&) { tackleConnect(); });
    subReactor.setClean([this](int fd) { tackleDisconnect(fd); });
    subReactor.setNormalIO([this](const Reactor::ValveHandles& handles) {
        tackleNormalIO(handles);
    });
    acceptor.open(address);
    mainReactor.enroll({acceptor.fileDiscription(), ValveHandle::Event::readFeasible});
}

template <typename Socket>
Server<Socket>::~Server() {
    threadPool.drain();
}

template <typename Socket>
void Server<Socket>::tackleConnect() {
    auto&& connection = acceptor.accept();
    connection.setNonBlocking();
    int fd = connection.fileDiscription();
    connections.obtain(fd, std::move(connection));
    sendBuffers.obtain(fd, std::make_unique<Buffer>());
    recvBuffers.obtain(fd, std::make_unique<Buffer>());
    valves.obtain({fd, ValveHandle::Event::writeFeasible}, {Valve::State::shut});
    valves.obtain({fd, ValveHandle::Event::readFeasible}, {Valve::State::open});
    subReactor.enroll(fd);
    subReactor.setEdgeTrigger(fd);
    subReactor.enroll({fd, ValveHandle::Event::writeFeasible});
    subReactor.enroll({fd, ValveHandle::Event::readFeasible});
}

template <typename Socket>
void Server<Socket>::tackleDisconnect(int fd) {
    subReactor.logout(fd);
    valves.discard({fd, ValveHandle::Event::writeFeasible});
    valves.discard({fd, ValveHandle::Event::readFeasible});
    sendBuffers.discard(fd);
    recvBuffers.discard(fd);
    connections.discard(fd);
}

template <typename Socket>
void Server<Socket>::tackleNormalIO(const Reactor::ValveHandles& handles) {
    threadPool.lock();
    for (auto& handle : handles) {
        auto&& valve = valves.lend(handle);
        if (not valve->tryTurnOn()) {
            continue;
        }
        if (handle.event == ValveHandle::Event::readFeasible) {
            threadPool.append([this, handle]() { tackleReadFeasible(handle); });
        }
        else if (handle.event == ValveHandle::Event::writeFeasible) {
            threadPool.append([this, handle]() { tackleWriteFeasible(handle); });
        }
    }
    threadPool.distribute();
    threadPool.unlock();
}

template <typename Socket>
void Server<Socket>::tackleReadFeasible(const ValveHandle& handle) {
    int fd = handle.fd;
    auto&& connection = connections.lend(fd);
    auto&& recvBuffer = recvBuffers.lend(fd);
    auto&& valve = valves.lend(handle);
    if (not connection.exist() || not recvBuffer.exist() || not valve.exist()) {
        return;
    }
    SocketAbstract<Socket>& socket = connection.peerValue();
    std::vector<uint8_t>& buffer = *recvBuffer.peerValue();
    while (valve->tryStartUp()) {
        size_t infoSize = 0;
        ioctl(fd, FIONREAD, &infoSize);
        if (infoSize <= 0) {
            return;
        }
        size_t start = buffer.size();
        buffer.resize(start + infoSize);
        socket.recv(buffer.data() + start, infoSize);
        readInfo(fd, {(const char*)(buffer.data() + start), infoSize}, buffer);
    }
}

template <typename Socket>
void Server<Socket>::tackleWriteFeasible(const ValveHandle& handle) {
    int fd = handle.fd;
    auto&& connection = connections.lend(fd);
    auto&& sendBuffer = sendBuffers.lend(fd);
    auto&& valve = valves.lend(handle);
    if (not connection.exist() || not sendBuffer.exist() || not valve.exist()) {
        return;
    }
    SocketAbstract<Socket>& socket = connection.peerValue();
    std::vector<uint8_t>& buffer = *sendBuffer.peerValue();
    while (valve->tryStartUp()) {
        size_t maxSize = socket.peerSendBufferSize();
        int writtenSize = socket.send(buffer.data(), std::min(maxSize, buffer.size()));
        buffer.erase(buffer.begin(), buffer.begin() + std::max(0, writtenSize));
        valve->tryDisable([&]() -> bool { return buffer.empty(); });
    }
}

template <typename Socket>
void Server<Socket>::writeInfo(int fd, std::string_view reply) {
    auto&& connection = connections.lend(fd);
    auto&& sendBuffer = sendBuffers.lend(fd);
    auto&& valve = valves.lend({fd, ValveHandle::Event::writeFeasible});
    if (not connection.exist() || not sendBuffer.exist() || not valve.exist()) {
        return;
    }
    SocketAbstract<Socket>& socket = connection.peerValue();
    std::vector<uint8_t>& buffer = *sendBuffer.peerValue();
    uint8_t* info = (uint8_t*)reply.data();
    size_t infoSize = reply.size();
    if (valve->peerState() == Valve::State::shut) {
        size_t maxSize = socket.peerSendBufferSize();
        int writtenSize = socket.send(info, std::min(maxSize, infoSize));
        if (writtenSize == infoSize) {
            return;
        }
        buffer.insert(buffer.begin(), info + std::max(0, writtenSize), info + infoSize);
    }
    else {
        buffer.reserve(buffer.size() + infoSize);
        buffer.insert(buffer.begin() + buffer.size(), info, info + infoSize);
    }
    if (valve->tryEnable([&]() { return buffer.empty(); })) {
        subReactor.reflash(fd);
    }
}

template <typename Socket>
void Server<Socket>::readInfo(int fd, std::string_view query, std::vector<uint8_t>& buffer) {}

}  // namespace frame
