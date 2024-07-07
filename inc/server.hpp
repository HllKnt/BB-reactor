#ifndef SERVER_H
#define SERVER_H

#include <memory>
#include <shared_mutex>
#include <string>
#include <tuple>
#include <unordered_map>

#include "nlohmann/json.hpp"
#include "service.hpp"

namespace localSocket {
class Server : protected Service
{
public:
    Server(const std::string& address);
    void addResource(const std::string name, const nlohmann::json& resource);

private:
    using Member = std::tuple<nlohmann::json, std::shared_mutex>;
    std::unordered_map<std::string, std::unique_ptr<Member>> groups;

    using ReturnType = std::tuple<nlohmann::json*, std::shared_mutex*>;
    ReturnType locate(
        const nlohmann::json& groupName, const nlohmann::json& path,
        const nlohmann::json& name
    );
    std::string responseGet(const nlohmann::json& aims);
    std::string responsePost(const nlohmann::json& aims);

protected:
    void parse(int fd, const std::string& info) override;
};

}  // namespace localSocket

#endif  // !SERVER_H
