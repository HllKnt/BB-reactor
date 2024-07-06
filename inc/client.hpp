#ifndef CLIENT_H
#define CLIENT_H

#include <semaphore>

#include "nlohmann/json.hpp"
#include "reactor.hpp"
#include "sockpp/unix_connector.h"

namespace localSocket {
class Client
{
public:
    Client(const std::string& address);
    ~Client();

    void getResource(const std::vector<std::string>& path);
    void postResource(const std::vector<std::string>& path, const nlohmann::json& resource);
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
