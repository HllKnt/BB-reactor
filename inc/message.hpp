#ifndef MESSAGE_H
#define MESSAGE_H

#include <string>
#include <vector>

#include "nlohmann/json.hpp"

namespace protocol {
struct Address
{
    std::string groupName;
    std::vector<std::string> path;
};

enum Method { get, post };

NLOHMANN_JSON_SERIALIZE_ENUM(
    Method,
    {
        {get, "get"},
        {post, "post"},
    }
)

struct RequestGet
{
    const Method method = get;
    std::vector<Address> addresses;
};

struct RequestPost
{
    const Method method = post;
    std::vector<Address> addresses;
    std::vector<nlohmann::json> content;
};

struct ResponseGet
{
    const Method method = get;
    std::vector<nlohmann::json> results;
};

struct ResponsePost
{
    const Method method = post;
    std::vector<bool> results;
};

void to_json(nlohmann::json& target, const Address& source);
void from_json(const nlohmann::json& source, Address& target);
void to_json(nlohmann::json& target, const RequestGet& source);
void from_json(const nlohmann::json& source, RequestGet& target);
void to_json(nlohmann::json& target, const RequestPost& source);
void from_json(const nlohmann::json& source, RequestPost& target);
void to_json(nlohmann::json& target, const ResponseGet& source);
void from_json(const nlohmann::json& source, ResponseGet& target);
void to_json(nlohmann::json& target, const ResponsePost& source);
void from_json(const nlohmann::json& source, ResponsePost& target);

}  // namespace protocol

#endif  // !MESSAGE_H
