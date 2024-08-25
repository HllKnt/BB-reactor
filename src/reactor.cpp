#include "reactor.hpp"

#include <sys/epoll.h>
#include <sys/eventfd.h>

using namespace frame;
Reactor::Reactor()
    : over{false},
      stop{eventfd(0, 0)},
      react{&Reactor::wait, this},
      lock{1},
      clean{[](int) {}},
      normalIO{[](const ValveHandles&) {}} {
    epoll.add(stop, EPOLLIN);
    epoll.setEdgeTrigger(stop);
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
    constexpr uint32_t null = 0;
    epoll.add(fd, null);
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

void Reactor::reflash(int fd) {
    lock.acquire();
    if (not epoll.exist(fd)) {
        lock.release();
        return;
    }
    uint32_t event = epoll.peerEvent(fd);
    epoll.mod(fd, event);
    lock.release();
}

void Reactor::enroll(const ValveHandle& handle) {
    int fd = handle.fd;
    uint32_t event = handle.event;
    lock.acquire();
    if (epoll.exist(fd) && (epoll.peerEvent(fd) & event)) {
        lock.release();
        return;
    }
    if (not epoll.exist(fd)) {
        epoll.add(fd, event);
        lock.release();
        return;
    }
    event |= epoll.peerEvent(fd);
    epoll.mod(fd, event);
    lock.release();
}

void Reactor::logout(const ValveHandle& handle) {
    int fd = handle.fd;
    uint32_t event = handle.event;
    lock.acquire();
    if (not epoll.exist(fd) || not(epoll.peerEvent(fd) & event)) {
        lock.release();
        return;
    }
    event = epoll.peerEvent(fd) & (~event);
    epoll.mod(fd, event);
    lock.release();
}

void Reactor::setEdgeTrigger(int fd) {
    lock.acquire();
    if (not epoll.exist(fd)) {
        lock.release();
        return;
    }
    epoll.setEdgeTrigger(fd);
    lock.release();
}

void Reactor::setLevelTrigger(int fd) {
    lock.acquire();
    if (not epoll.exist(fd)) {
        lock.release();
        return;
    }
    epoll.setLevelTrigger(fd);
    lock.release();
}

void Reactor::setClean(const CleanCallback& callback) { clean = callback; }

void Reactor::setNormalIO(const NormalIOCallback& callback) { normalIO = callback; }

void Reactor::wait() {
    while (true) {
        auto&& epollEvents = epoll.wait();
        if (over) {
            return;
        }
        normalIO(filter(epollEvents));
    }
}

auto Reactor::filter(const Epoll::EpollEevents& epollEvents) -> ValveHandles {
    ValveHandles res;
    for (auto& [fd, event] : epollEvents) {
        if (event & EPOLLHUP) {
            clean(fd);
            continue;
        }
        if (event & EPOLLIN) {
            res.emplace_back(fd, ValveHandle::Event::recvFeasible);
        }
        if (event & EPOLLOUT) {
            res.emplace_back(fd, ValveHandle::Event::sendFeasible);
        }
    }
    return res;
}
