#include "services/ChatService.hpp"
#include <drogon/HttpController.h>
#include <drogon/HttpResponse.h>
#include <json/forwards.h>
#include <json/value.h>
#include <algorithm>
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
#include "models/Users.h"
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
using User = drogon_model::messenger_db::Users;

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
    bool is_member =
        co_await checkChatAccess(  // in future: delegate this to db
            request_dto.user_id, message->getValueOfChatId()
        );
    if (!is_member) {
        throw messenger::exceptions::ForbiddenException("Access denied");
    }

    std::optional<User> sender_info =
        co_await user_repo->getById(message->getValueOfSenderId());
    std::vector<Attachment> attachments =
        co_await attachment_repo->getByMessage(message->getValueOfId());
    std::vector<std::optional<std::string>> attachments_download_urls(
        attachments.size()
    );
    for (std::size_t i = 0; i < attachments.size(); i++) {
        std::optional<std::string> download_url =
            s3_service_->generateDownloadUrl(
                attachments[i].getValueOfS3ObjectKey(),
                attachments[i].getValueOfFileName()
            );
        attachments_download_urls[i] = std::move(download_url);
    }

    co_return GetMessageByIdResponseDto(
        std::move(message.value()), std::move(attachments),
        std::move(attachments_download_urls), std::move(sender_info)
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
    std::unordered_set<int64_t> unique_senders_ids;

    for (const auto &chat_preview : chats_previews) {
        if (chat_preview.last_message.has_value()) {
            message_ids.push_back(chat_preview.last_message->getValueOfId());
            unique_senders_ids.insert(
                chat_preview.last_message->getValueOfSenderId()
            );
        }
    }

    std::vector<int64_t> senders_ids(
        unique_senders_ids.begin(), unique_senders_ids.end()
    );

    std::vector<std::vector<Attachment>> fetched_attachments =
        co_await attachment_repo->getByMessages(message_ids);

    std::vector<User> fetched_senders_info;
    if (!senders_ids.empty()) {
        fetched_senders_info = co_await user_repo->getByIds(senders_ids);
    }

    std::unordered_map<int64_t, std::vector<Attachment>> attachment_map;
    attachment_map.reserve(message_ids.size());
    for (std::size_t i = 0; i < message_ids.size(); i++) {
        attachment_map[message_ids[i]] = std::move(fetched_attachments[i]);
    }

    std::unordered_map<int64_t, User> senders_map;
    senders_map.reserve(fetched_senders_info.size());
    for (auto &user : fetched_senders_info) {
        senders_map[user.getValueOfId()] = std::move(user);
    }

    std::vector<std::vector<Attachment>> last_messages_attachments;
    std::vector<std::optional<User>> last_messages_senders;
    last_messages_attachments.reserve(chats_previews.size());
    last_messages_senders.reserve(chats_previews.size());

    for (const auto &chat_preview : chats_previews) {
        if (chat_preview.last_message.has_value()) {
            int64_t msg_id = chat_preview.last_message->getValueOfId();
            int64_t sender_id = chat_preview.last_message->getValueOfSenderId();

            last_messages_attachments.push_back(std::move(attachment_map[msg_id]
            ));

            auto it = senders_map.find(sender_id);
            if (it != senders_map.end()) {
                last_messages_senders.push_back(it->second);
            } else {
                last_messages_senders.push_back(std::nullopt);
            }
        } else {
            last_messages_attachments.push_back({});
            last_messages_senders.push_back(std::nullopt);
        }
    }

    GetUserChatsResponseDto response_dto(
        std::move(chats_previews), std::move(last_messages_attachments),
        std::move(last_messages_senders)
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
        was_created = false;
        chat = co_await chat_repo->getSaved(user_id);
    } else {
        chat = co_await chat_repo->getDirect(user_id, other_user_id);
        if (chat.has_value()) {
            was_created = false;
        } else {
            chat =
                co_await chat_repo->getOrCreateDirect(user_id, other_user_id);
        }
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
                s3_service_->generateDownloadUrl(
                    attachment.getValueOfS3ObjectKey(),
                    attachment.getValueOfFileName()
                )
            );
        }
    }

    std::unordered_set<int64_t> unique_sender_ids;
    for (const auto &message : chat_messages) {
        unique_sender_ids.insert(message.getValueOfSenderId());
    }
    std::vector<int64_t> senders_ids(
        unique_sender_ids.begin(), unique_sender_ids.end()
    );

    std::vector<User> fetched_senders =
        co_await user_repo->getByIds(senders_ids);

    std::unordered_map<int64_t, User> senders_map;
    for (auto &user : fetched_senders) {
        senders_map[user.getValueOfId()] = std::move(user);
    }

    std::vector<User> senders_info;
    senders_info.reserve(chat_messages.size());
    for (const auto &message : chat_messages) {
        senders_info.push_back(senders_map[message.getValueOfSenderId()]);
    }

    GetChatMessagesResponseDto response_dto(
        std::move(chat_messages), std::move(attachments),
        std::move(attachments_download_urls), std::move(senders_info)
    );
    co_return response_dto;
}

Task<SendMessageResponseDto> ChatService::sendMessage(
    SendMessageRequestDto request_dto
) {
    int64_t user_id = request_dto.user_id;
    int64_t chat_id = request_dto.chat_id;

    bool is_member = co_await checkChatAccess(
        user_id, chat_id
    );  // in future: delegate this to db
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
            s3_service_->generateDownloadUrl(
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

    auto sender_info = co_await user_repo->getById(user_id);
    SendMessageResponseDto response_dto(
        std::move(message), std::move(created_attachments),
        std::move(attachments_download_urls), std::move(sender_info.value())
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

Task<GetAttachmentLinksResponseDto> ChatService::getAttachmentLinks(
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
        std::string ext = s3_service_->getExtension(file_name);
        std::string mime_type = s3_service_->getMimeType(ext);
        if (!validateFileType(message_type, mime_type)) {
            throw messenger::exceptions::BadRequestException(
                "Mismatch message type and file types"
            );
        }
        files_info.push_back({file_name, ext, mime_type, file.file_size_bytes});
    }
    std::optional<std::vector<UploadPresignedResult>> upload_presigned_results =
        s3_service_->generateUploadUrl(chat_id, message_type, files_info);
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

Task<CreateGroupResponseDto> ChatService::createGroup(
    CreateGroupRequestDto request_dto
) {
    auto it = std::find(
        request_dto.members_ids.begin(), request_dto.members_ids.end(),
        request_dto.creator_id
    );

    if (it == request_dto.members_ids.end()) {
        throw messenger::exceptions::BadRequestException(
            "Creator ID must be included in the members list"
        );
    }
    Chat chat = co_await chat_repo->createGroup(
        request_dto.name, request_dto.creator_id,
        std::move(request_dto.members_ids)
    );
    CreateGroupResponseDto response_dto(std::move(chat));
    co_return response_dto;
}

Task<AddGroupChatMemberResponseDto> ChatService::addGroupChatMember(
    AddGroupChatMemberRequestDto request_dto
) {
    auto request_source_member =
        co_await chat_repo->getMember(request_dto.chat_id, request_dto.user_id);
    if (request_source_member.getValueOfRole() ==
        messenger::models::ChatRole::Member) {
        throw messenger::exceptions::ForbiddenException(
            "Not enough permissions to add members to this chat"
        );
    }
    auto trans_ptr = co_await drogon::app().getDbClient()->newTransactionCoro();
    co_await chat_repo->lockChat(request_dto.chat_id, trans_ptr);
    auto chat_members =
        co_await chat_repo->getMembers(request_dto.chat_id, trans_ptr);
    if (chat_members.size() >= 50) {
        throw messenger::exceptions::ConflictException(
            "There are already 50 members in this chat"
        );
    }
    if (request_dto.role == messenger::models::ChatRole::Owner) {
        throw messenger::exceptions::BadRequestException("Cannot add new owner"
        );
    }
    auto new_member = co_await chat_repo->addMember(
        request_dto.chat_id, request_dto.new_member_id, request_dto.role,
        trans_ptr
    );
    co_await trans_ptr->execSqlCoro("COMMIT;");
    AddGroupChatMemberResponseDto response_dto(std::move(new_member));
    co_return response_dto;
}

Task<GetChatMemberResponseDto> ChatService::getChatMember(
    GetChatMemberRequestDto request_dto
) {
    bool is_member =
        co_await checkChatAccess(request_dto.user_id, request_dto.chat_id);
    if (!is_member) {
        throw messenger::exceptions::ForbiddenException(
            "Request sender is not in chat"
        );
    }
    auto member = co_await chat_repo->getMember(
        request_dto.chat_id, request_dto.member_id
    );
    auto member_info = co_await user_repo->getById(member.getValueOfUserId());
    if (!member_info.has_value()) {
        throw messenger::exceptions::NotFoundException(
            "Couldn't find user with member's user_id"
        );
    }
    GetChatMemberResponseDto response_dto(
        std::move(member), std::move(member_info.value())
    );
    co_return response_dto;
}

Task<GetChatMembersResponseDto> ChatService::getChatMembers(
    GetChatMembersRequestDto request_dto
) {
    bool is_member =
        co_await checkChatAccess(request_dto.user_id, request_dto.chat_id);
    if (!is_member) {
        throw messenger::exceptions::ForbiddenException(
            "Request sender is not in chat"
        );
    }
    auto members = co_await chat_repo->getMembers(request_dto.chat_id);
    std::vector<int64_t> members_user_ids;
    members_user_ids.reserve(members.size());
    for (auto &el : members) {
        members_user_ids.push_back(el.getValueOfUserId());
    }
    std::vector<User> fetched_members_info =
        co_await user_repo->getByIds(members_user_ids);

    std::unordered_map<int64_t, User> users_map;
    for (auto &user : fetched_members_info) {
        users_map[user.getValueOfId()] = std::move(user);
    }
    std::vector<User> aligned_members_info;
    aligned_members_info.reserve(members.size());

    for (const auto &member : members) {
        auto it = users_map.find(member.getValueOfUserId());
        if (it != users_map.end()) {
            aligned_members_info.push_back(it->second);
        } else {
            aligned_members_info.push_back(User{});
        }
    }
    GetChatMembersResponseDto response_dto(
        std::move(members), std::move(aligned_members_info)
    );
    co_return response_dto;
}

Task<RemoveMemberResponseDto> ChatService::removeMember(
    RemoveMemberRequestDto request_dto
) {
    auto request_source_member =
        co_await chat_repo->getMember(request_dto.chat_id, request_dto.user_id);
    if (request_source_member.getValueOfRole() ==
            messenger::models::ChatRole::Member &&
        request_dto.user_id != request_dto.member_id) {
        throw messenger::exceptions::ForbiddenException(
            "Not enough permissions to remove someone from chat"
        );
    }
    auto members = co_await chat_repo->getMembers(request_dto.chat_id);
    if (request_source_member.getValueOfUserId() == request_dto.member_id &&
        request_source_member.getValueOfRole() ==
            messenger::models::ChatRole::Owner &&
        members.size() > 1) {
        throw messenger::exceptions::ConflictException(
            "Owner cannot leave chat without transfer of rights"
        );
    }
    auto it = std::find_if(members.begin(), members.end(), [&](const auto &m) {
        return m.getValueOfUserId() == request_dto.member_id;
    });
    if (it == members.end()) {
        throw messenger::exceptions::NotFoundException(
            "Member not found in chat"
        );
    }
    auto to_delete_member = *it;
    if (request_source_member.getValueOfRole() ==
            messenger::models::ChatRole::Admin &&
        request_dto.user_id != request_dto.member_id &&
        to_delete_member.getValueOfRole() !=
            messenger::models::ChatRole::Member) {
        throw messenger::exceptions::ForbiddenException(
            "Admins can only remove regular members"
        );
    }
    co_await chat_repo->removeMember(
        request_dto.chat_id, request_dto.member_id
    );
    co_return RemoveMemberResponseDto();
}

Task<UpdateMemberRoleResponseDto> ChatService::updateMemberRole(
    UpdateMemberRoleRequestDto request_dto
) {
    auto request_source_member =
        co_await chat_repo->getMember(request_dto.chat_id, request_dto.user_id);
    if (request_source_member.getValueOfRole() !=
        messenger::models::ChatRole::Owner) {
        throw messenger::exceptions::ForbiddenException(
            "Not enough permissions to change members' roles"
        );
    }
    if (request_dto.user_id == request_dto.member_id &&
        request_dto.new_role != messenger::models::ChatRole::Owner) {
        throw messenger::exceptions::ConflictException(
            "Cannot demote yourself without transfer of rights"
        );
    }
    auto transaction_ptr =
        co_await drogon::app().getDbClient()->newTransactionCoro();
    co_await chat_repo->updateMemberRole(
        request_dto.chat_id, request_dto.member_id, request_dto.new_role,
        transaction_ptr
    );
    if (request_dto.new_role == messenger::models::ChatRole::Owner) {
        co_await chat_repo->updateMemberRole(
            request_dto.chat_id, request_dto.user_id,
            messenger::models::ChatRole::Admin, transaction_ptr
        );
    }
    co_await transaction_ptr->execSqlCoro("COMMIT;");
    co_return UpdateMemberRoleResponseDto();
}

Task<UpdateChatInfoResponseDto> ChatService::updateChatInfo(
    UpdateChatInfoRequestDto request_dto
) {
    auto request_source_member =
        co_await chat_repo->getMember(request_dto.chat_id, request_dto.user_id);
    if (request_source_member.getValueOfRole() ==
        messenger::models::ChatRole::Member) {
        throw messenger::exceptions::ForbiddenException(
            "Not enough permissions to edit this chat"
        );
    }
    co_await chat_repo->updateInfo(
        request_dto.chat_id, request_dto.name, request_dto.avatar,
        request_dto.description
    );
    co_return UpdateChatInfoResponseDto();
}

Task<GetChatByIdResponseDto> ChatService::getChatById(
    GetChatByIdRequestDto request_dto
) {
    // TODO: access checking if chat is private
    auto chat = co_await chat_repo->getById(request_dto.chat_id);
    if (!chat.has_value()) {
        throw messenger::exceptions::NotFoundException(
            "Chat with id " + std::to_string(request_dto.chat_id) +
            " doesn't exist"
        );
    }
    GetChatByIdResponseDto response_dto(std::move(chat.value()));
    co_return response_dto;
}