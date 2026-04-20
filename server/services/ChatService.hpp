#pragma once

#include <drogon/HttpController.h>
#include <drogon/HttpResponse.h>
#include <memory>
#include "repositories/AttachmentRepository.hpp"
#include "repositories/ChatRepository.hpp"
#include "services/S3Service.hpp"

using namespace drogon;

namespace api {
namespace v1 {
class ChatService {
public:
    Task<HttpResponsePtr> getMessageById(
        const std::shared_ptr<Json::Value> request_json,
        int64_t message_id
    );
    Task<HttpResponsePtr> getUserChats(
        const std::shared_ptr<Json::Value> request_json,
        int64_t user_id
    );
    Task<HttpResponsePtr> createOrGetDirectChat(
        const std::shared_ptr<Json::Value> request_json
    );
    Task<HttpResponsePtr> getChatMessages(
        const std::shared_ptr<Json::Value> request_json,
        int64_t chat_id
    );
    Task<HttpResponsePtr> sendMessage(
        const std::shared_ptr<Json::Value> request_json,
        int64_t chat_id
    );
    Task<HttpResponsePtr> readMessages(
        const std::shared_ptr<Json::Value> request_json,
        int64_t chat_id
    );
    Task<HttpResponsePtr> getAttachmentLink(
        const std::shared_ptr<Json::Value> request_json
    );
    Task<HttpResponsePtr> createAttachment(
        const std::shared_ptr<Json::Value> request_json
    );

    void setChatRepo(
        std::shared_ptr<messenger::repositories::ChatRepositoryInterface>
            chat_repo
    ) {
        this->chat_repo = chat_repo;
        s3_service_.setChatRepo(this->chat_repo);
    }

    void setAttachmentRepo(
        std::shared_ptr<messenger::repositories::AttachmentRepositoryInterface>
            attachment_repo
    ) {
        this->attachment_repo = attachment_repo;
    }

private:
    std::shared_ptr<messenger::repositories::ChatRepositoryInterface> chat_repo;
    std::shared_ptr<messenger::repositories::AttachmentRepositoryInterface>
        attachment_repo;
    S3Service s3_service_ = S3Service(
        std::getenv("S3_ACCESS_KEY"),
        std::getenv("S3_SECRET_KEY"),
        std::getenv("S3_BASE_URL"),
        std::getenv("S3_PRIVATE_BUCKETNAME"),
        std::getenv("S3_SHOULD_USE_HTTPS") == std::string("true")
    );
    Task<bool> checkChatAccess(int64_t user_id, int64_t chat_id);
    bool validateMessageType(const std::string &message_type);
    bool validateFileType(
        const std::string &message_type,
        const std::string &mime_type
    );
};
}  // namespace v1
}  // namespace api
