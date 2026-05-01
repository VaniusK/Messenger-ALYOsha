#include "services/ChatService.hpp"
#include <drogon/HttpController.h>
#include <drogon/HttpResponse.h>
#include <json/forwards.h>
#include <json/value.h>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>
#include "controllers/ServerWebSocketController.h"
#include "dto/AttachmentData.hpp"
#include "dto/ChatServiceDtos.hpp"
#include "include/repositories/ChatRepository.hpp"
#include "jwt-cpp/jwt.h"
#include "jwt-cpp/traits/kazuho-picojson/defaults.h"
#include "models/Messages.h"
#include "services/S3Service.hpp"
#include "utils/Enum.hpp"
#include "utils/server_exceptions.hpp"
#include "utils/server_response_macro.hpp"

using namespace drogon;
using namespace api::v1;
using namespace minio::s3;

using ChatRepo = messenger::repositories::ChatRepository;
using Message = drogon_model::messenger_db::Messages;
using Chat = drogon_model::messenger_db::Chats;
using ChatPreview = messenger::dto::ChatPreview;
using Attachment = drogon_model::messenger_db::Attachments;

Task<bool> ChatService::checkChatAccess(int64_t user_id, int64_t chat_id) {
    std::vector<messenger::repositories::ChatMember> chat_members =
        co_await chat_repo->getMembers(chat_id);
    bool is_member = false;
    for (const auto &chat_member : chat_members) {
        if (chat_member.getValueOfUserId() == user_id) {
            is_member = true;
            break;
        }
    }
    co_return is_member;
}

Task<GetMessageByIdResponseDto> ChatService::getMessageById(
    GetMessageByIdRequestDto request_dto
) {
    std::optional<Message> message =
        co_await chat_repo->getMessageById(request_dto.message_id);
    if (!message.has_value()) {
        throw messenger::exceptions::NotFoundException(
            "Message with id " + std::to_string(request_dto.message_id) +
            " doesn't exist"
        );
    }
    bool is_member = co_await checkChatAccess(
        request_dto.user_id, message->getValueOfChatId()
    );
    if (!is_member) {
        throw messenger::exceptions::ForbiddenException("Access denied");
    }

    std::vector<Attachment> attachments =
        co_await attachment_repo->getByMessage(message->getValueOfId());
    std::vector<std::optional<std::string>> attachments_download_urls(
        attachments.size()
    );
    for (std::size_t i = 0; i < attachments.size(); i++) {
        std::optional<std::string> download_url =
            s3_service_.generateDownloadUrl(
                attachments[i].getValueOfS3ObjectKey(),
                attachments[i].getValueOfFileName()
            );
        attachments_download_urls[i] = std::move(download_url);
    }

    co_return GetMessageByIdResponseDto(
        std::move(message.value()), std::move(attachments),
        std::move(attachments_download_urls)
    );
}

Task<GetUserChatsResponseDto> ChatService::getUserChats(
    GetUserChatsRequestDto request_dto
) {
    if (request_dto.from_request_user_id != request_dto.from_token_user_id) {
        throw messenger::exceptions::ForbiddenException("Access denied");
    }
    std::vector<ChatPreview> chats_previews =
        co_await chat_repo->getByUser(request_dto.from_request_user_id);

    std::vector<int64_t> message_ids;
    for (const auto &chat_preview : chats_previews) {
        if (chat_preview.last_message.has_value()) {
            message_ids.push_back(chat_preview.last_message->getValueOfId());
        }
    }

    std::vector<std::vector<Attachment>> fetched_attachments =
        co_await attachment_repo->getByMessages(message_ids);

    std::unordered_map<int64_t, std::vector<Attachment>> attachment_map;
    attachment_map.reserve(message_ids.size());
    for (std::size_t i = 0; i < message_ids.size(); i++) {
        attachment_map[message_ids[i]] = std::move(fetched_attachments[i]);
    }
    std::vector<std::vector<Attachment>> last_messages_attachments;
    last_messages_attachments.reserve(chats_previews.size());

    for (const auto &chat_preview : chats_previews) {
        if (chat_preview.last_message.has_value()) {
            int64_t msg_id = chat_preview.last_message->getValueOfId();
            last_messages_attachments.push_back(std::move(attachment_map[msg_id]
            ));
        } else {
            last_messages_attachments.push_back({});
        }
    }
    GetUserChatsResponseDto response_dto(
        std::move(chats_previews), std::move(last_messages_attachments)
    );
    co_return response_dto;
}

Task<CreateOrGetDirectResponseDto> ChatService::createOrGetDirectChat(
    CreateOrGetDirectRequestDto request_dto
) {
    int64_t user_id = request_dto.user_id;
    int64_t other_user_id = request_dto.target_user_id;
    std::optional<Chat> chat;
    bool was_created = true;
    if (user_id == other_user_id) {
        chat = co_await chat_repo->getSaved(user_id);
    } else {
        chat = co_await chat_repo->getDirect(user_id, other_user_id);
        if (chat.has_value()) {
            was_created = false;
        }
        chat = co_await chat_repo->getOrCreateDirect(user_id, other_user_id);
    }
    CreateOrGetDirectResponseDto response_dto(
        std::move(chat.value()), was_created
    );
    co_return response_dto;
}

Task<GetChatMessagesResponseDto> ChatService::getChatMessages(
    GetChatMessagesRequestDto request_dto
) {
    int64_t user_id = request_dto.user_id;
    int64_t chat_id = request_dto.chat_id;
    bool is_member = co_await checkChatAccess(user_id, chat_id);
    if (!is_member) {
        throw messenger::exceptions::ForbiddenException("Access denied");
    }

    std::vector<Message> chat_messages = co_await chat_repo->getMessagesByChat(
        chat_id, request_dto.before_message_id, request_dto.limit
    );
    std::vector<int64_t> message_ids;
    message_ids.reserve(chat_messages.size());
    for (const auto &message : chat_messages) {
        message_ids.push_back(message.getValueOfId());
    }
    std::vector<std::vector<Attachment>> attachments =
        co_await attachment_repo->getByMessages(message_ids);
    std::vector<std::vector<std::optional<std::string>>>
        attachments_download_urls(attachments.size());
    for (std::size_t i = 0; i < attachments.size(); i++) {
        for (const auto &attachment : attachments[i]) {
            attachments_download_urls[i].push_back(
                s3_service_.generateDownloadUrl(
                    attachment.getValueOfS3ObjectKey(),
                    attachment.getValueOfFileName()
                )
            );
        }
    }
    GetChatMessagesResponseDto response_dto(
        std::move(chat_messages), std::move(attachments),
        std::move(attachments_download_urls)
    );
    co_return response_dto;
}

Task<SendMessageResponseDto> ChatService::sendMessage(
    SendMessageRequestDto request_dto
) {
    int64_t user_id = request_dto.user_id;
    int64_t chat_id = request_dto.chat_id;

    bool is_member = co_await checkChatAccess(user_id, chat_id);
    if (!is_member) {
        throw messenger::exceptions::ForbiddenException("Access denied");
    }
    std::string text = request_dto.text;
    std::optional<int64_t> reply_to_id = request_dto.reply_to_id;
    std::optional<int64_t> forward_from_id = request_dto.forward_from_id;
    std::string message_type = request_dto.message_type;

    std::vector<messenger::dto::AttachmentData> attachments_info;
    if (!request_dto.attachment_tokens.empty()) {
        const char *env_key = std::getenv("JWT_KEY");
        if (!env_key) {
            throw messenger::exceptions::InternalServerErrorException(
                "JWT_KEY is not set"
            );
        }
        const std::string key = env_key;
        auto verifier = jwt::verify()
                            .allow_algorithm(jwt::algorithm::hs256{key})
                            .with_issuer("alesha_messenger");

        for (const auto &token : request_dto.attachment_tokens) {
            try {
                auto decoded = jwt::decode(token);
                verifier.verify(decoded);

                std::string token_message_type =
                    decoded.get_payload_claim("message_type").as_string();
                if (token_message_type != message_type) {
                    throw messenger::exceptions::BadRequestException(
                        "Attachment token file type mismatch"
                    );
                }
                messenger::dto::AttachmentData info;
                info.file_name =
                    decoded.get_payload_claim("file_name").as_string();
                info.file_size_bytes = std::stoll(
                    decoded.get_payload_claim("file_size_bytes").as_string()
                );
                info.s3_object_key =
                    decoded.get_payload_claim("object_key").as_string();
                info.file_type =
                    decoded.get_payload_claim("file_type").as_string();

                attachments_info.push_back(info);
            } catch (const std::exception &e) {
                LOG_ERROR << "Invalid JWT token " << e.what();
                throw messenger::exceptions::BadRequestException(
                    "Invalid or expired attachment token"
                );
            }
        }
    }

    auto [message, created_attachments] = co_await chat_repo->sendMessage(
        chat_id, user_id, text, reply_to_id, forward_from_id, message_type,
        attachments_info
    );
    std::vector<std::optional<std::string>> attachments_download_urls(
        created_attachments.size()
    );
    for (std::size_t i = 0; i < created_attachments.size(); i++) {
        std::optional<std::string> download_url =
            s3_service_.generateDownloadUrl(
                created_attachments[i].getValueOfS3ObjectKey(),
                created_attachments[i].getValueOfFileName()
            );
        attachments_download_urls[i] = std::move(download_url);
    }

    bool successfully_read_sended_message = co_await chat_repo->markAsRead(
        message.getValueOfChatId(), user_id, message.getValueOfId()
    );
    if (!successfully_read_sended_message) {
        LOG_WARN << "Couldnt't mark message as read";
    }

    SendMessageResponseDto response_dto(
        std::move(message), std::move(created_attachments),
        std::move(attachments_download_urls)
    );

    Json::Value websocket_message_json;
    websocket_message_json["event_type"] = "NEW_MESSAGE";
    websocket_message_json["data"] = response_dto.toJson();
    std::vector<messenger::repositories::ChatMember> chat_members =
        co_await chat_repo->getMembers(chat_id);
    for (const auto &chat_member : chat_members) {
        if (chat_member.getValueOfUserId() != user_id) {
            ServerWebSocketController::notifyUser(
                chat_member.getValueOfUserId(),
                websocket_message_json.toStyledString()
            );
        }
    }

    co_return response_dto;
}

Task<ReadMessagesResponseDto> ChatService::readMessages(
    ReadMessagesRequestDto request_dto
) {
    bool success = co_await chat_repo->markAsRead(
        request_dto.chat_id, request_dto.user_id,
        request_dto.last_read_message_id
    );
    co_return ReadMessagesResponseDto(success);
}

bool ChatService::validateFileType(
    const std::string &message_type,
    const std::string &mime_type
) {
    if (message_type == messenger::models::MessageType::Media) {
        if (mime_type.find("image/") != 0 && mime_type.find("video/") != 0) {
            return false;
        }
    }
    if (message_type == messenger::models::MessageType::Voice) {
        if (mime_type.find("audio/") != 0) {
            return false;
        }
    }
    return true;
}

Task<GetAttachmentLinksResponseDto> ChatService::getAttachmentLink(
    GetAttachmentLinksRequestDto request_dto
) {
    int64_t user_id = request_dto.user_id;
    int64_t chat_id = request_dto.chat_id;
    bool is_member = co_await checkChatAccess(user_id, chat_id);
    if (!is_member) {
        throw messenger::exceptions::ForbiddenException("Access denied");
    }

    std::string message_type = request_dto.message_type;

    std::vector<AttachmentFileInfo> files_info;
    for (const auto &file : request_dto.files) {
        std::string file_name = file.original_filename;
        std::string ext = s3_service_.getExtension(file_name);
        std::string mime_type = s3_service_.getMimeType(ext);
        if (!validateFileType(message_type, mime_type)) {
            throw messenger::exceptions::BadRequestException(
                "Mismatch message type and file types"
            );
        }
        files_info.push_back({file_name, ext, mime_type});
    }
    std::optional<std::vector<UploadPresignedResult>> upload_presigned_results =
        s3_service_.generateUploadUrl(chat_id, message_type, files_info);
    if (!upload_presigned_results.has_value()) {
        throw messenger::exceptions::InternalServerErrorException(
            "Failed to generate presigned URLs"
        );
    }

    const char *env_key = std::getenv("JWT_KEY");
    if (!env_key) {
        throw messenger::exceptions::InternalServerErrorException(
            "JWT_KEY is not set"
        );
    }
    const std::string jwt_key = env_key;
    std::vector<std::string> tokens(upload_presigned_results.value().size());
    for (std::size_t i = 0; i < upload_presigned_results.value().size(); i++) {
        const auto &file = upload_presigned_results.value()[i];
        auto token =
            jwt::create()
                .set_issuer("alesha_messenger")
                .set_type("JWT")
                .set_issued_at(std::chrono::system_clock::now())
                .set_expires_at(
                    std::chrono::system_clock::now() + std::chrono::hours(2)
                )
                .set_payload_claim(
                    "object_key", jwt::claim(file.attachment_key)
                )
                .set_payload_claim("file_type", jwt::claim(file.content_type))
                .set_payload_claim("file_name", jwt::claim(file.file_name))
                .set_payload_claim(
                    "file_size_bytes",
                    jwt::claim(std::to_string(file.file_size_bytes))
                )
                .set_payload_claim("message_type", jwt::claim(message_type))
                .sign(jwt::algorithm::hs256{jwt_key});
        tokens[i] = std::move(token);
    }

    co_return GetAttachmentLinksResponseDto(
        std::move(upload_presigned_results.value()), std::move(tokens)
    );
}
