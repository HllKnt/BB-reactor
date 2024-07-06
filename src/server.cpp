#include "server.hpp"

#include <mutex>
#include <shared_mutex>
#include <string>

#include "nlohmann/json.hpp"
#include "service.hpp"

using namespace localSocket;
using namespace nlohmann;

Server::Server(const std::string& address, json&& resources)
    : Service(address), resources(resources) {}

std::string Server::responseNotGet() {
    json ans;
    ans["method"] = "get";
    ans["requsetPassed"] = "false";
    return ans.dump();
}

std::string Server::responseGot(const json& resource) {
    json ans;
    ans["method"] = "get";
    ans["requsetPassed"] = "true";
    ans["resource"] = resource;
    return ans.dump();
}

std::string Server::responseNotPost() {
    json ans;
    ans["method"] = "post";
    ans["requsetPassed"] = "false";
    return ans.dump();
}

std::string Server::responsePosted() {
    json ans;
    ans["method"] = "post";
    ans["requsetPassed"] = "true";
    return ans.dump();
}

json* Server::locate(const json& path) {
    json* ans = &resources;
    for (auto& i : path) {
        if (ans->find(i) == ans->end())
            return nullptr;
        ans = &(ans->at(i));
    }
    return ans;
}

void Server::parse(int fd, const std::string& info) {
    json request = json::parse(info);
    auto resource = locate(request["path"]);
    if (resource == nullptr) {
        if (request["method"] == "get")
            inform(fd, responseNotGet());
        else
            inform(fd, responseNotPost());
        return;
    }
    if (request["method"] == "get") {
        std::shared_lock<std::shared_mutex> lock(mutex);
        inform(fd, responseGot(*resource));
    }
    else {
        std::unique_lock<std::shared_mutex> lock(mutex);
        *resource = request["resource"];
        inform(fd, responsePosted());
    }
}
