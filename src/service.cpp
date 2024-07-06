#include "service.hpp"

#include <semaphore.h>
#include <sockpp/result.h>
#include <sockpp/socket.h>

#include <cstddef>
#include <memory>
#include <utility>

using namespace localSocket;

Service::Service(const std::string& address) : threadPool(8) {
    sockpp::initialize();
    acceptor.open(address);
    mainReactor.setReadCallback([this](int) { addClient(); });
    Channel channel;
    channel.setFD(acceptor.handle());
    channel.setRead();
    mainReactor.enroll(channel);
    subReactor.setDisconnectCallback([this](int fd) { removeClient(fd); });
    subReactor.setReadCallback([this](int fd) {
        threadPool.append([this, fd]() { readClient(fd); });
    });
    subReactor.setWriteCallback([this](int fd) {
        threadPool.append([this, fd]() { writeClient(fd); });
    });
}

Service::~Service() {
    mainReactor.endWaiting();
    subReactor.endWaiting();
    threadPool.drain();
}

void Service::addClient() {
    auto client = acceptor.accept().release();
    client.set_non_blocking();
    int fd = client.handle();
    Channel channel;
    channel.setFD(fd);
    channel.setRead();
    channel.setWrite();
    channel.setET();
    subReactor.enroll(channel);
    writingRight.add(fd, std::make_unique<std::mutex>());
    writeBuffer.add(fd, std::deque<std::unique_ptr<std::string>>());
    clients.add(fd, std::move(client));
}

void Service::removeClient(int fd) {
    writingRight.remove(fd);
    writeBuffer.remove(fd);
    clients.remove(fd);
}

void Service::readClient(int fd) {
    auto client = clients.lend(fd);
    if (!client.isExist())
        return;
    std::unique_ptr<std::string> info(new std::string);
    size_t additionalSize = 256, start = 0, size;
    info->resize(additionalSize);
    do {
        size = client->recv(info->data() + start, additionalSize).value();
        start += size;
        if (size >= additionalSize)
            info->resize(info->size() + additionalSize);
    } while (size > 0);
    parse(fd, *info);
}

void Service::writeClient(int fd) {
    auto client = clients.lend(fd);
    auto buffer = writeBuffer.lend(fd);
    auto mutex = writingRight.lend(fd);
    if (!client.isExist() || !buffer.isExist() || !mutex.isExist())
        return;
    std::unique_lock<std::mutex> lock(**mutex);
    if (buffer->empty())
        return;
    size_t maxSize = client->send_buffer_size().value();
    do {
        auto&& info = buffer->front();
        size_t writtenSize, infoSize = info->size();
        if (infoSize > maxSize) {
            writtenSize = client->send(info->data(), maxSize).value();
        }
        else {
            writtenSize = client->send(info->data(), infoSize).value();
        }
        if (writtenSize < 0)
            break;
        if (writtenSize < infoSize) {
            info->erase(info->begin(), info->begin() + writtenSize);
        }
        else {
            buffer->pop_front();
        }
    } while (!buffer->empty());
}

void Service::inform(int fd, const std::string& info) {
    auto client = clients.lend(fd);
    auto buffer = writeBuffer.lend(fd);
    auto mutex = writingRight.lend(fd);
    if (!client.isExist() || !buffer.isExist() || !mutex.isExist())
        return;
    std::unique_lock<std::mutex> lock(**mutex);
    size_t maxSize = client->send_buffer_size().value(), infoSize = info.size(),
           writtenSize;
    if (infoSize > maxSize) {
        writtenSize = client->send(info.data(), maxSize).value();
    }
    else {
        writtenSize = client->send(info.data(), infoSize).value();
    }
    if (writtenSize < 0 || writtenSize >= infoSize)
        return;
    buffer->emplace_back(new std::string(info.begin() + writtenSize, info.end())); //
}

void Service::parse(int fd, const std::string& info) {
    inform(fd, info);  // echo
}
