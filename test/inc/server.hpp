#pragma once

#include <sys/socket.h>

#include <vector>

#include "abstract.hpp"
#include "channel.hpp"
#include "keeper.hpp"
#include "reactor.hpp"
#include "threadPool.hpp"

namespace frame {
template <typename Socket>
class Server
{
public:
    Server();
    ~Server();

    template <typename... Args>
    void open(const Args&...);
    void writeInfo(int fd, std::string_view response);
    virtual void readInfo(int fd, std::queue<std::vector<uint8_t>>& requests);

private:
    ThreadPool threadPool;
    Acceptor<Socket> acceptor;
    Reactor mainReactor, subReactor;
    Keeper<int, Channel<Socket>> channels;

    void tackleConnect();
    void tackleClean(int fd);
    void tackleDisconnect(int fd);
    void tackleRecvFeasible(int fd);
    void tackleSendFeasible(int fd);
    void readyToRecv(const Reactor::Handles&);
    void readyToSend(const Reactor::Handles&);
};

}  // namespace frame

namespace frame {
template <typename Socket>
Server<Socket>::Server() : threadPool{std::thread::hardware_concurrency()} {
    mainReactor.appendEvent(Reactor::recvFeasible);
    mainReactor.setRecv([this](const Reactor::Handles&) { tackleConnect(); });
    subReactor.setTrigger(Reactor::edge);
    subReactor.appendEvent(Reactor::sendShut);
    subReactor.appendEvent(Reactor::recvShut);
    subReactor.appendEvent(Reactor::recvFeasible);
    subReactor.appendEvent(Reactor::sendFeasible);
    subReactor.setClean([this](int fd) {
        tackleClean(fd);
    });
    subReactor.setDisconnect([this](int fd) { tackleDisconnect(fd); });
    subReactor.setRecv([this](const Reactor::Handles& handles) { readyToRecv(handles); });
    subReactor.setSend([this](const Reactor::Handles& handles) { readyToSend(handles); });
}

template <typename Socket>
Server<Socket>::~Server() {
    threadPool.drain();
}

template <typename Socket>
template <typename... Args>
void Server<Socket>::open(const Args&... args) {
    acceptor.open(args...);
    mainReactor.enroll({acceptor.fileDiscription()});
}

template <typename Socket>
void Server<Socket>::tackleConnect() {
    auto connection = acceptor.accept();
    int fd = connection.fileDiscription();
    connection.setNonBlocking();
    channels.obtain(fd, {std::move(connection)});
    subReactor.enroll(fd);
}

template <typename Socket>
void Server<Socket>::tackleDisconnect(int fd) {
    shutdown(fd, SHUT_WR);
}

template <typename Socket>
void Server<Socket>::tackleClean(int fd) {
    subReactor.logout(fd);
    channels.discard(fd);
}

template <typename Socket>
void Server<Socket>::readyToRecv(const Reactor::Handles& handles) {
    threadPool.lock();
    for (auto& fd : handles) {
        auto&& tmp = channels.lend(fd);
        if (not tmp.exist()) {
            continue;
        }
        Channel<Socket>& channel = tmp.peerValue();
        if (not channel.tryReadyRecv()) {
            continue;
        }
        threadPool.append([this, fd]() { tackleRecvFeasible(fd); });
    }
    threadPool.distribute();
    threadPool.unlock();
}

template <typename Socket>
void Server<Socket>::readyToSend(const Reactor::Handles& handles) {
    threadPool.lock();
    for (auto& fd : handles) {
        auto&& tmp = channels.lend(fd);
        if (not tmp.exist()) {
            continue;
        }
        Channel<Socket>& channel = tmp.peerValue();
        if (not channel.tryReadySend()) {
            continue;
        }
        threadPool.append([this, fd]() { tackleSendFeasible(fd); });
    }
    threadPool.distribute();
    threadPool.unlock();
}

template <typename Socket>
void Server<Socket>::tackleRecvFeasible(int fd) {
    auto&& tmp = channels.lend(fd);
    if (not tmp.exist()) {
        return;
    }
    Channel<Socket>& channel = tmp.peerValue();
    channel.recv([this](int fd, std::queue<std::vector<uint8_t>>& requests) {
        readInfo(fd, requests);
    });
}

template <typename Socket>
void Server<Socket>::tackleSendFeasible(int fd) {
    auto&& tmp = channels.lend(fd);
    if (not tmp.exist()) {
        return;
    }
    Channel<Socket>& channel = tmp.peerValue();
    channel.trySendRest();
}

template <typename Socket>
void Server<Socket>::writeInfo(int fd, std::string_view response) {
    auto&& tmp = channels.lend(fd);
    if (not tmp.exist()) {
        return;
    }
    Channel<Socket>& channel = tmp.peerValue();
    if (not channel.trySend(response)) {
        subReactor.refresh(fd);
    }
}

template <typename Socket>
void Server<Socket>::readInfo(int fd, std::queue<std::vector<uint8_t>>& requests) {}

}  // namespace frame
