#include "channel.hpp"

#include <sys/epoll.h>

namespace localSocket {

Channel::Channel(int fd, Event event) : fd(fd), event(event) {}

bool operator<(const Channel& lhs, const Channel& rhs) {
    if (lhs.fd < rhs.fd)
        return true;
    else if (lhs.fd > rhs.fd)
        return false;
    if (lhs.event < rhs.event)
        return true;
    else if (lhs.event > rhs.event)
        return false;
    return false;
};

bool operator<(const int lhs, const Channel& rhs) {
    if (lhs < rhs.fd)
        return true;
    else if (lhs > rhs.fd)
        return false;
    return false;
};

bool operator<(const Channel& lhs, const int rhs) {
    if (lhs.fd < rhs)
        return true;
    else if (lhs.fd > rhs)
        return false;
    return false;
};

}  // namespace localSocket
