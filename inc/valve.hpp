#pragma once

#include <sys/epoll.h>

#include <cstdint>
#include <functional>
#include <semaphore>

namespace frame {
struct ValveHandle
{
    int fd;
    enum Event : uint32_t { readFeasible = EPOLLIN, writeFeasible = EPOLLOUT } event;
};

class Valve
{
public:
    using Empty = std::function<bool()>;
    enum State { open, shut, ready, busy, idle };

    Valve(State);
    Valve(Valve&&);

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

template <>
struct std::hash<frame::ValveHandle>
{
    size_t operator()(const frame::ValveHandle& handle) const {
        return (std::hash<int>{}(handle.fd) << 1) ^ (std::hash<int>{}(handle.event) >> 1);
    }
};

template <>
struct std::equal_to<frame::ValveHandle>
{
    bool operator()(const frame::ValveHandle& lhs, const frame::ValveHandle& rhs) const {
        return lhs.fd == rhs.fd && lhs.event == rhs.event;
    }
};
