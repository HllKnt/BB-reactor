#ifndef REACTOR_H
#define REACTOR_H

#include <functional>
#include <thread>

#include "epoll.hpp"

namespace localSocket {

class Reactor : public Epoll
{
public:
    Reactor();
    ~Reactor();

    void setReadCallback(std::function<void(int)>&& callback);
    void setWriteCallback(std::function<void(int)>&& callback);
    void setDisconnectCallback(std::function<void(int)>&& callback);
    void setCloseCallback(std::function<void(int)>&& callback);

private:
    int stopEvent;
    std::thread react;
    std::function<void(int)> readCallback;
    std::function<void(int)> writeCallback;
    std::function<void(int)> disconnectCallback;
    std::function<void(int)> closeCallback;

    void work();
};

}  // namespace server

#endif  // !REACTOR_H
