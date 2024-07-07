#include "client.hpp"

#include <nlohmann/detail/conversions/to_json.hpp>

using namespace localSocket;
using namespace nlohmann;

Client::Client(const std::string& address) : semaphore(0) { sockpp::initialize();
    conn.connect(sockpp::unix_address(address));
    conn.set_non_blocking();
    reactor.setReadCallback([this](int) { recieveResponse(); });
    Channel channel;
    channel.setFD(conn.handle());
    channel.setRead();
    channel.setET();
    reactor.enroll(channel);
}

Client::~Client() { reactor.endWaiting(); }

namespace localSocket {
void to_json(json& target, const MessageGet& source) {
    target = json{
        {"group", source.group},
        {"path", source.path},
        {"name", source.name},
    };
}

void to_json(json& target, const MessagePost& source) {
    target = json{
        {"group", source.group},
        {"path", source.path},
        {"name", source.name},
        {"resource", source.resource},
    };
}

}  // namespace localSocket

void Client::requestGet(const std::vector<MessageGet>& aims) {
    json message;
    message["method"] = "get";
    message["aims"] = aims;
    conn.write(message.dump());
}

void Client::requestPost(const std::vector<MessagePost>& aims) {
    json message;
    message["method"] = "post";
    message["aims"] = aims;
    conn.write(message.dump());
}

json Client::getResponse() {
    semaphore.acquire();
    return response;
}

void Client::recieveResponse() {
    std::unique_ptr<std::string> info(new std::string);
    size_t additionalSize = 256, start = 0, size;
    info->resize(additionalSize);
    do {
        size = conn.recv(info->data() + start, additionalSize).value();
        if (size >= additionalSize)
            info->resize(info->size() + additionalSize);
        start += size;
    } while (size > 0);
    response = json::parse(*info);
    semaphore.release();
}
