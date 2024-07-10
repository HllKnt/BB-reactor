#ifndef CLIENT_H
#define CLIENT_H

#include <semaphore>

#include "message.hpp"
#include "nlohmann/json.hpp"
#include "reactor.hpp"
#include "sockpp/unix_connector.h"

namespace localSocket {
class Client
{
public:
    Client(const std::string& address);
    ~Client();

    void request(const protocol::RequestGet& request);
    void request(const protocol::RequestPost& request);
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
