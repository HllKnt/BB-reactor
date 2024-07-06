#ifndef SERVER_H
#define SERVER_H

#include <shared_mutex>
#include <string>

#include "nlohmann/json.hpp"
#include "service.hpp"

namespace localSocket {
class Server : protected Service
{
public:
    Server(const std::string& address, nlohmann::json&& resources);

private:
    nlohmann::json resources;
    std::shared_mutex mutex;

    std::string responseNotGet();
    std::string responseGot(const nlohmann::json& resource);
    std::string responseNotPost();
    std::string responsePosted();
    nlohmann::json* locate(const nlohmann::json& path);

protected:
    void parse(int fd, const std::string& info) override;
};

}  // namespace localSocket

#endif  // !SERVER_H
