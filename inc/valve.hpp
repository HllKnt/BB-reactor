#pragma once

#include <sys/epoll.h>

#include <cstdint>
#include <functional>
#include <semaphore>

namespace frame {
struct ValveHandle
{
    int fd;
    enum Event : uint32_t { recvFeasible = EPOLLIN, sendFeasible = EPOLLOUT } event;
};

class Valve
{
public:
    using Empty = std::function<bool()>;
    enum State { open, shut, ready, busy, idle };

    Valve(State);

    State peerState();
    bool tryTurnOn();
    bool tryStartUp();
    bool tryEnable(const Empty&);
    bool tryDisable(const Empty&);

private:
    State state;
    std::binary_semaphore lock;
};

}  // namespace frame
