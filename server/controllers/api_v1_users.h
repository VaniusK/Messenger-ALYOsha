#pragma once

#include <drogon/HttpController.h>

using namespace drogon;

namespace api
{
namespace v1
{
class users : public drogon::HttpController<users>
{
  public:
    METHOD_LIST_BEGIN
    METHOD_ADD(users::getUserById, "/{1:user_id}", Get);
    METHOD_ADD(users::getUserByHandle, "/handle/{1:user_handle}", Get);
    METHOD_ADD(users::searchUser, "/search", Get);

    METHOD_LIST_END
    Task<HttpResponsePtr> getUserById(const HttpRequestPtr req, std::string &&user_id);
    Task<HttpResponsePtr> getUserByHandle(const HttpRequestPtr req, std::string &&user_handle);
    Task<HttpResponsePtr> searchUser(const HttpRequestPtr req);


};
}
}
