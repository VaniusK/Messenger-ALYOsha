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
    Task<HttpResponsePtr> registerUser(
        const std::shared_ptr<Json::Value> request_json
    );
    Task<HttpResponsePtr> loginUser(
        const std::shared_ptr<Json::Value> request_json
    );
    Task<HttpResponsePtr> getUserById(int64_t user_id);
    Task<HttpResponsePtr> getUserByHandle(std::string &&user_handle);
    Task<HttpResponsePtr> searchUser(
        const std::shared_ptr<Json::Value> request_json
    );

    void setUserRepo(
        const std::shared_ptr<messenger::repositories::UserRepositoryInterface>
            user_repo
    ) {
        this->user_repo = user_repo;
    }

private:
    std::shared_ptr<messenger::repositories::UserRepositoryInterface> user_repo;
};
}  // namespace v1
}  // namespace api
