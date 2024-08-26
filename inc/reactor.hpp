#pragma once

#include <sys/epoll.h>

#include <functional>
#include <semaphore>
#include <thread>

#include "epoll.hpp"
#include "valve.hpp"

namespace frame {
class Reactor
{
public:
    using ValveHandles = std::vector<ValveHandle>;
    using CleanCallback = std::function<void(int)>;
    using NormalIOCallback = std::function<void(const ValveHandles&)>;

    Reactor();
    ~Reactor();

    void enroll(int fd);
    void logout(int fd);
    void refresh(int fd);
    void setEdgeTrigger(int fd);
    void setLevelTrigger(int fd);
    void enroll(const ValveHandle&);
    void logout(const ValveHandle&);
    void setClean(const CleanCallback&);
    void setNormalIO(const NormalIOCallback&);

private:
    int stop;
    bool over;
    Epoll epoll;
    std::thread react;
    CleanCallback clean;
    NormalIOCallback normalIO;
    std::binary_semaphore lock;

    void wait();
    ValveHandles filter(const Epoll::EpollEevents& epollEvents);
};

}  // namespace frame
