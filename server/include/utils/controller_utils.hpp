#include <drogon/HttpRequest.h>
#include <json/value.h>
#include <vector>

namespace api::v1::utils {
bool find_missed_fields(
    Json::Value &resp_json,
    const std::shared_ptr<Json::Value> req_json,
    std::vector<std::string> &&necessary_fields
);

bool find_missed_queries(
    Json::Value &resp_json,
    const drogon::HttpRequestPtr req,
    std::vector<std::string> &&necessary_queries
);
}  // namespace api::v1::utils
