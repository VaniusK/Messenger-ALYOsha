#pragma once

#include <drogon/HttpController.h>
#include <drogon/HttpRequest.h>
#include <drogon/HttpResponse.h>
#include <memory>
#include "repositories/SecretChatsRepository.hpp"
#include "services/SecretChatsService.hpp"
#include "services/S3Service.hpp"
#include "services/ClientNotifier.hpp"

using namespace drogon;

namespace api::v1 {
class SecretChatsController : public drogon::HttpController<SecretChatsController> {
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

    SecretChatsController(){
        secret_chats_service.setS3Service(
            std::make_shared<S3Service>(
                std::getenv("S3_ACCESS_KEY"), std::getenv("S3_SECRET_KEY"),
                std::getenv("S3_BASE_URL"),
                std::getenv("S3_PRIVATE_BUCKETNAME"),
                std::getenv("S3_SHOULD_USE_HTTPS") == std::string("true")
            )
        );
        secret_chats_service.setSecretChatRepo(std::make_shared<messenger::repositories::SecretChatsRepository>());
        secret_chats_service.setClientNotifier(std::make_shared<WebsocketClientNotifier>());
        
        secret_chats_service.startBackGroundTasks();
    }

    private:
        SecretChatsService secret_chats_service;
};
}  // namespace api::v1
