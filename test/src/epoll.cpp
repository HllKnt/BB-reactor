#include "epoll.hpp"

#include <sys/epoll.h>
#include <unistd.h>

#include <vector>

using namespace frame;

Epoll::Epoll()
    : self{epoll_create1(EPOLL_CLOEXEC)}, buffer{new std::vector<epoll_event>(1024)} {}

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

auto Epoll::wait() -> EpollEevents {
    int size = epoll_wait(self, buffer->data(), buffer->capacity(), -1);
    EpollEevents res;
    for (int i = 0; i < size; i++) {
        int fd = (*buffer)[i].data.fd;
        uint32_t event = (*buffer)[i].events;
        res.emplace_back((int)fd, (uint32_t)event);
    }
    return res;
}
