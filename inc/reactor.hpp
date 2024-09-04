#pragma once

#include <sys/epoll.h>

#include <functional>
#include <semaphore>
#include <thread>
#include <vector>

#include "epoll.hpp"

namespace frame {
class Reactor
{
public:
    using Handles = std::vector<int>;
    using Clean = std::function<void(int)>;
    using Disconnect = std::function<void(int)>;
    using Recv = std::function<void(const Handles&)>;
    using Send = std::function<void(const Handles&)>;
    enum Event : uint32_t {
        recvFeasible = EPOLLIN,
        sendFeasible = EPOLLOUT,
        recvShut = EPOLLRDHUP,
        sendShut = EPOLLHUP
    };
    enum Trigger : uint32_t { edge = EPOLLET, level = 0 };

    Reactor();
    ~Reactor();

    void enroll(int fd);
    void logout(int fd);
    void refresh(int fd);
    void appendEvent(Event);
    void setTrigger(Trigger);
    void setRecv(const Recv&);
    void setSend(const Send&);
    void setClean(const Clean&);
    void setDisconnect(const Disconnect&);

private:
    int stop;
    bool over;
    Epoll epoll;
    std::thread react;
    uint32_t defaultConfig;
    std::binary_semaphore lock;
    Recv recv;
    Send send;
    Clean clean;
    Disconnect disconnect;

    void wait();
    void filter(const Epoll::EpollEevents& epollEvents);
};

}  // namespace frame
