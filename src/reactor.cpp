#include "reactor.hpp"

#include <sys/eventfd.h>

#include "channel.hpp"

using namespace localSocket;

Reactor::Reactor()
    : disconnectCallback([](int) {}), readCallback([](int) {}), writeCallback([](int) {}) {
    stopEvent = eventfd(0, 0);
    react = std::thread(&Reactor::work, this);
    enroll({stopEvent, Channel::readFeasible});
}

Reactor::~Reactor() {
    eventfd_write(stopEvent, 1);
    react.join();
}

void Reactor::setCloseCallback(std::function<void(int)>&& callback) {
    this->closeCallback = callback;
}

void Reactor::setDisconnectCallback(std::function<void(int)>&& callback) {
    this->disconnectCallback = callback;
}

void Reactor::setReadCallback(std::function<void(int)>&& callback) {
    this->readCallback = callback;
}

void Reactor::setWriteCallback(std::function<void(int)>&& callback) {
    this->writeCallback = callback;
}

void Reactor::work() {
    bool stop = false;
    auto handle = [&](const Channel& channel) {
        if (channel.fd == stopEvent) {
            stop = true;
            return;
        }
        if (channel.event == Channel::closed)
            closeCallback(channel.fd);
        if (channel.event == Channel::disconnected)
            disconnectCallback(channel.fd);
        if (channel.event == Channel::readFeasible)
            readCallback(channel.fd);
        if (channel.event == Channel::writeFeasible)
            writeCallback(channel.fd);
    };
    while (!stop) {
        auto&& channels = wait();
        for (int i = 0; i < channels.size() && !stop; i++) {
            handle(channels[i]);
        }
    }
}
