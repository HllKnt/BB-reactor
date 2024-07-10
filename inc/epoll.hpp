#ifndef EPOLL_H
#define EPOLL_H

#include <sys/epoll.h>

#include <memory>
#include <set>
#include <shared_mutex>
#include <vector>

#include "channel.hpp"

namespace localSocket {
class Epoll
{
public:
    Epoll();
    ~Epoll();

    enum TriggerMode { level, edge };
    void enroll(const Channel& channel, TriggerMode mode = level);
    void remove(const Channel& channel);

protected:
    std::vector<Channel> wait();

private:
    int self;
    std::shared_mutex mutex;
    std::set<Channel, std::less<>> roster;
    std::unique_ptr<std::vector<epoll_event>> events;

    uint32_t enrolledEvents(int fd);
    void remove(int fd);
};

}  // namespace localSocket

#endif  // !EPOLL_H
