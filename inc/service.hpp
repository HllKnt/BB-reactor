#ifndef SERVIVE_H
#define SERVIVE_H

#include <sockpp/unix_acceptor.h>
#include <sockpp/unix_stream_socket.h>

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
    using Buffer = std::queue<std::string>; 
    using Member = std::tuple<Buffer, std::mutex>;
    Manager<int, std::unique_ptr<Member>> writtenMembers;

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
