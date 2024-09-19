#include "reactor.hpp"

#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/socket.h>

using namespace frame;

Reactor::Reactor()
    : lock{1},
      over{false},
      defaultConfig{0},
      stop{eventfd(0, 0)},
      react{&Reactor::wait, this},
      clean{[](int) {}},
      disconnect([](int) {}),
      recv{[](const Handles&) {}},
      send{[](const Handles&) {}} {
    epoll.add(stop, EPOLLIN | EPOLLET);
}

Reactor::~Reactor() {
    over = true;
    constexpr uint8_t anything = 1;
    eventfd_write(stop, anything);
    if (react.joinable())
        react.join();
    close(stop);
}

void Reactor::enroll(int fd) {
    lock.acquire();
    if (epoll.exist(fd)) {
        lock.release();
        return;
    }
    epoll.add(fd, defaultConfig);
    lock.release();
}

void Reactor::logout(int fd) {
    lock.acquire();
    if (not epoll.exist(fd)) {
        lock.release();
        return;
    }
    epoll.del(fd);
    lock.release();
}

void Reactor::refresh(int fd) {
    lock.acquire();
    if (not epoll.exist(fd)) {
        lock.release();
        return;
    }
    uint32_t event = epoll.peerEvent(fd);
    epoll.mod(fd, event);
    lock.release();
}

void Reactor::appendEvent(Event event) { defaultConfig |= event; }

void Reactor::setTrigger(Trigger trigger) { defaultConfig |= trigger; }

void Reactor::setRecv(const Recv& callback) { recv = callback; }

void Reactor::setSend(const Send& callback) { send = callback; }

void Reactor::setClean(const Clean& callback) { clean = callback; }

void Reactor::setDisconnect(const Disconnect& callback) { disconnect = callback; }

void Reactor::wait() {
    while (true) {
        auto&& epollEvents = epoll.wait();
        if (over) {
            return;
        }
        filter(epollEvents);
    }
}

void Reactor::filter(const Epoll::EpollEevents& epollEvents) {
    Handles recvHandles, sendHandles;
    for (auto& [fd, event] : epollEvents) {
        if (event & EPOLLHUP) {
            clean(fd);
            continue;
        }
        if (event & EPOLLRDHUP) {
            disconnect(fd);
            continue;
        }
        if (event & EPOLLIN) {
            recvHandles.push_back(fd);
        }
        if (event & EPOLLOUT) {
            sendHandles.push_back(fd);
        }
    }
    if (not recvHandles.empty()) {
        this->recv(recvHandles);
    }
    if (not sendHandles.empty()) {
        this->send(sendHandles);
    }
}
