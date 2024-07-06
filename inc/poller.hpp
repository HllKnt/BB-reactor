#ifndef POLLER_H
#define POLLER_H

#include <sys/epoll.h>

#include <vector>

namespace localSocket {
class Epoll
{
public:
    Epoll();
    ~Epoll();

    void enroll(int fd, epoll_event& event);
    void remove(int fd);
    auto wait() -> std::vector<epoll_event>;

private:
    int max_events_default = 1024;
    epoll_event* events;
    int fd;
};

class Channel
{
public:
    enum Event { input, output, disconnect };
    Channel();
    Channel(int fd, Event event);

    int getFD();
    Event getEvent();
    void setFD(int fd);
    void setRead();
    void setWrite();
    void setET();

private:
    friend class Poller;
    int fd;
    Event event;
    epoll_event epollEvent;
};

class Poller
{
public:
    Poller();
    ~Poller();

    void endWaiting();
    bool isWaitOver();
    void enroll(Channel& channel);
    void remove(Channel& channel);

protected:
    auto wait() -> std::vector<Channel>;

private:
    Epoll epoll;
    int endWaitingEvent_fd;
    bool waitOver;
};

}  // namespace server

#endif  // !POLLER_H
