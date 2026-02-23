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
    static Task<HttpResponsePtr> registerUser(
        const std::shared_ptr<Json::Value> request_json,
        const std::shared_ptr<messenger::repositories::UserRepositoryInterface>
            user_repo
    );
    static Task<HttpResponsePtr> loginUser(
        const std::shared_ptr<Json::Value> request_json,
        const std::shared_ptr<messenger::repositories::UserRepositoryInterface>
            user_repo
    );
    static Task<HttpResponsePtr> getUserById(
        int64_t user_id,
        const std::shared_ptr<messenger::repositories::UserRepositoryInterface>
            user_repo
    );
    static Task<HttpResponsePtr> getUserByHandle(
        std::string &&user_handle,
        const std::shared_ptr<messenger::repositories::UserRepositoryInterface>
            user_repo
    );
    static Task<HttpResponsePtr> searchUser(
        const std::shared_ptr<Json::Value> request_json,
        const std::shared_ptr<messenger::repositories::UserRepositoryInterface>
            user_repo
    );
};
}  // namespace v1
}  // namespace api
