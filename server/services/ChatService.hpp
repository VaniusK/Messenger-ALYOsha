#pragma once

#include <drogon/HttpController.h>
#include <drogon/HttpResponse.h>
#include <memory>
#include "dto/ChatServiceDtos.hpp"
#include "repositories/AttachmentRepository.hpp"
#include "repositories/ChatRepository.hpp"
#include "services/S3Service.hpp"

using namespace drogon;
using namespace messenger::dto;

namespace api {
namespace v1 {
class ChatService {
public:
    Task<GetMessageByIdResponseDto> getMessageById(
        GetMessageByIdRequestDto request_dto
    );
    Task<GetUserChatsResponseDto> getUserChats(
        GetUserChatsRequestDto request_dto
    );
    Task<CreateOrGetDirectResponseDto> createOrGetDirectChat(
        CreateOrGetDirectRequestDto request_dto
    );
    Task<GetChatMessagesResponseDto> getChatMessages(
        GetChatMessagesRequestDto request_dto
    );
    Task<SendMessageResponseDto> sendMessage(SendMessageRequestDto request_dto);
    Task<ReadMessagesResponseDto> readMessages(
        ReadMessagesRequestDto request_dto
    );
    Task<GetAttachmentLinksResponseDto> getAttachmentLink(
        GetAttachmentLinksRequestDto request_dto
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
    bool validateFileType(
        const std::string &message_type,
        const std::string &mime_type
    );
};
}  // namespace v1
}  // namespace api
