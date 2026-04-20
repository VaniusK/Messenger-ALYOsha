#pragma once

#include <drogon/HttpController.h>
#include "repositories/UserRepository.hpp"
#include "services/UserService.hpp"

using namespace drogon;

namespace api {
namespace v1 {
  struct UserLastAuthTry{
  int try_count = 0;
  std::chrono::steady_clock::time_point window_start;
};
class UserController : public drogon::HttpController<UserController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(UserController::getUserById, "/v1/users/{1:user_id}", Get);
    ADD_METHOD_TO(
        UserController::getUserByHandle,
        "/v1/users/handle/{1:user_handle}",
        Get
    );
    ADD_METHOD_TO(
        UserController::searchUser,
        "/v1/users/search",
        Get,
        "api::v1::JsonValidatorFilter"
    );
    ADD_METHOD_TO(UserController::registerUser, "/v1/auth/register", Post, "api::v1::JsonValidatorFilter");
    ADD_METHOD_TO(UserController::loginUser, "/v1/auth/login", Post, "api::v1::JsonValidatorFilter");

    METHOD_LIST_END
    Task<HttpResponsePtr>
    getUserById(const HttpRequestPtr req, int64_t &&user_id);
    Task<HttpResponsePtr>
    getUserByHandle(const HttpRequestPtr req, std::string &&user_handle);
    Task<HttpResponsePtr> searchUser(const HttpRequestPtr req);
    Task<HttpResponsePtr> registerUser(const HttpRequestPtr req);
    Task<HttpResponsePtr> loginUser(const HttpRequestPtr req);

    UserController() {
        drogon::app().getLoop()->runEvery(
            std::stoi(std::getenv("SERVER_IP_LIST_CLEANING_SECONDS_COOLDOWN")),
            [this]() { this->cleanUpOldUsersAuthTries(); }
        );
        user_service.setUserRepo(
            std::make_shared<messenger::repositories::UserRepository>()
        );
        user_service.setChatRepo(
            std::make_shared<messenger::repositories::ChatRepository>(
                std::make_unique<messenger::repositories::MessageRepository>(std::make_unique<messenger::repositories::AttachmentRepository>()),
                std::make_unique<messenger::repositories::UserRepository>()
            )
        );
    }

    void setUserRepo(
        std::shared_ptr<messenger::repositories::UserRepositoryInterface>
            user_repo
    ) {
        this->user_service.setUserRepo(user_repo);
    }
    void setChatRepo(std::shared_ptr<messenger::repositories::ChatRepositoryInterface> chat_repo) {
        this->user_service.setChatRepo(chat_repo);
    }

private:
    UserService user_service;
    std::unordered_map<std::string, UserLastAuthTry> users_last_auth_try;
    std::mutex users_last_auth_try_mutex;

    bool checkUserAuthTries(const std::string &user_handle);
    void cleanUpOldUsersAuthTries();

    bool isPasswordValid(const std::string&);
    bool isHandleValid(const std::string&);

    const int MAX_REQUESTS = std::stoi(std::getenv("SERVER_MAX_REQUESTS_PER_WINDOW"));
    const int WINDOW_SECONDS = std::stoi(std::getenv("SERVER_WINDOW_FOR_REQUESTS_SECONDS"));
};
}  // namespace v1
}  // namespace api
