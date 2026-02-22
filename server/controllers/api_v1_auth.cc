#include "api_v1_auth.h"
#include <drogon/HttpResponse.h>
#include <drogon/HttpTypes.h>
#include <drogon/drogon_callbacks.h>
#include <json/value.h>
#include "services/UserService.hpp"
#include "utils/controller_utils.hpp"
#include "utils/server_response_macro.hpp"

using namespace api::v1;

Task<HttpResponsePtr> auth::registerUser(HttpRequestPtr req) {
    Json::Value request_json = *(req->getJsonObject());
    Json::Value response_json;

    if (utils::find_missed_fields(
            response_json, request_json, {"handle", "password", "display_name"}
        )) {
        RETURN_RESPONSE_CODE_400(response_json)
    }
    co_return co_await UserService::registerUser(std::move(request_json), response_json, user_repo);
}

Task<HttpResponsePtr> auth::loginUser(HttpRequestPtr req) {
    Json::Value request_json = *(req->getJsonObject());
    Json::Value response_json;
    if (utils::find_missed_fields(
            response_json, request_json, {"handle", "password"}
        )) {
        RETURN_RESPONSE_CODE_400(response_json)
    }
    co_return co_await UserService::loginUser(std::move(request_json), response_json, user_repo);
}
