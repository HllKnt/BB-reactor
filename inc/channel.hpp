#ifndef CHANNEL_H
#define CHANNEL_H

#include <sys/epoll.h>

namespace localSocket {
class Channel
{
public:
    enum Event {
        readFeasible = EPOLLIN,
        writeFeasible = EPOLLOUT,
        disconnected = EPOLLRDHUP,
        closed = EPOLLHUP
    };

public:
    Channel(int fd, Event event);

public:
    int fd;
    Event event;
};

bool operator<(const Channel& lhs, const Channel& rhs);
bool operator<(const int lhs, const Channel& rhs);
bool operator<(const Channel& lhs, const int rhs);

}  // namespace localSocket

#endif  // !CHANNEL_H
