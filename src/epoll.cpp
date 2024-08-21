#include "epoll.hpp"

using namespace frame;

Epoll::Epoll() : self{epoll_create1(0)}, epollEvents{new std::vector<epoll_event>(1024)} {}

Epoll::~Epoll() { close(self); }

bool Epoll::exist(int fd) { return roster.contains(fd); }

uint32_t Epoll::peerEvent(int fd) { return roster.at(fd); }

void Epoll::add(int fd, uint32_t event) {
    roster.emplace(fd, event);
    epoll_event epollEvent = {};
    epollEvent.data.fd = fd;
    epollEvent.events = event;
    epoll_ctl(self, EPOLL_CTL_ADD, fd, &epollEvent);
}

void Epoll::mod(int fd, uint32_t event) {
    roster.at(fd) = event;
    epoll_event epollEvent = {};
    epollEvent.data.fd = fd;
    epollEvent.events = event;
    epoll_ctl(self, EPOLL_CTL_MOD, fd, &epollEvent);
}

void Epoll::del(int fd) {
    roster.erase(fd);
    epoll_ctl(self, EPOLL_CTL_DEL, fd, nullptr);
}

void Epoll::setEdgeTrigger(int fd) {
    auto& event = roster.at(fd);
    event |= EPOLLET;
    epoll_event epollEvent = {};
    epollEvent.data.fd = fd;
    epollEvent.events = event;
    epoll_ctl(self, EPOLL_CTL_MOD, fd, &epollEvent);
}

void Epoll::setLevelTrigger(int fd) {
    auto& event = roster.at(fd);
    event &= ~EPOLLET;
    epoll_event epollEvent = {};
    epollEvent.data.fd = fd;
    epollEvent.events = event;
    epoll_ctl(self, EPOLL_CTL_MOD, fd, &epollEvent);
}

auto Epoll::wait() -> EpollEevents {
    int size = epoll_wait(self, epollEvents->data(), epollEvents->capacity(), -1);
    epollEvents->resize(size);
    EpollEevents res;
    for (auto& i : *epollEvents) {
        res.emplace_back(i.data.fd, i.events);
    }
    return res;
}
