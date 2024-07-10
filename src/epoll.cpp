#include "epoll.hpp"

#include <sys/epoll.h>

#include <mutex>

using namespace localSocket;

Epoll::Epoll() : events(new std::vector<epoll_event>(1024)) { self = epoll_create1(0); }

Epoll::~Epoll() { close(self); }

void Epoll::enroll(const Channel& channel, TriggerMode mode) {
    epoll_event epollEvent = {};
    epollEvent.events = channel.event;
    epollEvent.data.fd = channel.fd;
    if (mode == edge)
        epollEvent.events |= EPOLLET;
    auto enrolled = enrolledEvents(channel.fd);
    if (enrolled) {
        epollEvent.events |= enrolled;
        epoll_ctl(self, EPOLL_CTL_MOD, channel.fd, &epollEvent);
    }
    else {
        std::unique_lock<std::shared_mutex> writeLock(mutex);
        roster.insert(channel);
        epoll_ctl(self, EPOLL_CTL_ADD, channel.fd, &epollEvent);
    }
}

void Epoll::remove(const Channel& channel) {
    std::shared_lock<std::shared_mutex> readLock(mutex);
    if (roster.count(channel.fd) <= 0 || roster.find(channel) == roster.end())
        return;
    readLock.unlock();
    std::unique_lock<std::shared_mutex> writeLock(mutex);
    roster.erase(channel);
    writeLock.unlock();
    auto enrolled = enrolledEvents(channel.fd);
    if (enrolled) {
        epoll_event epollEvent = {};
        epollEvent.events = enrolled;
        epollEvent.data.fd = channel.fd;
        epoll_ctl(self, EPOLL_CTL_MOD, channel.fd, &epollEvent);
    }
    else {
        epoll_ctl(self, EPOLL_CTL_DEL, channel.fd, nullptr);
    }
}

std::vector<Channel> Epoll::wait() {
    int size = epoll_wait(self, events->data(), events->capacity(), -1);
    events->resize(size);
    std::vector<Channel> res;
    for (auto i : *events) {
        if (i.events & EPOLLHUP) {
            remove(i.data.fd);
            res.emplace_back(i.data.fd, Channel::closed);
            continue;
        }
        if (i.events & EPOLLRDHUP) {
            res.emplace_back(i.data.fd, Channel::disconnected);
            continue;
        }
        if (i.events & EPOLLIN) {
            res.emplace_back(i.data.fd, Channel::readFeasible);
        }
        if (i.events & EPOLLOUT) {
            res.emplace_back(i.data.fd, Channel::writeFeasible);
        }
    }
    return res;
}

uint32_t Epoll::enrolledEvents(int fd) {
    std::shared_lock<std::shared_mutex> readLock(mutex);
    auto [start, end] = roster.equal_range(fd);
    uint32_t res = 0;
    for (auto it = start; it != end; it++) res |= it->event;
    return res;
}

void Epoll::remove(int fd) {
    std::unique_lock<std::shared_mutex> writeLock(mutex);
    auto [start, end] = roster.equal_range(fd);
    roster.erase(start, end);
    epoll_ctl(self, EPOLL_CTL_DEL, fd, nullptr);
}
