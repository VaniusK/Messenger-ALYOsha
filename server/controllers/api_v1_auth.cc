#include "api_v1_auth.h"
#include <drogon/HttpResponse.h>
#include <drogon/HttpTypes.h>
#include <drogon/drogon_callbacks.h>
#include <json/value.h>
#include <regex>
#include "services/UserService.hpp"
#include "utils/controller_utils.hpp"
#include "utils/server_response_macro.hpp"

using namespace api::v1;

bool auth::isPasswordValid(const std::string &password){
    return !(password.size() < 8 || password.size() > 72);
}
bool auth::isHandleValid(const std::string &handle){
    static const std::regex handle_regex("^[a-zA-Z0-9_]{3,32}$");
    return std::regex_match(handle, handle_regex);
}

Task<HttpResponsePtr> auth::registerUser(HttpRequestPtr req) {
    auto request_json = req->getJsonObject();
    Json::Value response_json;
    LOG_INFO << "Entered userController -> registerUser";

    if (utils::find_missed_fields(
            response_json, request_json, {"handle", "password", "display_name"}
        )) {
        RETURN_RESPONSE_CODE_400(response_json)
    }
    if (!isHandleValid((*request_json)["handle"].asString())){
        response_json["message"] = "Handle is invalid";
        RETURN_RESPONSE_CODE_400(response_json)
    }
    if (!isPasswordValid((*request_json)["password"].asString())){
        response_json["message"] = "Password is invalid";
        RETURN_RESPONSE_CODE_400(response_json)
    }
    co_return co_await user_service.registerUser(request_json);
}

Task<HttpResponsePtr> auth::loginUser(HttpRequestPtr req) {
    auto request_json = req->getJsonObject();
    Json::Value response_json;
    LOG_INFO << "Entered userController -> loginUser";
    if (utils::find_missed_fields(
            response_json, request_json, {"handle", "password"}
        )) {
        RETURN_RESPONSE_CODE_400(response_json)
    }
    std::string user_handle = (*request_json)["handle"].asString();
    if (!isHandleValid(user_handle)){
        response_json["message"] = "Handle is invalid";
        RETURN_RESPONSE_CODE_400(response_json)
    }
    if (!isPasswordValid((*request_json)["password"].asString())){
        response_json["message"] = "Password is invalid";
        RETURN_RESPONSE_CODE_400(response_json)
    }
    if (!checkUserAuthTries(user_handle)) {
        LOG_WARN << "Too many auth tries to user " << user_handle;
        RETURN_RESPONSE_CODE_429(response_json)
    }
    co_return co_await user_service.loginUser(request_json);
}

bool auth::checkUserAuthTries(const std::string &user_handle) {
    auto now = std::chrono::steady_clock::now();
    std::unique_lock<std::shared_mutex> lock(users_last_auth_try_mutex);
    auto &client = users_last_auth_try[user_handle];
    auto time_diff = std::chrono::duration_cast<std::chrono::seconds>(
        now - client.window_start
    );
    if (time_diff > std::chrono::seconds(WINDOW_SECONDS)) {
        client.try_count = 1;
        client.window_start = now;
    } else {
        client.try_count++;
        if (client.try_count > MAX_REQUESTS) {
            return false;
        }
    }
    return true;
}

void auth::cleanUpOldUsersAuthTries(){
    auto now = std::chrono::steady_clock::now();

    std::unique_lock<std::shared_mutex> lock(users_last_auth_try_mutex);

    std::size_t size_before = users_last_auth_try.size();
    std::erase_if(users_last_auth_try, [now, this](const auto& item){
        auto [user_handle, client] = item;
        auto time_diff = std::chrono::duration_cast<std::chrono::seconds>(now - client.window_start);

        return time_diff > std::chrono::seconds(WINDOW_SECONDS);
    });
    LOG_INFO << "Cleaned " << (size_before - users_last_auth_try.size()) << " old users auth tries.";
}

