#ifndef RESOURCE_H
#define RESOURCE_H

#include <memory>
#include <shared_mutex>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

#include "message.hpp"
#include "nlohmann/json.hpp"

namespace protocol {
class Resource
{
public:
    using Type = std::tuple<std::string, nlohmann::json>;
    Resource(const std::vector<Type>& resources);

    using ReturnType = std::tuple<nlohmann::json*, std::shared_mutex*>;
    ReturnType locate(const protocol::Address& address);

private:
    using Group = std::tuple<nlohmann::json, std::shared_mutex>;
    std::unordered_map<std::string, std::unique_ptr<Group>> groups;
};

}  // namespace protocol

#endif  // !RESOURCE_H
