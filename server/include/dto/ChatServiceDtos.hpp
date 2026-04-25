#pragma once
#include <drogon/HttpRequest.h>
#include <json/value.h>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>
#include "dto/ServicesDtoBase.hpp"
#include "models/Users.h"

using User = drogon_model::messenger_db::Users;

namespace messenger::dto {

struct GetUserChatsRequestDto : RequestDto {
    int64_t from_token_user_id;
    int64_t from_request_user_id;

    GetUserChatsRequestDto(
        drogon::HttpRequestPtr req,
        int64_t from_request_user_id_
    ) {
        from_token_user_id = req->getAttributes()->get<int64_t>("user_id");
        from_request_user_id = from_request_user_id_;
    }
};

struct GetUserChatsResponseDto : ResponseDto {};

struct CreateOrGetDirectRequestDto : RequestDto {
    int64_t user_id;
    int64_t target_user_id;

    CreateOrGetDirectRequestDto(drogon::HttpRequestPtr req) {
        auto request_json = req->getJsonObject();
        user_id = req->getAttributes()->get<int64_t>("user_id");
        target_user_id = (*request_json)["target_user_id"].asInt64();
    }
};

struct CreateOrGetDirectResponseDto : ResponseDto {};

struct GetChatMessagesRequestDto : RequestDto {
    int64_t chat_id;
    int64_t user_id;

    GetChatMessagesRequestDto(drogon::HttpRequestPtr req, int64_t chat_id_) {
        user_id = req->getAttributes()->get<int64_t>("user_id");
        chat_id = chat_id_;
    }
};

struct GetChatMessagesResponseDto : ResponseDto {};

struct SendMessageRequestDto : RequestDto {
    int64_t user_id;
    int64_t chat_id;
    std::string text;
    std::string message_type;
    std::optional<int64_t> reply_to_id;
    std::optional<int64_t> forward_from_id;
    std::vector<std::string> attachment_tokens;

    SendMessageRequestDto(drogon::HttpRequestPtr req, int64_t chat_id_) {
        user_id = req->getAttributes()->get<int64_t>("user_id");
        chat_id = chat_id_;
        auto request_json = req->getJsonObject();
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

struct SendMessageResponseDto : ResponseDto {};

struct ReadMessagesRequestDto : RequestDto {
    int64_t user_id;
    int64_t chat_id;
    int64_t last_read_message_id;

    ReadMessagesRequestDto(drogon::HttpRequestPtr req, int64_t chat_id_) {
        user_id = req->getAttributes()->get<int64_t>("user_id");
        chat_id = chat_id_;
        auto request_json = req->getJsonObject();
        last_read_message_id =
            (*request_json)["last_read_message_id"].asInt64();
    }
};

struct ReadMessagesResponseDto : ResponseDto {};

struct GetMessageByIdRequestDto : RequestDto {
    int64_t message_id;
    int64_t user_id;

    GetMessageByIdRequestDto(drogon::HttpRequestPtr req, int64_t message_id_) {
        user_id = req->getAttributes()->get<int64_t>("user_id");
        message_id = message_id_;
    }
};

struct GetMessageByIdResponseDto : ResponseDto {};

struct AttachmentRequestDto : RequestDto {
    std::string original_filename;
    int64_t file_size_bytes;

    AttachmentRequestDto(
        std::string &&original_filename_,
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

    GetAttachmentLinksRequestDto(drogon::HttpRequestPtr req) {
        user_id = req->getAttributes()->get<int64_t>("user_id");
        auto request_json = req->getJsonObject();
        chat_id = (*request_json)["chat_id"].asInt64();
        message_type = (*request_json)["message_type"].asString();
        for (auto file : (*request_json)["files"]) {
            files.push_back(
                {file["original_filename"].asString(),
                 file["file_size_bytes"].asInt64()}
            );
        }
    }
};

struct GetAttachmentLinksResponseDto : ResponseDto {};

}  // namespace messenger::dto
