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

    State peerState();
    bool tryTurnOnWithAgain();
    bool tryStartUpWithAgain();
    bool tryEnable(const BufferIsEmpty&);
    bool tryDisable(const BufferIsEmpty&);

private:
    State state;
    std::binary_semaphore lock;
};

}  // namespace frame
