#ifndef SERVIVE_H
#define SERVIVE_H

#include <sockpp/unix_acceptor.h>
#include <sockpp/unix_stream_socket.h>

#include <deque>

#include "manager.hpp"
#include "reactor.hpp"
#include "threadPool.hpp"

namespace localSocket {
class Service
{
public:
    Service(const std::string& address);
    ~Service();

private:
    Reactor mainReactor, subReactor;
    ThreadPool threadPool;
    sockpp::unix_acceptor acceptor;
    Manager<int, sockpp::unix_socket> clients;
    Manager<int, std::unique_ptr<std::mutex>> writingRight;
    Manager<int, std::deque<std::unique_ptr<std::string>>> writeBuffer;

    void addClient();
    void removeClient(int fd);
    void readClient(int fd);
    void writeClient(int fd);

protected:
    virtual void parse(int fd, const std::string& info);
    void inform(int fd, const std::string& info);
};

}  // namespace localSocket

#endif  // !SERVER_H
