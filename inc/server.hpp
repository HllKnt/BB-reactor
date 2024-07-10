#ifndef SERVER_H
#define SERVER_H

#include <string>

#include "baseServer.hpp"
#include "message.hpp"
#include "nlohmann/json.hpp"
#include "resource.hpp"

namespace localSocket {
class Server : protected BaseServer
{
public:
    using Type = std::tuple<std::string, nlohmann::json>;
    Server(const std::string& address, const std::vector<Type>& resources);

private:
    protocol::Resource resource;

    protocol::ResponseGet response(const protocol::RequestGet& request);
    protocol::ResponsePost response(const protocol::RequestPost& request);

protected:
    void handle(int fd, const std::string& info) override;
};

}  // namespace localSocket

#endif  // !SERVER_H
