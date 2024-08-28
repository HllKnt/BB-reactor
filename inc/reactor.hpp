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
    using NormalIO = std::function<void(const ValveHandles&)>;
    using Disconnect = std::function<void(int)>;
    using Clean = std::function<void(int)>;

    Reactor();
    ~Reactor();

    void enroll(int fd);
    void logout(int fd);
    void refresh(int fd);
    void setEdgeTrigger(int fd);
    void setLevelTrigger(int fd);
    void enroll(const ValveHandle&);
    void logout(const ValveHandle&);
    void setNormalIO(const NormalIO&);
    void setDisconnect(const Disconnect&);
    void setClean(const Clean&);

private:
    int stop;
    bool over;
    Epoll epoll;
    Clean clean;
    Disconnect disconnect;
    NormalIO normalIO;
    std::thread react;
    std::binary_semaphore lock;

    void wait();
    ValveHandles filter(const Epoll::EpollEevents& epollEvents);
};

}  // namespace frame
