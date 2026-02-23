#pragma once

#include <drogon/HttpController.h>
#include "repositories/UserRepository.hpp"
#include <memory>

using namespace drogon;

namespace api
{
namespace v1
{
class auth : public drogon::HttpController<auth>
{
  public:
    METHOD_LIST_BEGIN
    METHOD_ADD(auth::registerUser, "/register", Post, "api::v1::JsonValidatorFilter");
    METHOD_ADD(auth::loginUser, "/login", Post, "api::v1::JsonValidatorFilter");
    METHOD_LIST_END
    Task<HttpResponsePtr> registerUser(const HttpRequestPtr req);
    Task<HttpResponsePtr> loginUser(const HttpRequestPtr req);

    auth(){
      user_repo = std::make_shared<messenger::repositories::UserRepository>();
    }
    void setRepo(std::shared_ptr<messenger::repositories::UserRepositoryInterface> user_repo) {
        this->user_repo = user_repo;
    }
  private:
    std::shared_ptr<messenger::repositories::UserRepositoryInterface> user_repo;
};
}
}
