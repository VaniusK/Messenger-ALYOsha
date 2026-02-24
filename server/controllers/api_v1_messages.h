#pragma once

#include <drogon/HttpController.h>

using namespace drogon;

namespace api
{
namespace v1
{
class messages : public drogon::HttpController<messages>
{
  public:
    METHOD_LIST_BEGIN
    METHOD_ADD(messages::getById, "/{1:message_id}", Get);
    METHOD_LIST_END
    Task<HttpResponsePtr> getById(const HttpRequestPtr req, std::string &&message_id);
};
}
}
