#ifndef REACTOR_H
#define REACTOR_H

#include <functional>
#include <thread>

#include "poller.hpp"

namespace localSocket {

class Reactor : public Poller
{
public:
    Reactor();
    ~Reactor();

    void setDisconnectCallback(std::function<void(int)>&& callback);
    void setReadCallback(std::function<void(int)>&& callback);
    void setWriteCallback(std::function<void(int)>&& callback);

private:
    std::function<void(int)> disconnectCallback;
    std::function<void(int)> readCallback;
    std::function<void(int)> writeCallback;
    std::thread react;

    void work();
};

}  // namespace server

#endif  // !REACTOR_H
