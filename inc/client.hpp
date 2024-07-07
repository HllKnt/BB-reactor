#ifndef CLIENT_H
#define CLIENT_H

#include <semaphore>

#include "nlohmann/json.hpp"
#include "reactor.hpp"
#include "sockpp/unix_connector.h"

namespace localSocket {
struct MessageGet
{
    std::string group;
    std::vector<std::string> path;
    std::string name;
};

struct MessagePost
{
    std::string group;
    std::vector<std::string> path;
    std::string name;
    nlohmann::json resource;
};

class Client
{
public:
    Client(const std::string& address);
    ~Client();

    void requestGet(const std::vector<MessageGet>& aims);
    void requestPost(const std::vector<MessagePost>& aims);
    nlohmann::json getResponse();

private:
    Reactor reactor;
    sockpp::unix_connector conn;
    std::binary_semaphore semaphore;
    nlohmann::json response;

    void recieveResponse();
};

}  // namespace localSocket

#endif  // !CLIENT_H
