#include "reactor.hpp"

using namespace localSocket;

Reactor::Reactor()
    : disconnectCallback([](int) {}), readCallback([](int) {}), writeCallback([](int) {}) {
    react = std::thread(&Reactor::work, this);
}

Reactor::~Reactor() {
    this->endWaiting();
    react.join();
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
    auto tackle = [this](Channel& channel) {
        int fd = channel.getFD();
        if (channel.getEvent() == Channel::disconnect) {
            Channel channel;
            channel.setFD(fd);
            remove(channel);
            disconnectCallback(fd);
        }
        else if (channel.getEvent() == Channel::input) {
            readCallback(fd);
        }
        else {
            writeCallback(fd);
        }
    };
    while (!isWaitOver()) {
        auto&& res = wait();
        for (auto i : res) tackle(i);
    }
}
