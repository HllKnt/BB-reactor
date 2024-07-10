#include "server.hpp"

#include <mutex>
#include <shared_mutex>
#include <string>

#include "baseServer.hpp"
#include "message.hpp"
#include "nlohmann/json.hpp"

using namespace localSocket;
using namespace nlohmann;
using namespace protocol;

Server::Server(const std::string& address, const std::vector<Type>& resources)
    : BaseServer(address), resource(resources) {}

ResponseGet Server::response(const RequestGet& request) {
    ResponseGet response;
    for (auto&& i : request.addresses) {
        auto&& [object, mutex] = resource.locate(i);
        if (object == nullptr) {
            response.results.push_back(json());
            continue;
        }
        std::shared_lock<std::shared_mutex> lock(*mutex);
        response.results.push_back(*object);
    }
    return response;
}

ResponsePost Server::response(const RequestPost& request) {
    ResponsePost response;
    for (int i = 0; i < request.addresses.size() && i < request.content.size(); i++) {
        auto&& [object, mutex] = resource.locate(request.addresses[i]);
        if (object == nullptr) {
            response.results.push_back(false);
        }
        std::unique_lock<std::shared_mutex> lock(*mutex);
        *object = request.content[i];
        response.results.push_back(true);
    }
    return response;
}

void Server::handle(int fd, const std::string& info) {
    json message = json::parse(info);
    if (message["method"].template get<Method>() == get) {
        RequestGet request = message;
        inform(fd, json(response(request)).dump());
    }
    else if (message["method"].template get<Method>() == post) {
        RequestPost request = message;
        inform(fd, json(response(request)).dump());
    }
}
