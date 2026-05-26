#pragma once

#include <drogon/HttpController.h>
#include <drogon/HttpRequest.h>
#include <drogon/HttpResponse.h>

using namespace drogon;

namespace api::v1 {
class SecretChatsController : HttpController<SecretChatsController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(
        SecretChatsController::chatInit,
        "/v1/chats/secret/init",
        Post,
        "api::v1::JsonValidatorFilter",
        "api::v1::AuthFilter"
    );
    ADD_METHOD_TO(
        SecretChatsController::chatAccept,
        "/v1/chats/secret/accept",
        Post,
        "api::v1::JsonValidatorFilter",
        "api::v1::AuthFilter"
    );
    ADD_METHOD_TO(
        SecretChatsController::sendMessage,
        "/v1/chats/secret/message",
        Post,
        "api::v1::JsonValidatorFilter",
        "api::v1::AuthFilter"
    );
    METHOD_LIST_END
    Task<HttpResponsePtr> chatInit(const HttpRequestPtr req);
    Task<HttpResponsePtr> chatAccept(const HttpRequestPtr req);
    Task<HttpResponsePtr> sendMessage(const HttpRequestPtr req);
};
}  // namespace api::v1
