#pragma once

#include <drogon/HttpController.h>

using namespace drogon;

namespace api
{
namespace v1
{
class auth : public drogon::HttpController<auth>
{
  public:
    METHOD_LIST_BEGIN
    METHOD_ADD(auth::registerUser, "/register", Post);
    METHOD_ADD(auth::loginUser, "/login", Post);
    METHOD_LIST_END
    Task<HttpResponsePtr> registerUser(const HttpRequestPtr req);
    Task<HttpResponsePtr> loginUser(const HttpRequestPtr req);
};
}
}
