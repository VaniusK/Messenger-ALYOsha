#pragma once
#include <drogon/HttpController.h>
#include <json/value.h>
#include <memory>
#include "include/repositories/UserRepository.hpp"
#include "repositories/ChatRepository.hpp"
#include "utils/password_hasher.hpp"

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

    void setChatRepo(
        const std::shared_ptr<messenger::repositories::ChatRepositoryInterface>
            chat_repo
    ) {
        this->chat_repo = chat_repo;
    }

    void setPasswordHasher(
        const std::shared_ptr<messenger::utils::PasswordHasherInterface>
            password_hasher
    ) {
        this->password_hasher = password_hasher;
    }

private:
    std::shared_ptr<messenger::repositories::UserRepositoryInterface> user_repo;
    std::shared_ptr<messenger::repositories::ChatRepositoryInterface> chat_repo;
    std::shared_ptr<messenger::utils::PasswordHasherInterface> password_hasher =
        std::make_shared<messenger::utils::BCryptPasswordHasher>();
};
}  // namespace v1
}  // namespace api
