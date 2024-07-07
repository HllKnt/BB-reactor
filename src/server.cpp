#include "server.hpp"

#include <mutex>
#include <shared_mutex>
#include <string>

#include "nlohmann/json.hpp"
#include "service.hpp"

using namespace localSocket;
using namespace nlohmann;

Server::Server(const std::string& address) : Service(address) {}

void Server::addResource(const std::string name, const nlohmann::json& resource) {
    groups.emplace(name, std::make_unique<Member>());
    auto&& [target, mutex] = *groups.at(name);
    target = resource;
}

using ReturnType = std::tuple<nlohmann::json*, std::shared_mutex*>;
ReturnType Server::locate(const json& groupName, const json& path, const json& name) {
    if (groups.find(groupName) == groups.end())
        return {nullptr, nullptr};
    auto&& [group, mutex] = *groups.at(groupName);
    json* object = &group;
    for (auto& i : path) {
        if (object->find(i) == object->end())
            return {nullptr, nullptr};
        object = &object->at(i);
    }
    if (object->find(name) == object->end())
        return {nullptr, nullptr};
    return {&object->at(name), &mutex};
}

std::string Server::responseGet(const nlohmann::json& aims) {
    json message;
    message["method"] = "get";
    message["passed"] = false;
    for (auto& i : aims) {
        auto [object, mutex] = locate(i["group"], i["path"], i["name"]);
        if (object == nullptr)
            continue;
        message["passed"] = true;
        std::shared_lock<std::shared_mutex> lock(*mutex);
        message["resource"][i["name"]] = *object;
    }
    return message.dump();
}

std::string Server::responsePost(const nlohmann::json& aims) {
    json message;
    message["method"] = "post";
    message["passed"] = false;
    for (auto& i : aims) {
        auto [object, mutex] = locate(i["group"], i["path"], i["name"]);
        if (object == nullptr)
            continue;
        message["passed"] = true;
        std::unique_lock<std::shared_mutex> lock(*mutex);
        *object = i["resource"];
    }
    return message.dump();
}

void Server::parse(int fd, const std::string& info) {
    json message = json::parse(info);
    if (message["method"] == "get") {
        inform(fd, responseGet(message["aims"]));
    }
    else if (message["method"] == "post") {
        inform(fd, responsePost(message["aims"]));
    }
}
