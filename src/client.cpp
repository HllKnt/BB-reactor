#include "client.hpp"

#include <sys/ioctl.h>

#include <nlohmann/detail/conversions/to_json.hpp>

#include "message.hpp"

using namespace localSocket;
using namespace nlohmann;
using namespace protocol;

Client::Client(const std::string& address) : semaphore(0) {
    sockpp::initialize();
    conn.connect(sockpp::unix_address(address));
    conn.set_non_blocking();
    reactor.setReadCallback([this](int) { recieveResponse(); });
    reactor.enroll({conn.handle(), Channel::readFeasible}, Epoll::TriggerMode::edge);
}

Client::~Client() {}

void Client::request(const RequestGet& request) {
    semaphore.try_acquire();
    conn.write(json(request).dump());
}

void Client::request(const RequestPost& request) {
    semaphore.try_acquire();
    conn.write(json(request).dump());
}

json Client::getResponse() {
    semaphore.acquire();
    return response;
}

void Client::recieveResponse() {
    size_t size = 0;
    ioctl(conn.handle(), FIONREAD, &size);
    std::unique_ptr<std::string> info(new std::string);
    info->resize(size);
    conn.recv(info->data(), info->size()).value();
    response = json::parse(*info);
    semaphore.release();
}
