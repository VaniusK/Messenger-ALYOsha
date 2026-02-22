#include <json/value.h>
#include <vector>

namespace api::v1::utils {
bool find_missed_fields(
    Json::Value &resp_json,
    Json::Value &req_json,
    std::vector<std::string> &&necessary_fields
);
}