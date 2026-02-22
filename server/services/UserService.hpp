#pragma once
#include <drogon/HttpController.h>
#include <json/value.h>

using namespace drogon;

namespace api {
namespace v1 {
class UserService {
public:
    Task<HttpResponsePtr>
    registerUser(Json::Value &&request_json, Json::Value response_json);
    Task<HttpResponsePtr>
    loginUser(Json::Value &&request_json, Json::Value response_json);
    Task<HttpResponsePtr> getUserById(
        Json::Value &&request_json,
        Json::Value response_json,
        int64_t user_id
    );
    Task<HttpResponsePtr> getUserByHandle(
        Json::Value &&request_json,
        Json::Value response_json,
        std::string &&user_handle
    );
    Task<HttpResponsePtr>
    searchUser(Json::Value &&request_json, Json::Value response_json);
};
}  // namespace v1
}  // namespace api
