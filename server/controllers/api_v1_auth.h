#pragma once

#include <drogon/HttpController.h>
#include "repositories/UserRepository.hpp"
#include "repositories/ChatRepository.hpp"
#include "services/UserService.hpp"
#include <memory>

using namespace drogon;

namespace api
{
namespace v1
{

struct UserLastAuthTry{
  int try_count = 0;
  std::chrono::steady_clock::time_point window_start;
};

class auth : public drogon::HttpController<auth>
{
  public:
    METHOD_LIST_BEGIN
    METHOD_ADD(auth::registerUser, "/register", Post, "api::v1::IpFilter", "api::v1::JsonValidatorFilter");
    METHOD_ADD(auth::loginUser, "/login", Post, "api::v1::IpFilter", "api::v1::JsonValidatorFilter");
    METHOD_LIST_END
    Task<HttpResponsePtr> registerUser(const HttpRequestPtr req);
    Task<HttpResponsePtr> loginUser(const HttpRequestPtr req);

    auth(){
      drogon::app().getLoop()->runEvery(std::stoi(std::getenv("SERVER_IP_LIST_CLEANING_SECONDS_COOLDOWN")), [this](){
        this->cleanUpOldUsersAuthTries();
      });
      user_service.setUserRepo(std::make_shared<messenger::repositories::UserRepository>());
      user_service.setChatRepo(std::make_shared<messenger::repositories::ChatRepository>(std::make_unique<messenger::repositories::MessageRepository>(), std::make_unique<messenger::repositories::UserRepository>()));
    }
    void setUserRepo(std::shared_ptr<messenger::repositories::UserRepositoryInterface> user_repo) {
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
}
}
