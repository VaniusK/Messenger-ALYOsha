#include "api_v1_messages.h"

using namespace api::v1;

Task<HttpResponsePtr> messages::getById(const HttpRequestPtr req, std::string &&message_id){
    // MessageRepository repo;
    // some logic with co_await
    Json::Value ret;
    ret["status"] = message_id;
    auto resp = HttpResponse::newHttpJsonResponse(ret);
    co_return resp;
}
