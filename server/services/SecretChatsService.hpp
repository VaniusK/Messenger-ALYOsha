#include <drogon/utils/coroutine.h>
#include <memory>
#include "dto/SecretChatsServiceDtos.hpp"
#include "repositories/SecretChatsRepository.hpp"
#include "services/ClientNotifier.hpp"
#include "services/S3Service.hpp"

using namespace messenger::dto;

namespace api::v1 {

class SecretChatsService {
public:
    drogon::Task<SecretChatInitResponseDto> chatInit(
        SecretChatInitRequestDto request_dto
    );
    drogon::Task<SecretChatAcceptResponseDto> chatAccept(
        SecretChatAcceptRequestDto request_dto
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

    void setClientNotifier(
        std::shared_ptr<ClientNotifierInterface> client_notifier
    ) {
        this->client_notifier = client_notifier;
    }

    void setSecretChatRepo(
        std::shared_ptr<messenger::repositories::SecretChatsRepositoryInterface>
            secret_chats_repo
    ) {
        this->secret_chats_repo = secret_chats_repo;
    }

    void setS3Service(std::shared_ptr<S3ServiceInterface> s3_service_) {
        s3_service = s3_service_;
    }

    void startBackGroundTasks();

private:
    std::shared_ptr<ClientNotifierInterface> client_notifier;
    std::shared_ptr<messenger::repositories::SecretChatsRepositoryInterface>
        secret_chats_repo;
    std::shared_ptr<S3ServiceInterface> s3_service;

    template <typename PayloadType>
    Json::Value buildWebsocketJson(
        int32_t message_type,
        int64_t sender_id,
        const PayloadType &payload,
        const std::string &payload_field_name
    ) {
        Json::Value result;
        result["message_type"] = message_type;
        result["sender_id"] = sender_id;
        result[payload_field_name] = payload;
        return result;
    }
};

}  // namespace api::v1
