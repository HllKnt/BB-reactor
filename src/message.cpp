#include "message.hpp"

#include <nlohmann/detail/conversions/from_json.hpp>
#include <nlohmann/detail/conversions/to_json.hpp>
#include <nlohmann/detail/macro_scope.hpp>

using namespace nlohmann;

namespace protocol {
void to_json(json& target, const Address& source) {
    target = json{
        {"groupName", source.groupName},
        {"path", source.path},
    };
}

void from_json(const json& source, Address& target) {
    source.at("groupName").get_to(target.groupName);
    source.at("path").get_to(target.path);
}

void to_json(json& target, const RequestGet& source) {
    target = json{
        {"method", source.method},
        {"addresses", source.addresses},
    };
}

void from_json(const json& source, RequestGet& target) {
    source.at("addresses").get_to(target.addresses);
}

void to_json(json& target, const RequestPost& source) {
    target = json{
        {"method", source.method},
        {"addresses", source.addresses},
        {"content", source.content},
    };
}

void from_json(const json& source, RequestPost& target) {
    source.at("addresses").get_to(target.addresses);
    source.at("content").get_to(target.content);
}

void to_json(json& target, const ResponseGet& source) {
    target = json{
        {"method", source.method},
        {"results", source.results},
    };
}

void from_json(const json& source, ResponseGet& target) {
    source.at("results").get_to(target.results);
}

void to_json(json& target, const ResponsePost& source) {
    target = json{
        {"method", source.method},
        {"results", source.results},
    };
}

void from_json(const json& source, ResponsePost& target) {
    source.at("results").get_to(target.results);
}

}  // namespace protocol
