#include "poller.hpp"

#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <unistd.h>

#include <vector>

using namespace localSocket;

Epoll::Epoll() : fd(-1) {
    if (fd = epoll_create1(0); fd == -1)
        throw "";
    events = new epoll_event[max_events_default];
}

Epoll::~Epoll() {
    close(fd);
    delete[] events;
}

void Epoll::enroll(int fd, epoll_event& event) {
    if (int res = epoll_ctl(this->fd, EPOLL_CTL_ADD, fd, &event); res == -1)
        throw "";
}

void Epoll::remove(int fd) {
    if (int res = epoll_ctl(this->fd, EPOLL_CTL_DEL, fd, nullptr); res == -1)
        throw "";
}

std::vector<epoll_event> Epoll::wait() {
    int size = epoll_wait(this->fd, events, max_events_default, -1);
    auto ans = std::vector<epoll_event>(size);
    for (int i = 0; i < size; i++) ans[i] = events[i];
    return ans;
}

Channel::Channel() : epollEvent({}) {}

Channel::Channel(int fd, Event event) : fd(fd), event(event) {}

int Channel::getFD() { return fd; }

Channel::Event Channel::getEvent() { return event; }

void Channel::setFD(int fd) {
    this->fd = fd;
    this->epollEvent.data.fd = fd;
}

void Channel::setRead() { this->epollEvent.events |= EPOLLIN; }

void Channel::setWrite() { this->epollEvent.events |= EPOLLOUT; }

void Channel::setET() { this->epollEvent.events |= EPOLLET; }

Poller::Poller() : waitOver(false) {
    endWaitingEvent_fd = eventfd(0, 0);
    Channel channel;
    channel.setFD(endWaitingEvent_fd);
    channel.setRead();
    enroll(channel);
}

Poller::~Poller() { close(endWaitingEvent_fd); }

void Poller::endWaiting() {
    waitOver = true;
    eventfd_write(endWaitingEvent_fd, 1);
}

bool Poller::isWaitOver() { return waitOver; }

void Poller::enroll(Channel& channel) { epoll.enroll(channel.fd, channel.epollEvent); }

void Poller::remove(Channel& channel) { epoll.remove(channel.fd); }

std::vector<Channel> Poller::wait() {
    auto&& events = epoll.wait();
    std::vector<Channel> ans;
    for (auto i : events) {
        if (i.data.fd == endWaitingEvent_fd)
            return std::vector<Channel>();
        if (i.events & EPOLLRDHUP || i.events & EPOLLHUP) {
            ans.emplace_back(i.data.fd, Channel::disconnect);
            continue;
        }
        if (i.events & EPOLLIN) {
            ans.emplace_back(i.data.fd, Channel::input);
        }
        if (i.events & EPOLLOUT) {
            ans.emplace_back(i.data.fd, Channel::output);
        }
    }
    return ans;
}
