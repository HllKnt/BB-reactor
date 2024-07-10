#include "resource.hpp"

using namespace protocol;
using namespace nlohmann;

Resource::Resource(const std::vector<Type>& resources) {
    for (auto&& i : resources) {
        auto&& [name, source] = i;
        groups.emplace(name, std::make_unique<Group>());
        auto&& [target, mutex] = *groups.at(name);
        target = source;
    }
}

using ReturnType = std::tuple<nlohmann::json*, std::shared_mutex*>;
ReturnType Resource::locate(const protocol::Address& address) {
    if (groups.find(address.groupName) == groups.end())
        return {nullptr, nullptr};
    auto&& [group, mutex] = *groups.at(address.groupName);
    json* object = &group;
    for (auto& i : address.path) {
        if (object->find(i) == object->end())
            return {nullptr, nullptr};
        object = &object->at(i);
    }
    return {object, &mutex};
}
