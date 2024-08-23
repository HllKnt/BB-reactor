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
    Acceptor<Socket> acceptor;
    Keeper<int, Connection<Socket>> connections;
    Keeper<ValveHandle, Valve> valves;
    using Buffer = std::vector<uint8_t>;
    Keeper<int, std::unique_ptr<Buffer>> recvBuffers;
    Keeper<int, std::unique_ptr<std::queue<Buffer>>> sendBuffers;

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
    mainReactor.setClean([this](int fd) { tackleDisconnect(fd); });
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
    int fd = connection.fileDiscription();
    connection.setNonBlocking();
    valves.obtain({fd, ValveHandle::Event::readFeasible}, {Valve::State::open});
    valves.obtain({fd, ValveHandle::Event::writeFeasible}, {Valve::State::shut});
    recvBuffers.obtain(fd, std::unique_ptr<Buffer>{new Buffer});
    sendBuffers.obtain(fd, std::unique_ptr<std::queue<Buffer>>{new std::queue<Buffer>});
    connections.obtain(fd, std::move(connection));
    mainReactor.enroll(fd);
    subReactor.enroll(fd);
    subReactor.setEdgeTrigger(fd);
    subReactor.enroll({fd, ValveHandle::Event::writeFeasible});
    subReactor.enroll({fd, ValveHandle::Event::readFeasible});
}

template <typename Socket>
void Server<Socket>::tackleDisconnect(int fd) {
    mainReactor.logout(fd);
    subReactor.logout(fd);
    valves.discard({fd, ValveHandle::Event::readFeasible});
    valves.discard({fd, ValveHandle::Event::writeFeasible});
    recvBuffers.discard(fd);
    sendBuffers.discard(fd);
    connections.discard(fd);
}

template <typename Socket>
void Server<Socket>::tackleNormalIO(const Reactor::ValveHandles& handles) {
    threadPool.lock();
    for (auto& handle : handles) {
        auto&& valve = valves.lend(handle);
        if (not valve.exist() || not valve->tryTurnOn()) {
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
    Connection<Socket>& socket = connection.peerValue();
    Buffer& buffer = *recvBuffer.peerValue();
    while (valve->tryStartUp()) {
        size_t infoSize = 0;
        ioctl(fd, FIONREAD, &infoSize);
        if (infoSize <= 0) {
            continue;
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
    Connection<Socket>& socket = connection.peerValue();
    std::queue<Buffer>& buffers = *sendBuffer.peerValue();
    size_t maxSize = socket.peerSendBufferSize();
    while (valve->tryStartUp()) {
        Buffer& buffer = buffers.front();
        int writtenSize = socket.send(buffer.data(), std::min(maxSize, buffer.size()));
        writtenSize = std::max(0, writtenSize);
        if (writtenSize < buffer.size()) {
            buffer.erase(buffer.begin(), buffer.begin() + writtenSize);
        }
        else {
            buffers.pop();
        }
        valve->tryDisable([&]() -> bool { return buffers.empty(); });
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
    Connection<Socket>& socket = connection.peerValue();
    std::queue<Buffer>& buffers = *sendBuffer.peerValue();
    uint8_t* info = (uint8_t*)reply.data();
    size_t infoSize = reply.size();
    if (valve->peerState() == Valve::State::shut) {
        size_t maxSize = socket.peerSendBufferSize();
        int writtenSize = socket.send(info, std::min(maxSize, infoSize));
        writtenSize = std::max(0, writtenSize);
        if (writtenSize >= infoSize) {
            return;
        }
        info = info + writtenSize;
        infoSize -= writtenSize;
        buffers.emplace(info, info + infoSize);
    }
    else {
        buffers.emplace(info, info + infoSize);
    }
    if (valve->tryEnable([&]() { return buffers.empty(); })) {
        subReactor.reflash(fd);
    }
}

template <typename Socket>
void Server<Socket>::readInfo(int fd, std::string_view query, std::vector<uint8_t>& buffer) {}

}  // namespace frame
