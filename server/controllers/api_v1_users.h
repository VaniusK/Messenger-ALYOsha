#pragma once

#include <drogon/HttpController.h>
#include "repositories/UserRepository.hpp"
#include "services/UserService.hpp"

using namespace drogon;

namespace api
{
namespace v1
{
class users : public drogon::HttpController<users>
{
  public:
    METHOD_LIST_BEGIN
    METHOD_ADD(users::getUserById, "/{1:user_id}", Get, "api::v1::IpFilter");
    METHOD_ADD(users::getUserByHandle, "/handle/{1:user_handle}", Get, "api::v1::IpFilter");
    METHOD_ADD(users::searchUser, "/search", Get, "api::v1::IpFilter", "api::v1::JsonValidatorFilter");

    METHOD_LIST_END
    Task<HttpResponsePtr> getUserById(const HttpRequestPtr req, int64_t &&user_id);
    Task<HttpResponsePtr> getUserByHandle(const HttpRequestPtr req, std::string &&user_handle);
    Task<HttpResponsePtr> searchUser(const HttpRequestPtr req);
    users(){
        user_service.setUserRepo(std::make_shared<messenger::repositories::UserRepository>());
    }
    void setUserRepo(std::shared_ptr<messenger::repositories::UserRepositoryInterface> user_repo) {
        this->user_service.setUserRepo(user_repo);
    }
  private:
    UserService user_service;

};
}
}
