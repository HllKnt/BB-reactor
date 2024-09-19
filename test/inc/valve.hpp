#pragma once

#include <sys/epoll.h>

#include <functional>
#include <semaphore>

namespace frame {
class Valve
{
public:
    using BufferIsEmpty = std::function<bool()>;
    enum State { open, shut, ready, busy, idle };

    Valve(State);

    bool tryTurnOn();
    bool tryTurnOff();
    bool tryEnable(const BufferIsEmpty&);
    bool tryDisable(const BufferIsEmpty&);
    State peerState();

private:
    State state;
    std::binary_semaphore lock;
};

}  // namespace frame
