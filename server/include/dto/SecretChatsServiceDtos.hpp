#include <drogon/HttpRequest.h>
#include <jsoncpp/json/value.h>
#include <cstdint>
#include <string>
#include "ChatServiceDtos.hpp"
#include "ServicesDtoBase.hpp"

namespace messenger::dto {
struct ChatInitRequestDto : RequestDto {
    int64_t source_user_id;
    int64_t target_user_id;
    std::string initialUserPublicKey;

    ChatInitRequestDto(
        int64_t source_user_id_,
        int64_t target_user_id_,
        std::string initialUserPublicKey_
    )
        : source_user_id(source_user_id_),
          target_user_id(target_user_id_),
          initialUserPublicKey(std::move(initialUserPublicKey_)) {
    }

    ChatInitRequestDto(
        drogon::HttpRequestPtr req,
        std::shared_ptr<Json::Value> request_json
    ) {
        source_user_id = req->getAttributes()->get<int64_t>("user_id");
        target_user_id = (*request_json)["target_user_id"].asInt64();
        initialUserPublicKey = (*request_json)["public_key"].asString();
    }
};

struct ChatInitResponseDto : ResponseDto {
    ChatInitResponseDto() = default;

    Json::Value toJson() {
        Json::Value response_json;
        response_json["message"] =
            "Secret chat initialization request successfully sent to target "
            "user";
        return response_json;
    }
};

struct ChatAcceptRequestDto : RequestDto {
    int64_t source_user_id;
    int64_t target_user_id;
    std::string acceptorUserPublicKey;

    ChatAcceptRequestDto(
        int64_t source_user_id_,
        int64_t target_user_id_,
        std::string acceptorUserPublicKey_
    )
        : source_user_id(source_user_id_),
          target_user_id(target_user_id_),
          acceptorUserPublicKey(std::move(acceptorUserPublicKey_)) {
    }

    ChatAcceptRequestDto(
        drogon::HttpRequestPtr req,
        std::shared_ptr<Json::Value> request_json
    ) {
        source_user_id = req->getAttributes()->get<int64_t>("user_id");
        target_user_id = (*request_json)["target_user_id"].asInt64();
        acceptorUserPublicKey = (*request_json)["public_key"].asString();
    }
};

struct ChatAcceptResponseDto : ResponseDto {
    ChatAcceptResponseDto() = default;

    Json::Value toJson() {
        Json::Value response_json;
        response_json["message"] =
            "Secret chat initialization request successfully accepted";
        return response_json;
    }
};

}  // namespace messenger::dto
