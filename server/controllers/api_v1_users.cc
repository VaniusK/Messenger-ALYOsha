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
    LOG_INFO <<"Entered userController -> getUserById";
    co_return co_await user_service.getUserById(user_id);
}

Task<HttpResponsePtr>
users::getUserByHandle(const HttpRequestPtr req, std::string &&user_handle) {
    LOG_INFO << "Entered userController -> getUserByHandle";
    co_return co_await user_service.getUserByHandle(std::move(user_handle));
}

Task<HttpResponsePtr> users::searchUser(const HttpRequestPtr req) {
    LOG_INFO << "Entered userController -> searchUser";
    auto request_json = req->getJsonObject();
    Json::Value response_json;
    if (utils::find_missed_fields(
            response_json, request_json, {"query", "limit"}
        )) {
        RETURN_RESPONSE_CODE_400(response_json)
    }
    co_return co_await user_service.searchUser(request_json);
}
