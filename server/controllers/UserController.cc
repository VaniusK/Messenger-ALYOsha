#include "UserController.h"
#include <drogon/drogon.h>
#include <json/forwards.h>
#include <json/value.h>
#include <exception>
#include <string>
#include <regex>
#include "services/UserService.hpp"
#include "utils/controller_utils.hpp"
#include "utils/server_response_macro.hpp"
#include "utils/server_exceptions.hpp"
#include "dto/UserServiceDtos.hpp"

using namespace api::v1;
using namespace messenger::dto;

Task<HttpResponsePtr>
UserController::getUserById(const HttpRequestPtr req, int64_t user_id) {
    LOG_INFO << "Entered userController -> getUserById";
    Json::Value response_json;
    try{
        GetUserResponseDto response_dto = co_await user_service.getUserById(user_id);
        response_json = response_dto.toJson();
        RETURN_RESPONSE_CODE_200(response_json)
    }
    catch (messenger::exceptions::NotFoundException &e){
        response_json["message"] = e.what();
        RETURN_RESPONSE_CODE_404(response_json)
    }
    catch (messenger::exceptions::InternalServerErrorException &e) {
        response_json["messsage"] = e.what();
        RETURN_RESPONSE_CODE_500(response_json)
    }
    catch (std::exception &e) {
        response_json["message"] = std::string("Internal server error: ") + e.what();
        RETURN_RESPONSE_CODE_500(response_json)
    }
}

Task<HttpResponsePtr>
UserController::getUserByHandle(const HttpRequestPtr req, std::string user_handle) {
    LOG_INFO << "Entered userController -> getUserByHandle";
    Json::Value response_json;
    try{
        GetUserResponseDto response_dto = co_await user_service.getUserByHandle(std::move(user_handle));
        response_json = response_dto.toJson();
        RETURN_RESPONSE_CODE_200(response_json)
    }
    catch (messenger::exceptions::NotFoundException &e){
        response_json["message"] = e.what();
        RETURN_RESPONSE_CODE_404(response_json)
    }
    catch (messenger::exceptions::InternalServerErrorException &e) {
        response_json["messsage"] = e.what();
        RETURN_RESPONSE_CODE_500(response_json)
    }
    catch (std::exception &e) {
        response_json["message"] = std::string("Internal server error: ") + e.what();
        RETURN_RESPONSE_CODE_500(response_json)
    }
}

Task<HttpResponsePtr> UserController::searchUser(const HttpRequestPtr req) {
    LOG_INFO << "Entered userController -> searchUser";
    Json::Value response_json;
    if (utils::find_missed_queries(
            response_json, req, {"query", "limit"}
        )) {
        RETURN_RESPONSE_CODE_400(response_json)
    }
    SearchUserRequestDto request_dto(req);
    SearchUserResponseDto response_dto = co_await user_service.searchUser(std::move(request_dto));
    
    response_json["results"] = response_dto.toJson();
    RETURN_RESPONSE_CODE_200(response_json)
}

bool UserController::isPasswordValid(const std::string &password){
    return !(password.size() < 8 || password.size() > 72);
}
bool UserController::isHandleValid(const std::string &handle){
    static const std::regex handle_regex("^[a-zA-Z0-9_]{3,32}$");
    return std::regex_match(handle, handle_regex);
}

Task<HttpResponsePtr> UserController::registerUser(HttpRequestPtr req) {
    auto request_json = req->getJsonObject();
    Json::Value response_json;
    LOG_INFO << "Entered userController -> registerUser";

    if (utils::find_missed_fields(
            response_json, request_json, {"handle", "password", "display_name"}
        )) {
        RETURN_RESPONSE_CODE_400(response_json)
    }
    RegisterUserRequestDto request_dto(request_json);
    if (!isHandleValid(request_dto.handle)){
        response_json["message"] = "Handle is invalid";
        RETURN_RESPONSE_CODE_400(response_json)
    }
    if (!isPasswordValid(request_dto.password)){
        response_json["message"] = "Password is invalid";
        RETURN_RESPONSE_CODE_400(response_json)
    }

    try{
        RegisterUserResponseDto response_dto = co_await user_service.registerUser(std::move(request_dto));
        response_json = response_dto.toJson();
        RETURN_RESPONSE_CODE_201(response_json)
    }
    catch (messenger::exceptions::ConflictException &e){
        response_json["message"] = e.what();
        RETURN_RESPONSE_CODE_409(response_json)
    }
    catch (messenger::exceptions::InternalServerErrorException &e) {
        response_json["messsage"] = e.what();
        RETURN_RESPONSE_CODE_500(response_json)
    }
    catch (std::exception &e) {
        response_json["message"] = std::string("Internal server error: ") + e.what();
        RETURN_RESPONSE_CODE_500(response_json)
    }
}

Task<HttpResponsePtr> UserController::loginUser(HttpRequestPtr req) {
    auto request_json = req->getJsonObject();
    Json::Value response_json;
    LOG_INFO << "Entered userController -> loginUser";
    if (utils::find_missed_fields(
            response_json, request_json, {"handle", "password"}
        )) {
        RETURN_RESPONSE_CODE_400(response_json)
    }
    LoginUserRequestDto request_dto(request_json);
    if (!isHandleValid(request_dto.handle)){
        response_json["message"] = "Handle is invalid";
        RETURN_RESPONSE_CODE_400(response_json)
    }
    if (!isPasswordValid(request_dto.password)){
        response_json["message"] = "Password is invalid";
        RETURN_RESPONSE_CODE_400(response_json)
    }
    if (!checkUserAuthTries(request_dto.handle)) {
        LOG_WARN << "Too many auth tries to user " << request_dto.handle;
        RETURN_RESPONSE_CODE_429(response_json)
    }

    try{
        LoginUserResponseDto response_dto = co_await user_service.loginUser(request_dto);
        response_json = response_dto.toJson();
        RETURN_RESPONSE_CODE_200(response_json)
    }
    catch (messenger::exceptions::UnauthorizedException &e){
        response_json["message"] = e.what();
        RETURN_RESPONSE_CODE_401(response_json)
    }
    catch (messenger::exceptions::InternalServerErrorException &e) {
        response_json["messsage"] = e.what();
        RETURN_RESPONSE_CODE_500(response_json)
    }
    catch (std::exception &e) {
        response_json["message"] = std::string("Internal server error: ") + e.what();
        RETURN_RESPONSE_CODE_500(response_json)
    }
    
}

bool UserController::checkUserAuthTries(const std::string &user_handle) {
    auto now = std::chrono::steady_clock::now();
    std::lock_guard<std::mutex> lock(users_last_auth_try_mutex);
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

void UserController::cleanUpOldUsersAuthTries(){
    auto now = std::chrono::steady_clock::now();

    std::lock_guard<std::mutex> lock(users_last_auth_try_mutex);

    std::size_t size_before = users_last_auth_try.size();
    std::erase_if(users_last_auth_try, [now, this](const auto& item){
        auto [user_handle, client] = item;
        auto time_diff = std::chrono::duration_cast<std::chrono::seconds>(now - client.window_start);

        return time_diff > std::chrono::seconds(WINDOW_SECONDS);
    });
    LOG_INFO << "Cleaned " << (size_before - users_last_auth_try.size()) << " old users auth tries.";
}
