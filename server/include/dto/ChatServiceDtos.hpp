#pragma once
#include <drogon/HttpRequest.h>
#include <json/value.h>
#include <jsoncpp/json/value.h>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include "ServicesDtoBase.hpp"
#include "dto/ChatPreview.hpp"
#include "dto/ServicesDtoBase.hpp"
#include "models/Attachments.h"
#include "models/ChatMembers.h"
#include "models/Chats.h"
#include "models/Users.h"
#include "services/S3Service.hpp"

using User = drogon_model::messenger_db::Users;
using Chat = drogon_model::messenger_db::Chats;
using ChatPreview = messenger::dto::ChatPreview;
using Attachment = drogon_model::messenger_db::Attachments;
using ChatMember = drogon_model::messenger_db::ChatMembers;

namespace messenger::dto {

struct GetUserChatsRequestDto : RequestDto {
    int64_t from_token_user_id;
    int64_t from_request_user_id;

    GetUserChatsRequestDto(
        int64_t from_token_user_id_,
        int64_t from_request_user_id_
    )
        : from_token_user_id(from_token_user_id_),
          from_request_user_id(from_request_user_id_) {
    }

    GetUserChatsRequestDto(
        drogon::HttpRequestPtr req,
        int64_t from_request_user_id_
    ) {
        from_token_user_id = req->getAttributes()->get<int64_t>("user_id");
        from_request_user_id = from_request_user_id_;
    }
};

struct GetUserChatsResponseDto : ResponseDto {
    std::vector<ChatPreview> chats_previews;

    std::vector<std::vector<Attachment>> last_message_attachments;

    GetUserChatsResponseDto() = default;

    GetUserChatsResponseDto(
        std::vector<ChatPreview> chats_previews_,
        std::vector<std::vector<Attachment>> last_message_attachments_
    )
        : chats_previews(std::move(chats_previews_)),
          last_message_attachments(std::move(last_message_attachments_)) {
    }

    Json::Value toJson() override {
        Json::Value response_json;
        Json::Value jsonArray(Json::arrayValue);
        for (std::size_t i = 0; i < chats_previews.size(); i++) {
            const auto &chat_preview = chats_previews[i];
            Json::Value chat_json;
            chat_json["chat_id"] = chat_preview.chat_id;
            chat_json["title"] = chat_preview.title;
            chat_json["avatar_path"] = chat_preview.avatar_path.has_value()
                                           ? chat_preview.avatar_path.value()
                                           : "";
            chat_json["last_message"] =
                chat_preview.last_message.has_value()
                    ? chat_preview.last_message.value().toJson()
                    : Json::Value();
            if (chat_json["last_message"].isMember("id")) {
                chat_json["last_message"]["attachments"] =
                    Json::Value(Json::arrayValue);
                for (const auto &attachment : last_message_attachments[i]) {
                    chat_json["last_message"]["attachments"].append(
                        attachment.toJson()
                    );
                }
            }
            chat_json["unread_count"] = chat_preview.unread_count;
            chat_json["type"] = chat_preview.type;
            jsonArray.append(chat_json);
        }
        response_json["chats"] = jsonArray;
        return response_json;
    }
};

struct CreateOrGetDirectRequestDto : RequestDto {
    int64_t user_id;
    int64_t target_user_id;

    CreateOrGetDirectRequestDto(int64_t user_id_, int64_t target_user_id_)
        : user_id(user_id_), target_user_id(target_user_id_) {
    }

    CreateOrGetDirectRequestDto(
        drogon::HttpRequestPtr req,
        std::shared_ptr<Json::Value> request_json
    ) {
        user_id = req->getAttributes()->get<int64_t>("user_id");
        target_user_id = (*request_json)["target_user_id"].asInt64();
    }
};

struct CreateOrGetDirectResponseDto : ResponseDto {
    Chat chat;
    bool was_created;

    CreateOrGetDirectResponseDto() = default;

    CreateOrGetDirectResponseDto(Chat chat_, bool was_created_)
        : chat(std::move(chat_)), was_created(was_created_) {
    }

    Json::Value toJson() override {
        Json::Value response_json;
        response_json["chat"] = chat.toJson();
        return response_json;
    }
};

struct GetChatMessagesRequestDto : RequestDto {
    int64_t chat_id;
    int64_t user_id;
    std::optional<int64_t> before_message_id;
    int64_t limit;

    GetChatMessagesRequestDto(
        int64_t chat_id_,
        int64_t user_id_,
        std::optional<int64_t> before_message_id_,
        int64_t limit_
    )
        : chat_id(chat_id_),
          user_id(user_id_),
          before_message_id(before_message_id_),
          limit(limit_) {
    }

    GetChatMessagesRequestDto(drogon::HttpRequestPtr req, int64_t chat_id_) {
        user_id = req->getAttributes()->get<int64_t>("user_id");
        chat_id = chat_id_;
        std::string limit_str = req->getParameter("limit");
        if (!limit_str.empty()) {
            try {
                limit = std::stoll(limit_str);
                limit = std::clamp<int64_t>(limit, 1, 100);
            } catch (...) {
                limit = 50;
            }
        } else {
            limit = 50;
        }

        std::string before_id_str = req->getParameter("before_id");
        if (!before_id_str.empty()) {
            try {
                before_message_id = std::stoll(before_id_str);
            } catch (...) {
                before_message_id = std::nullopt;
            }
        } else {
            before_message_id = std::nullopt;
        }
    }
};

struct GetChatMessagesResponseDto : ResponseDto {
    std::vector<Message> messages;
    std::vector<std::vector<Attachment>> attachments;
    std::vector<std::vector<std::optional<std::string>>>
        attachments_download_urls;

    GetChatMessagesResponseDto() = default;

    GetChatMessagesResponseDto(
        std::vector<Message> messages_,
        std::vector<std::vector<Attachment>> attachments_,
        std::vector<std::vector<std::optional<std::string>>>
            attachments_download_urls_
    )
        : messages(messages_),
          attachments(attachments_),
          attachments_download_urls(attachments_download_urls_) {
    }

    Json::Value toJson() override {
        Json::Value response_json;
        Json::Value jsonArray(Json::arrayValue);
        for (std::size_t i = 0; i < messages.size(); i++) {
            Json::Value message_json = messages[i].toJson();
            message_json["attachments"] = Json::Value(Json::arrayValue);
            for (std::size_t j = 0; j < attachments[i].size(); j++) {
                const auto &attachment = attachments[i][j];
                Json::Value attachment_json = attachment.toJson();
                std::optional<std::string> download_url =
                    attachments_download_urls[i][j];
                attachment_json["download_url"] =
                    download_url.has_value() ? download_url.value() : "";
                message_json["attachments"].append(attachment_json);
            }
            jsonArray.append(message_json);
        }
        response_json["messages"] = jsonArray;
        return response_json;
    }
};

struct SendMessageRequestDto : RequestDto {
    int64_t user_id;
    int64_t chat_id;
    std::string text;
    std::string message_type;
    std::optional<int64_t> reply_to_id;
    std::optional<int64_t> forward_from_id;
    std::vector<std::string> attachment_tokens;

    SendMessageRequestDto(
        int64_t user_id_,
        int64_t chat_id_,
        std::string text_,
        std::string message_type_,
        std::optional<int64_t> reply_to_id_,
        std::optional<int64_t> forward_from_id_,
        std::vector<std::string> attachment_tokens_
    )
        : user_id(user_id_),
          chat_id(chat_id_),
          text(std::move(text_)),
          message_type(std::move(message_type_)),
          reply_to_id(reply_to_id_),
          forward_from_id(forward_from_id_),
          attachment_tokens(std::move(attachment_tokens_)) {
    }

    SendMessageRequestDto(
        drogon::HttpRequestPtr req,
        std::shared_ptr<Json::Value> request_json,
        int64_t chat_id_
    ) {
        user_id = req->getAttributes()->get<int64_t>("user_id");
        chat_id = chat_id_;
        text = (*request_json)["text"].asString();
        reply_to_id = request_json->isMember("reply_to_id")
                          ? std::optional<int64_t>(
                                (*request_json)["reply_to_id"].asInt64()
                            )
                          : std::nullopt;
        forward_from_id = request_json->isMember("forward_from_id")
                              ? std::optional<int64_t>(
                                    (*request_json)["forward_from_id"].asInt64()
                                )
                              : std::nullopt;
        message_type = (*request_json)["type"].asString();
        if (request_json->isMember("attachment_tokens") &&
            (*request_json)["attachment_tokens"].isArray()) {
            for (const auto &token : (*request_json)["attachment_tokens"]) {
                attachment_tokens.push_back(token.asString());
            }
        }
    }
};

struct SendMessageResponseDto : ResponseDto {
    Message message;
    std::vector<Attachment> attachments;
    std::vector<std::optional<std::string>> attachments_download_urls;

    SendMessageResponseDto() = default;

    SendMessageResponseDto(
        Message message_,
        std::vector<Attachment> attachments_,
        std::vector<std::optional<std::string>> attachments_download_urls_
    )
        : message(std::move(message_)),
          attachments(std::move(attachments_)),
          attachments_download_urls(std::move(attachments_download_urls_)) {
    }

    Json::Value toJson() override {
        Json::Value response_json;
        response_json["message"] = message.toJson();
        Json::Value json_attachments_array(Json::arrayValue);
        for (std::size_t i = 0; i < attachments.size(); i++) {
            const auto &attachment = attachments[i];
            Json::Value attachment_json = attachment.toJson();
            std::optional<std::string> download_url =
                attachments_download_urls[i];
            attachment_json["download_url"] =
                download_url.has_value() ? download_url.value() : "";
            json_attachments_array.append(attachment_json);
        }
        response_json["message"]["attachments"] = json_attachments_array;
        return response_json;
    }
};

struct ReadMessagesRequestDto : RequestDto {
    int64_t user_id;
    int64_t chat_id;
    int64_t last_read_message_id;

    ReadMessagesRequestDto(
        int64_t user_id_,
        int64_t chat_id_,
        int64_t last_read_message_id_
    )
        : user_id(user_id_),
          chat_id(chat_id_),
          last_read_message_id(last_read_message_id_) {
    }

    ReadMessagesRequestDto(
        drogon::HttpRequestPtr req,
        std::shared_ptr<Json::Value> request_json,
        int64_t chat_id_
    ) {
        user_id = req->getAttributes()->get<int64_t>("user_id");
        chat_id = chat_id_;
        last_read_message_id =
            (*request_json)["last_read_message_id"].asInt64();
    }
};

struct ReadMessagesResponseDto : ResponseDto {
    bool success;

    ReadMessagesResponseDto() = default;

    ReadMessagesResponseDto(bool success_) : success(success_) {
    }

    Json::Value toJson() {
        Json::Value response_json;
        if (success) {
            response_json["message"] = "Succussfully read last messages";
        } else {
            response_json["message"] = "Failed to mark messages as read";
        }
        return response_json;
    }
};

struct GetMessageByIdRequestDto : RequestDto {
    int64_t message_id;
    int64_t user_id;

    GetMessageByIdRequestDto(int64_t message_id_, int64_t user_id_)
        : message_id(message_id_), user_id(user_id_) {
    }

    GetMessageByIdRequestDto(drogon::HttpRequestPtr req, int64_t message_id_) {
        user_id = req->getAttributes()->get<int64_t>("user_id");
        message_id = message_id_;
    }
};

struct GetMessageByIdResponseDto : ResponseDto {
    Message message;
    std::vector<Attachment> attachments;
    std::vector<std::optional<std::string>> attachments_download_urls;

    GetMessageByIdResponseDto() = default;

    GetMessageByIdResponseDto(
        Message message_,
        std::vector<Attachment> attachments_,
        std::vector<std::optional<std::string>> attachments_download_urls_
    )
        : message(std::move(message_)),
          attachments(std::move(attachments_)),
          attachments_download_urls(std::move(attachments_download_urls_)) {
    }

    Json::Value toJson() override {
        Json::Value response_json;
        response_json["message"] = message.toJson();
        Json::Value json_attachments_array(Json::arrayValue);
        for (std::size_t i = 0; i < attachments.size(); i++) {
            const auto &attachment = attachments[i];
            Json::Value attachment_json = attachment.toJson();
            std::optional<std::string> download_url =
                attachments_download_urls[i];
            attachment_json["download_url"] =
                download_url.has_value() ? download_url.value() : "";
            json_attachments_array.append(attachment_json);
        }
        response_json["message"]["attachments"] = json_attachments_array;
        return response_json;
    }
};

struct AttachmentRequestDto : RequestDto {
    std::string original_filename;
    int64_t file_size_bytes;

    AttachmentRequestDto(
        std::string original_filename_,
        int64_t file_size_bytes_
    )
        : original_filename(std::move(original_filename_)),
          file_size_bytes(file_size_bytes_) {
    }
};

struct GetAttachmentLinksRequestDto : RequestDto {
    int64_t user_id;
    int64_t chat_id;
    std::string message_type;
    std::vector<AttachmentRequestDto> files;

    GetAttachmentLinksRequestDto(
        int64_t user_id_,
        int64_t chat_id_,
        std::string message_type_,
        std::vector<AttachmentRequestDto> files_
    )
        : user_id(user_id_),
          chat_id(chat_id_),
          message_type(std::move(message_type_)),
          files(std::move(files_)) {
    }

    GetAttachmentLinksRequestDto(
        drogon::HttpRequestPtr req,
        std::shared_ptr<Json::Value> request_json
    ) {
        user_id = req->getAttributes()->get<int64_t>("user_id");
        chat_id = (*request_json)["chat_id"].asInt64();
        message_type = (*request_json)["message_type"].asString();
        for (const auto &file : (*request_json)["files"]) {
            files.push_back(
                {file["original_filename"].asString(),
                 file["file_size_bytes"].asInt64()}
            );
        }
    }
};

struct GetAttachmentLinksResponseDto : ResponseDto {
    std::vector<api::v1::UploadPresignedResult> attachments;
    std::vector<std::string> tokens;

    GetAttachmentLinksResponseDto() = default;

    GetAttachmentLinksResponseDto(
        std::vector<api::v1::UploadPresignedResult> attachments_,
        std::vector<std::string> tokens_
    )
        : attachments(std::move(attachments_)), tokens(std::move(tokens_)) {
    }

    Json::Value toJson() {
        Json::Value response_json;
        Json::Value attachments_array(Json::arrayValue);
        for (std::size_t i = 0; i < attachments.size(); i++) {
            Json::Value file_json;
            const auto &file = attachments[i];
            file_json["upload_url"] = file.upload_url;
            file_json["attachment_key"] = file.attachment_key;
            file_json["content_type"] = file.content_type;
            file_json["token"] = tokens[i];
            attachments_array.append(file_json);
        }
        response_json["attachments"] = attachments_array;
        return response_json;
    }
};

struct CreateGroupRequestDto : RequestDto {
    int64_t creator_id;
    std::string name;
    std::vector<int64_t> members_ids;

    CreateGroupRequestDto(std::string name_, std::vector<int64_t> members_ids_)
        : name(std::move(name_)), members_ids(std::move(members_ids_)) {
    }

    CreateGroupRequestDto(
        drogon::HttpRequestPtr req,
        std::shared_ptr<Json::Value> request_json
    ) {
        creator_id = req->getAttributes()->get<int64_t>("user_id");
        name = (*request_json)["chat_name"].asString();
        if (request_json->isMember("members") &&
            (*request_json)["members"].isArray()) {
            for (const auto &id : (*request_json)["members"]) {
                members_ids.push_back(id.asInt64());
            }
        }
    }
};

struct CreateGroupResponseDto : ResponseDto {
    Chat chat;

    CreateGroupResponseDto() = default;

    CreateGroupResponseDto(Chat chat_) : chat(std::move(chat_)) {
    }

    Json::Value toJson() {
        Json::Value response_json;
        response_json["chat"] = chat.toJson();
        return response_json;
    }
};

struct AddGroupChatMemberRequestDto : RequestDto {
    int64_t user_id;
    int64_t chat_id;
    int64_t new_member_id;
    std::string role;

    AddGroupChatMemberRequestDto(
        int64_t user_id_,
        int64_t chat_id_,
        int64_t new_member_id_,
        std::string role_
    )
        : user_id(user_id_),
          chat_id(chat_id_),
          new_member_id(new_member_id_),
          role(std::move(role_)) {
    }

    AddGroupChatMemberRequestDto(drogon::HttpRequestPtr req, int64_t chat_id_) {
        user_id = req->getAttributes()->get<int64_t>("user_id");
        auto request_json = req->getJsonObject();
        chat_id = chat_id_;
        new_member_id = (*request_json)["user_id"].asInt64();
        role = (*request_json)["role"].asString();
    }
};

struct AddGroupChatMemberResponseDto : ResponseDto {
    ChatMember chat_member;

    AddGroupChatMemberResponseDto() = default;

    AddGroupChatMemberResponseDto(ChatMember chat_member_)
        : chat_member(std::move(chat_member_)) {
    }

    Json::Value toJson() {
        Json::Value response_json;
        response_json["chat_member"] = chat_member.toJson();
        return response_json;
    }
};

struct GetChatMemberRequestDto : RequestDto {
    int64_t user_id;
    int64_t chat_id;
    int64_t member_id;

    GetChatMemberRequestDto(
        int64_t user_id_,
        int64_t chat_id_,
        int64_t member_id_
    )
        : user_id(user_id_), chat_id(chat_id_), member_id(member_id_) {
    }

    GetChatMemberRequestDto(
        drogon::HttpRequestPtr req,
        int64_t chat_id_,
        int64_t member_id_
    ) {
        user_id = req->getAttributes()->get<int64_t>("user_id");
        chat_id = chat_id_;
        member_id = member_id_;
    }
};

struct GetChatMemberResponseDto : ResponseDto {
    ChatMember member;

    GetChatMemberResponseDto() = default;

    GetChatMemberResponseDto(ChatMember member_) : member(std::move(member_)) {
    }

    Json::Value toJson() {
        Json::Value response_json;
        response_json["chat_member"] = member.toJson();
        return response_json;
    }
};

struct GetChatMembersRequestDto : RequestDto {
    int64_t user_id;
    int64_t chat_id;

    GetChatMembersRequestDto(int64_t user_id_, int64_t chat_id_)
        : user_id(user_id_), chat_id(chat_id_) {
    }

    GetChatMembersRequestDto(drogon::HttpRequestPtr req, int64_t chat_id_) {
        user_id = req->getAttributes()->get<int64_t>("user_id");
        chat_id = chat_id_;
    }
};

struct GetChatMembersResponseDto : ResponseDto {
    std::vector<ChatMember> members;

    GetChatMembersResponseDto() = default;

    GetChatMembersResponseDto(std::vector<ChatMember> members_)
        : members(std::move(members_)) {
    }

    Json::Value toJson() {
        Json::Value response_json;
        Json::Value json_array(Json::arrayValue);
        for (const auto &member : members) {
            json_array.append(member.toJson());
        }
        response_json["members"] = json_array;
        return response_json;
    }
};

struct RemoveMemberRequestDto : RequestDto {
    int64_t user_id;
    int64_t chat_id;
    int64_t member_id;

    RemoveMemberRequestDto(
        int64_t user_id_,
        int64_t chat_id_,
        int64_t member_id_
    )
        : user_id(user_id_), chat_id(chat_id_), member_id(member_id_) {
    }

    RemoveMemberRequestDto(
        drogon::HttpRequestPtr req,
        int64_t chat_id_,
        int64_t member_id_
    ) {
        user_id = req->getAttributes()->get<int64_t>("user_id");
        chat_id = chat_id_;
        member_id = member_id_;
    }
};

struct RemoveMemberResponseDto : ResponseDto {
    RemoveMemberResponseDto() = default;

    Json::Value toJson() {
        Json::Value response_json;
        response_json["message"] = "Successfully removed member";
        return response_json;
    }
};

struct UpdateMemberRoleRequestDto : RequestDto {
    int64_t user_id;
    int64_t chat_id;
    int64_t member_id;
    std::string new_role;

    UpdateMemberRoleRequestDto(
        int64_t user_id_,
        int64_t chat_id_,
        int64_t member_id_,
        std::string new_role_
    )
        : user_id(user_id_),
          chat_id(chat_id_),
          member_id(member_id_),
          new_role(std::move(new_role_)) {
    }

    UpdateMemberRoleRequestDto(
        drogon::HttpRequestPtr req,
        std::shared_ptr<Json::Value> request_json,
        int64_t chat_id_,
        int64_t member_id_
    ) {
        user_id = req->getAttributes()->get<int64_t>("user_id");
        chat_id = chat_id_;
        member_id = member_id_;
        new_role = (*request_json)["role"].asString();
    }
};

struct UpdateMemberRoleResponseDto : ResponseDto {
    UpdateMemberRoleResponseDto() = default;

    Json::Value toJson() {
        Json::Value response_json;
        response_json["message"] = "Successfully changed role";
        return response_json;
    }
};

struct UpdateChatInfoRequestDto : RequestDto {
    int64_t user_id;
    int64_t chat_id;
    std::optional<std::string> name;
    std::optional<std::string> avatar;
    std::optional<std::string> description;

    UpdateChatInfoRequestDto(
        int64_t chat_id_,
        std::optional<std::string> name_,
        std::optional<std::string> avatar_,
        std::optional<std::string> description_
    )
        : chat_id(chat_id_),
          name(std::move(name_)),
          avatar(std::move(avatar_)),
          description(std::move(description_)) {
    }

    UpdateChatInfoRequestDto(
        drogon::HttpRequestPtr req,
        std::shared_ptr<Json::Value> request_json,
        int64_t chat_id_
    ) {
        chat_id = chat_id_;
        user_id = req->getAttributes()->get<int64_t>("user_id");
        name = request_json->isMember("name")
                   ? std::make_optional((*request_json)["name"].asString())
                   : std::nullopt;

        avatar = request_json->isMember("avatar")
                     ? std::make_optional((*request_json)["avatar"].asString())
                     : std::nullopt;

        description =
            request_json->isMember("description")
                ? std::make_optional((*request_json)["description"].asString())
                : std::nullopt;
    }
};

struct UpdateChatInfoResponseDto : ResponseDto {
    UpdateChatInfoResponseDto() = default;

    Json::Value toJson() {
        Json::Value response_json;
        response_json["message"] = "Successfully changed chat info";
        return response_json;
    }
};

}  // namespace messenger::dto
