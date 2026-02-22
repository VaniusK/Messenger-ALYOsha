#pragma once
#include <drogon/HttpController.h>
#include <json/value.h>
#include <memory>
#include "include/repositories/UserRepository.hpp"

using namespace drogon;

namespace api {
namespace v1 {
class UserService {
public:
    static Task<HttpResponsePtr>
    registerUser(Json::Value &&request_json, Json::Value response_json, std::shared_ptr<messenger::repositories::UserRepositoryInterface> user_repo);
    static Task<HttpResponsePtr>
    loginUser(Json::Value &&request_json, Json::Value response_json, std::shared_ptr<messenger::repositories::UserRepositoryInterface> user_repo);
    static Task<HttpResponsePtr> getUserById(
        Json::Value &&request_json,
        Json::Value response_json,
        int64_t user_id,
        std::shared_ptr<messenger::repositories::UserRepositoryInterface> user_repo
    );
    static Task<HttpResponsePtr> getUserByHandle(
        Json::Value &&request_json,
        Json::Value response_json,
        std::string &&user_handle,
        std::shared_ptr<messenger::repositories::UserRepositoryInterface> user_repo
    );
    static Task<HttpResponsePtr>
    searchUser(Json::Value &&request_json, Json::Value response_json, std::shared_ptr<messenger::repositories::UserRepositoryInterface> user_repo);
};
}  // namespace v1
}  // namespace api
