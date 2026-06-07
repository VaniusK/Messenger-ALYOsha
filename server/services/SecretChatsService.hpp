#include <drogon/plugins/Plugin.h>
#include <drogon/utils/coroutine.h>
#include <json/value.h>
#include <memory>
#include <vector>
#include "dto/SecretChatsServiceDtos.hpp"
#include "repositories/SecretChatsRepository.hpp"
#include "services/S3Service.hpp"

using namespace messenger::dto;

namespace api::v1 {

class SecretChatsService : public drogon::Plugin<SecretChatsService> {
public:
    void initAndStart(const Json::Value &config) override;
    void shutdown() override;

    drogon::Task<SecretChatInitResponseDto> chatInit(
        SecretChatInitRequestDto request_dto
    );
    drogon::Task<SecretChatAcceptResponseDto> chatAccept(
        SecretChatAcceptRequestDto request_dto
    );
    drogon::Task<DeleteSecretChatResponseDto> deleteChat(
        DeleteSecretChatRequestDto request_dto
    );
    drogon::Task<SendSecretMessageResponseDto> sendMessage(
        SendSecretMessageRequestDto request_dto
    );
    drogon::Task<ReadSecretMessageResponseDto> readMessage(
        ReadSecretMessageRequestDto request_dto
    );
    drogon::Task<GetSecretUploadUrlsResponseDto> getAttachmentLinks(
        GetSecretUploadUrlsRequestDto request_dto
    );
    drogon::Task<GetSecretDownloadUrlResponseDto> getDownloadAttachmentLinks(
        GetSecretDownloadUrlRequestDto request_dto
    );
    drogon::Task<std::vector<std::string>> syncOfflineData(int64_t user_id);

private:
    std::shared_ptr<messenger::repositories::SecretChatsRepositoryInterface>
        secret_chats_repo;
    std::shared_ptr<S3ServiceInterface> s3_service;

    Json::Value buildWebsocketJson(
        int32_t message_type,
        int64_t sender_id,
        std::string chat_id,
        const std::string &payload,
        const std::string &payload_field_name
    ) {
        Json::Value result;
        result["message_type"] = message_type;
        result["sender_id"] = sender_id;
        result["chat_id"] = chat_id;
        result[payload_field_name] = payload;
        return result;
    }
};

}  // namespace api::v1
