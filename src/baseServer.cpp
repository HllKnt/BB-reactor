#include "baseServer.hpp"

#include <semaphore.h>
#include <sockpp/result.h>
#include <sockpp/socket.h>

#include <algorithm>
#include <cstddef>
#include <memory>
#include <utility>

#include "sys/ioctl.h"

using namespace localSocket;

BaseServer::BaseServer(const std::string& address) : threadPool(8) {
    sockpp::initialize();
    acceptor.open(address);
    mainReactor.setReadCallback([this](int) { addClient(); });
    mainReactor.enroll({acceptor.handle(), Channel::readFeasible});
    subReactor.setReadCallback([this](int fd) {
        threadPool.append([this, fd]() { readClient(fd); });
    });
    subReactor.setWriteCallback([this](int fd) {
        threadPool.append([this, fd]() { writeClient(fd); });
    });
    subReactor.setCloseCallback([this](int fd) { removeClient(fd); });
}

void BaseServer::addClient() {
    auto client = acceptor.accept().release();
    client.set_non_blocking();
    int fd = client.handle();
    subReactor.enroll(Channel{fd, Channel::readFeasible}, Epoll::TriggerMode::edge);
    writtenMembers.add(fd, std::make_unique<Member>());
    clients.add(fd, std::move(client));
}

void BaseServer::removeClient(int fd) {
    writtenMembers.remove(fd);
    clients.remove(fd);
}

void BaseServer::readClient(int fd) {
    auto client = clients.lend(fd);
    if (!client.isExist())
        return;
    size_t size = 0;
    ioctl(client->handle(), FIONREAD, &size);
    std::unique_ptr<std::string> info(new std::string);
    info->resize(size);
    client->recv(info->data(), info->size()).value();
    handle(fd, *info);
}

void BaseServer::writeClient(int fd) {
    auto client = clients.lend(fd);
    auto member = writtenMembers.lend(fd);
    if (!client.isExist() || !member.isExist())
        return;
    auto&& [buffer, mutex] = **member;
    std::unique_lock<std::mutex> lock(mutex);
    size_t maxSize = client->send_buffer_size().value();
    while (!buffer.empty()) {
        auto&& info = buffer.front();
        if (info.empty())
            continue;
        size_t infoSize = info.size();
        int writtenSize = client->send(info.data(), std::min(infoSize, maxSize)).value();
        if (writtenSize < 0)
            return;
        if (writtenSize < infoSize)
            info.erase(writtenSize);
        else
            buffer.pop();
    }
    subReactor.remove({client->handle(), Channel::writeFeasible});
}

void BaseServer::inform(int fd, const std::string& info) {
    if (info.empty())
        return;
    auto client = clients.lend(fd);
    auto member = writtenMembers.lend(fd);
    if (!client.isExist() || !member.isExist())
        return;
    auto&& [buffer, mutex] = **member;
    std::unique_lock<std::mutex> lock(mutex);
    size_t maxSize = client->send_buffer_size().value(), infoSize = info.size();
    int writtenSize = client->send(info.data(), std::min(infoSize, maxSize)).value();
    if (writtenSize >= infoSize)
        return;
    if (writtenSize < 0)
        buffer.push(info);
    else
        buffer.push(info.substr(writtenSize));
    subReactor.enroll({client->handle(), Channel::writeFeasible}, Epoll::TriggerMode::edge);
}

void BaseServer::handle(int fd, const std::string& info) {
    // inform(fd, info);  // echo
}
