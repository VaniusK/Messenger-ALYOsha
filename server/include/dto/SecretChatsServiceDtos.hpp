#pragma once
#include <drogon/drogon.h>
#include <jsoncpp/json/value.h>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include "ServicesDtoBase.hpp"

namespace messenger::dto {
struct SecretChatInitRequestDto : RequestDto {
    int64_t source_user_id;
    int64_t target_user_id;
    std::string initialUserPublicKey;

    SecretChatInitRequestDto(
        int64_t source_user_id_,
        int64_t target_user_id_,
        std::string initialUserPublicKey_
    )
        : source_user_id(source_user_id_),
          target_user_id(target_user_id_),
          initialUserPublicKey(std::move(initialUserPublicKey_)) {
    }

    SecretChatInitRequestDto(
        drogon::HttpRequestPtr req,
        std::shared_ptr<Json::Value> request_json
    ) {
        source_user_id = req->getAttributes()->get<int64_t>("user_id");
        target_user_id = (*request_json)["target_user_id"].asInt64();
        initialUserPublicKey = (*request_json)["public_key"].asString();
    }
};

struct SecretChatInitResponseDto : ResponseDto {
    SecretChatInitResponseDto() = default;

    Json::Value toJson() {
        Json::Value response_json;
        response_json["message"] =
            "Secret chat initialization request successfully sent to target "
            "user";
        return response_json;
    }
};

struct SecretChatAcceptRequestDto : RequestDto {
    int64_t source_user_id;
    int64_t target_user_id;
    std::string acceptorUserPublicKey;

    SecretChatAcceptRequestDto(
        int64_t source_user_id_,
        int64_t target_user_id_,
        std::string acceptorUserPublicKey_
    )
        : source_user_id(source_user_id_),
          target_user_id(target_user_id_),
          acceptorUserPublicKey(std::move(acceptorUserPublicKey_)) {
    }

    SecretChatAcceptRequestDto(
        drogon::HttpRequestPtr req,
        std::shared_ptr<Json::Value> request_json
    ) {
        source_user_id = req->getAttributes()->get<int64_t>("user_id");
        target_user_id = (*request_json)["target_user_id"].asInt64();
        acceptorUserPublicKey = (*request_json)["public_key"].asString();
    }
};

struct SecretChatAcceptResponseDto : ResponseDto {
    SecretChatAcceptResponseDto() = default;

    Json::Value toJson() {
        Json::Value response_json;
        response_json["message"] =
            "Secret chat initialization request successfully accepted";
        return response_json;
    }
};

struct SendSecretMessageRequestDto : RequestDto {
    int64_t source_user_id;
    int64_t target_user_id;
    Json::Value encrypted_payload;

    SendSecretMessageRequestDto(
        int64_t source_user_id_,
        int64_t target_user_id_,
        Json::Value encrypted_payload_
    )
        : source_user_id(source_user_id_),
          target_user_id(target_user_id_),
          encrypted_payload(std::move(encrypted_payload_)) {
    }

    SendSecretMessageRequestDto(
        drogon::HttpRequestPtr req,
        std::shared_ptr<Json::Value> request_json
    ) {
        source_user_id = req->getAttributes()->get<int64_t>("user_id");
        target_user_id = (*request_json)["target_user_id"].asInt64();
        encrypted_payload = (*request_json)["encrypted_payload"];
    }
};

struct SendSecretMessageResponseDto : ResponseDto {
    SendSecretMessageResponseDto() = default;

    Json::Value toJson() {
        Json::Value response_json;
        response_json["message"] = "Message sent";
        return response_json;
    }
};

struct ReadSecretMessageRequestDto : RequestDto {
    int64_t source_user_id;
    int64_t target_user_id;
    Json::Value encrypted_payload;

    ReadSecretMessageRequestDto(
        int64_t source_user_id_,
        int64_t target_user_id_,
        Json::Value encrypted_payload_
    )
        : source_user_id(source_user_id_),
          target_user_id(target_user_id_),
          encrypted_payload(std::move(encrypted_payload_)) {
    }

    ReadSecretMessageRequestDto(
        drogon::HttpRequestPtr req,
        std::shared_ptr<Json::Value> request_json
    ) {
        source_user_id = req->getAttributes()->get<int64_t>("user_id");
        target_user_id = (*request_json)["target_user_id"].asInt64();
        encrypted_payload = (*request_json)["encrypted_payload"];
    }
};

struct ReadSecretMessageResponseDto : ResponseDto {
    ReadSecretMessageResponseDto() = default;

    Json::Value toJson() {
        Json::Value response_json;
        response_json["message"] = "Message read";
        return response_json;
    }
};

struct GetSecretUploadUrlsRequestDto : RequestDto {
    int32_t count;

    GetSecretUploadUrlsRequestDto(int32_t count_) : count(count_) {
    }
};

struct SecretUploadInfo {
    std::string s3_key;
    std::string upload_url;
};

struct GetSecretUploadUrlsResponseDto : ResponseDto {
    std::vector<SecretUploadInfo> attachments_info;
    GetSecretUploadUrlsResponseDto() = default;

    GetSecretUploadUrlsResponseDto(
        std::vector<SecretUploadInfo> attachments_info_
    )
        : attachments_info(std::move(attachments_info_)) {
    }

    Json::Value toJson() {
        Json::Value response_json(Json::arrayValue);
        for (const auto &el : attachments_info) {
            Json::Value attachment_info;
            attachment_info["s3_key"] = el.s3_key.c_str();
            attachment_info["upload_url"] = el.upload_url.c_str();
            response_json.append(attachment_info);
        }

        return response_json;
    }
};

struct GetSecretDownloadUrlRequestDto : RequestDto {
    std::vector<std::string> attachment_keys;

    GetSecretDownloadUrlRequestDto(std::vector<std::string> attachment_keys_)
        : attachment_keys(std::move(attachment_keys_)) {
    }

    GetSecretDownloadUrlRequestDto(std::shared_ptr<Json::Value> request_json) {
        for (const auto &el : (*request_json)["attachment_keys"]) {
            attachment_keys.push_back(el.asString());
        }
    }
};

struct GetSecretDownloadUrlResponseDto : ResponseDto {
    std::vector<std::string> urls;

    GetSecretDownloadUrlResponseDto() = default;

    GetSecretDownloadUrlResponseDto(std::vector<std::string> urls_)
        : urls(std::move(urls_)) {
    }

    Json::Value toJson() {
        Json::Value response_json(Json::arrayValue);
        for (const auto &url : urls) {
            response_json.append(url);
        }
        return response_json;
    }
};

}  // namespace messenger::dto
