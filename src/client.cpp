#include "client.hpp"

using namespace localSocket;
using namespace nlohmann;

Client::Client(const std::string& address) : semaphore(0) {
    sockpp::initialize();
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

void Client::getResource(const std::vector<std::string>& path) {
    json request;
    request["method"] = "get";
    request["path"] = path;
    conn.write(request.dump());
}

void Client::postResource(const std::vector<std::string>& path, const json& resource) {
    json request;
    request["method"] = "post";
    request["path"] = path;
    request["resource"] = resource;
    conn.write(request.dump());
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
        start += size;
        if (size >= additionalSize)
            info->resize(info->size() + additionalSize);
    } while (size > 0);
    response = json::parse(*info);
    semaphore.release();
}
