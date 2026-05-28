#include "services/SecretChatsService.hpp"
#include <drogon/HttpAppFramework.h>
#include <drogon/utils/coroutine.h>
#include <json/value.h>
#include <chrono>
#include <exception>
#include <vector>
#include "dto/SecretChatsServiceDtos.hpp"
#include "include/enums/WebsocketsMessagesTypes.h"
#include "utils/server_exceptions.hpp"

namespace api::v1 {

void SecretChatsService::initAndStart(const Json::Value &config) {
    LOG_INFO << "Initializing SecretChatsService Plugin...";
    s3_service = std::make_shared<S3Service>(
        std::getenv("S3_ACCESS_KEY"), std::getenv("S3_SECRET_KEY"),
        std::getenv("S3_BASE_URL"), std::getenv("S3_PRIVATE_BUCKETNAME"),
        std::getenv("S3_SHOULD_USE_HTTPS") == std::string("true")
    );
    secret_chats_repo =
        std::make_shared<messenger::repositories::SecretChatsRepository>();
    client_notifier = std::make_shared<WebsocketClientNotifier>();

    LOG_INFO
        << "Starting SecretChatService background tasks (Cleanup timer: 24h)";

    drogon::app().getLoop()->runEvery(std::chrono::hours(24), [this]() {
        drogon::async_run([this]() -> drogon::Task<void> {
            try {
                LOG_INFO << "Running sheduled cleanup for E2E chats...";
                co_await secret_chats_repo->removeStaleRecords();
            } catch (const std::exception &e) {
                LOG_ERROR << "Exception during sheduled cleanup: " << e.what();
            }
        });
    });
}

void SecretChatsService::shutdown() {
    LOG_INFO << "Shutting down SecretChatsService plugin...";
}

drogon::Task<SecretChatInitResponseDto> SecretChatsService::chatInit(
    SecretChatInitRequestDto request_dto
) {
    if (request_dto.source_user_id == request_dto.target_user_id) {
        throw messenger::exceptions::BadRequestException(
            "Can't create secret chat with yourself."
        );
    }
    Json::Value ws_message_json = buildWebsocketJson(
        static_cast<int32_t>(WebsocketMessageType::SECRET_CHAT_REQUEST),
        request_dto.source_user_id, request_dto.initialUserPublicKey,
        "public_key"
    );
    if (!client_notifier->notifyClient(
            request_dto.target_user_id, ws_message_json.toStyledString()
        )) {
        co_await secret_chats_repo->saveHandshakeSignal(
            request_dto.source_user_id, request_dto.target_user_id,
            static_cast<int32_t>(WebsocketMessageType::SECRET_CHAT_REQUEST),
            request_dto.initialUserPublicKey
        );
    }
    co_return SecretChatInitResponseDto();
}

drogon::Task<SecretChatAcceptResponseDto> SecretChatsService::chatAccept(
    SecretChatAcceptRequestDto request_dto
) {
    Json::Value ws_message_json = buildWebsocketJson(
        static_cast<int32_t>(WebsocketMessageType::SECRET_CHAT_ACCEPT),
        request_dto.source_user_id, request_dto.acceptorUserPublicKey,
        "public_key"
    );

    if (!client_notifier->notifyClient(
            request_dto.target_user_id, ws_message_json.toStyledString()
        )) {
        co_await secret_chats_repo->saveHandshakeSignal(
            request_dto.source_user_id, request_dto.target_user_id,
            static_cast<int32_t>(WebsocketMessageType::SECRET_CHAT_ACCEPT),
            request_dto.acceptorUserPublicKey
        );
    }
    co_return SecretChatAcceptResponseDto();
}

drogon::Task<SendSecretMessageResponseDto> SecretChatsService::sendMessage(
    SendSecretMessageRequestDto request_dto
) {
    Json::Value ws_message_json = buildWebsocketJson(
        static_cast<int32_t>(WebsocketMessageType::SECRET_NEW_MESSAGE),
        request_dto.source_user_id, request_dto.encrypted_payload,
        "encrypted_payload"
    );

    if (!client_notifier->notifyClient(
            request_dto.target_user_id, ws_message_json.toStyledString()
        )) {
        co_await secret_chats_repo->saveEncryptedMessage(
            request_dto.source_user_id, request_dto.target_user_id,
            static_cast<int32_t>(WebsocketMessageType::SECRET_NEW_MESSAGE),
            request_dto.encrypted_payload
        );
    }
    co_return SendSecretMessageResponseDto();
}

drogon::Task<ReadSecretMessageResponseDto> SecretChatsService::readMessage(
    ReadSecretMessageRequestDto request_dto
) {
    Json::Value ws_message_json = buildWebsocketJson(
        static_cast<int32_t>(WebsocketMessageType::SECRET_MESSAGE_READ),
        request_dto.source_user_id, request_dto.encrypted_payload,
        "encrypted_payload"
    );

    if (!client_notifier->notifyClient(
            request_dto.target_user_id, ws_message_json.toStyledString()
        )) {
        co_await secret_chats_repo->saveEncryptedMessage(
            request_dto.source_user_id, request_dto.target_user_id,
            static_cast<int32_t>(WebsocketMessageType::SECRET_MESSAGE_READ),
            request_dto.encrypted_payload
        );
    }
    co_return ReadSecretMessageResponseDto();
}

drogon::Task<GetSecretUploadUrlsResponseDto>
SecretChatsService::getAttachmentLinks(GetSecretUploadUrlsRequestDto request_dto
) {
    std::vector<SecretUploadInfo> attachments_info;
    for (int i = 0; i < request_dto.count; i++) {
        auto result = s3_service->generateSecretChatUploadUrl();
        if (!result.has_value()) {
            throw messenger::exceptions::InternalServerErrorException(
                "Failed to generate attachment upload url"
            );
        }
        attachments_info.push_back(
            {std::move(result->attachment_key), std::move(result->upload_url)}
        );
    }

    co_return GetSecretUploadUrlsResponseDto(std::move(attachments_info));
}

drogon::Task<GetSecretDownloadUrlResponseDto>
SecretChatsService::getDownloadAttachmentLinks(
    GetSecretDownloadUrlRequestDto request_dto
) {
    std::vector<std::string> urls;
    for (const auto &key : request_dto.attachment_keys) {
        auto result = s3_service->generateDownloadUrl(key, "");
        if (!result.has_value()) {
            throw messenger::exceptions::InternalServerErrorException(
                "Failed to generate attachment download url"
            );
        }
        urls.push_back(std::move(result.value()));
    }
    co_return GetSecretDownloadUrlResponseDto(std::move(urls));
}

drogon::Task<std::vector<std::string>> SecretChatsService::syncOfflineData(
    int64_t user_id
) {
    std::vector<std::string> offline_data;
    auto handshakes = co_await secret_chats_repo->popHandshakeSignals(user_id);
    for (const auto &hs : handshakes) {
        offline_data.push_back(buildWebsocketJson(
                                   hs.message_type, hs.sender_id, hs.public_key,
                                   "public_key"
        )
                                   .toStyledString());
    }
    auto messages = co_await secret_chats_repo->popEncryptedMessages(user_id);
    for (const auto &msg : messages) {
        offline_data.push_back(buildWebsocketJson(
                                   msg.message_type, msg.sender_id, msg.payload,
                                   "encrypted_payload"
        )
                                   .toStyledString());
    }

    co_return offline_data;
}
}  // namespace api::v1
