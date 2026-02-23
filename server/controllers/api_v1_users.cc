#include "api_v1_users.h"
#include <drogon/drogon.h>
#include <json/forwards.h>
#include <json/value.h>
#include <string>
#include "services/UserService.hpp"
#include "utils/controller_utils.hpp"
#include "utils/server_response_macro.hpp"

using namespace api::v1;

Task<HttpResponsePtr>
users::getUserById(const HttpRequestPtr req, int64_t &&user_id) {
    Json::Value response_json;
    co_return co_await UserService::getUserById(user_id, user_repo);
}

Task<HttpResponsePtr>
users::getUserByHandle(const HttpRequestPtr req, std::string &&user_handle) {
    Json::Value response_json;
    co_return co_await UserService::getUserByHandle(std::move(user_handle), user_repo);
}

Task<HttpResponsePtr> users::searchUser(const HttpRequestPtr req) {
    auto request_json = req->getJsonObject();
    Json::Value response_json;
    if (utils::find_missed_fields(
            response_json, request_json, {"query", "limit"}
        )) {
        RETURN_RESPONSE_CODE_400(response_json)
    }
    co_return co_await UserService::searchUser(request_json, user_repo);
}
