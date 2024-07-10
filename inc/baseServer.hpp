#ifndef BASESERVER_H
#define BASESERVER_H

#include <sockpp/unix_acceptor.h>
#include <sockpp/unix_stream_socket.h>

#include "manager.hpp"
#include "reactor.hpp"
#include "threadPool.hpp"

namespace localSocket {
class BaseServer
{
public:
    BaseServer(const std::string& address);

protected:
    virtual void handle(int fd, const std::string& info);
    void inform(int fd, const std::string& info);

private:
    Reactor mainReactor, subReactor;
    ThreadPool threadPool;
    sockpp::unix_acceptor acceptor;
    Manager<int, sockpp::unix_socket> clients;
    using Buffer = std::queue<std::string>;
    using Member = std::tuple<Buffer, std::mutex>;
    Manager<int, std::unique_ptr<Member>> writtenMembers;

    void addClient();
    void removeClient(int fd);
    void readClient(int fd);
    void writeClient(int fd);
};

}  // namespace localSocket

#endif  // !BASESERVER_H
