#pragma once

#include <drogon/HttpController.h>
#include <drogon/HttpRequest.h>
#include <drogon/HttpResponse.h>
#include "services/SecretChatsService.hpp"

using namespace drogon;

namespace api::v1 {
class SecretChatsController
    : public drogon::HttpController<SecretChatsController> {
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
    ADD_METHOD_TO(
        SecretChatsController::readMessage,
        "/v1/chats/secret/message/read",
        Post,
        "api::v1::JsonValidatorFilter",
        "api::v1::AuthFilter"
    );
    ADD_METHOD_TO(
        SecretChatsController::getAttachmentLink,
        "/v1/chats/secret/attachments/upload",
        Get,
        "api::v1::AuthFilter"
    );
    ADD_METHOD_TO(
        SecretChatsController::getDownloadAttachmentLink,
        "/v1/chats/secret/attachments/download",
        Post,
        "api::v1::AuthFilter"
    );
    METHOD_LIST_END
    Task<HttpResponsePtr> chatInit(const HttpRequestPtr req);
    Task<HttpResponsePtr> chatAccept(const HttpRequestPtr req);
    Task<HttpResponsePtr> sendMessage(const HttpRequestPtr req);
    Task<HttpResponsePtr> readMessage(const HttpRequestPtr req);
    Task<HttpResponsePtr> getAttachmentLink(const HttpRequestPtr req);
    Task<HttpResponsePtr> getDownloadAttachmentLink(const HttpRequestPtr req);
};
}  // namespace api::v1
