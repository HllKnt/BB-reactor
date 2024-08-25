#pragma once

#include <sys/ioctl.h>

#include <vector>

#include "abstract.hpp"
#include "channel.hpp"
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

    void writeInfo(int fd, std::string_view response);
    virtual void readInfo(int fd, std::queue<std::vector<uint8_t>>& requests);

private:
    ThreadPool threadPool;
    Acceptor<Socket> acceptor;
    Reactor mainReactor, subReactor;
    Keeper<int, Channel<Socket>> channels;

    void tackleConnect();
    void tackleDisconnect(int fd);
    void tackleReadFeasible(int fd);
    void tackleWriteFeasible(int fd);
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
    mainReactor.enroll({acceptor.fileDiscription(), ValveHandle::Event::recvFeasible});
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
    channels.obtain(fd, {std::move(connection)});
    mainReactor.enroll(fd);
    subReactor.enroll(fd);
    subReactor.setEdgeTrigger(fd);
    subReactor.enroll({fd, ValveHandle::Event::sendFeasible});
    subReactor.enroll({fd, ValveHandle::Event::recvFeasible});
}

template <typename Socket>
void Server<Socket>::tackleDisconnect(int fd) {
    mainReactor.logout(fd);
    subReactor.logout(fd);
    channels.discard(fd);
}

template <typename Socket>
void Server<Socket>::tackleNormalIO(const Reactor::ValveHandles& handles) {
    threadPool.lock();
    for (auto& handle : handles) {
        auto&& tmp = channels.lend(handle.fd);
        if (not tmp.exist()) {
            continue;
        }
        Channel<Socket>& channel = tmp.peerValue();
        if (not channel.tryTurnOnVavle(handle)) {
            continue;
        }
        if (handle.event == ValveHandle::Event::recvFeasible) {
            threadPool.append([this, handle]() { tackleReadFeasible(handle.fd); });
        }
        else if (handle.event == ValveHandle::Event::sendFeasible) {
            threadPool.append([this, handle]() { tackleWriteFeasible(handle.fd); });
        }
    }
    threadPool.distribute();
    threadPool.unlock();
}

template <typename Socket>
void Server<Socket>::tackleReadFeasible(int fd) {
    auto&& tmp = channels.lend(fd);
    if (not tmp.exist()) {
        return;
    }
    Channel<Socket>& channel = tmp.peerValue();
    channel.recv([this](int fd, std::queue<std::vector<uint8_t>>& buffer) {
        readInfo(fd, buffer);
    });
}

template <typename Socket>
void Server<Socket>::tackleWriteFeasible(int fd) {
    auto&& tmp = channels.lend(fd);
    if (not tmp.exist()) {
        return;
    }
    Channel<Socket>& channel = tmp.peerValue();
    channel.send();
}

template <typename Socket>
void Server<Socket>::writeInfo(int fd, std::string_view response) {
    auto&& tmp = channels.lend(fd);
    if (not tmp.exist()) {
        return;
    }
    Channel<Socket>& channel = tmp.peerValue();
    if (not channel.tryWriteInfo(response)) {
        subReactor.reflash(fd);
    }
}

template <typename Socket>
void Server<Socket>::readInfo(int fd, std::queue<std::vector<uint8_t>>& requests) {}

}  // namespace frame
