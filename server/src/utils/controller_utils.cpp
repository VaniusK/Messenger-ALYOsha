#include <drogon/HttpRequest.h>
#include <json/value.h>
#include <string>
#include <vector>

namespace api::v1::utils {
bool find_missed_fields(
    Json::Value &resp_json,
    const std::shared_ptr<Json::Value> req_json,
    std::vector<std::string> &&necessary_fields
) {
    std::vector<std::string> unfinded_fields;
    for (const auto &required_field : necessary_fields) {
        if (!(req_json->isMember(required_field))) {
            unfinded_fields.push_back(required_field);
        }
    }
    if (!unfinded_fields.empty()) {
        std::string missing_fields_str = "Missing fields: ";
        for (const auto &field : unfinded_fields) {
            missing_fields_str += field;
            if (field != unfinded_fields.back()) {
                missing_fields_str += ", ";
            }
        }
        resp_json["message"] =
            "Invalid JSON: couldn't find some fields. " + missing_fields_str;
        return true;
    }
    return false;
}

bool find_missed_queries(
    Json::Value &resp_json,
    const drogon::HttpRequestPtr req,
    std::vector<std::string> &&necessary_queries
) {
    std::vector<std::string> unfinded_queries;
    for (const auto &required_query : necessary_queries) {
        if (req->getParameter(required_query).empty()) {
            unfinded_queries.push_back(required_query);
        }
    }
    if (!unfinded_queries.empty()) {
        std::string missing_queries_str = "Missing query-parameters: ";
        for (const auto &field : unfinded_queries) {
            missing_queries_str += field;
            if (field != unfinded_queries.back()) {
                missing_queries_str += ", ";
            }
        }
        resp_json["message"] =
            "Invalid request: couldn't find some query-parameters. " +
            missing_queries_str;
        return true;
    }
    return false;
}
}  // namespace api::v1::utils