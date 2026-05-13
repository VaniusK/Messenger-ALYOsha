#include "ChatController.h"
#include <drogon/HttpResponse.h>
#include <json/forwards.h>
#include <json/value.h>
#include "dto/ChatServiceDtos.hpp"
#include "services/ChatService.hpp"
#include "utils/Enum.hpp"
#include "utils/controller_utils.hpp"
#include "utils/server_exceptions.hpp"
#include "utils/server_response_macro.hpp"

using namespace api::v1;
using namespace messenger::dto;

Task<HttpResponsePtr>
ChatController::getUserChats(const HttpRequestPtr req, int64_t user_id) {
    LOG_INFO << "Entered ChatController -> getUserChats";
    GetUserChatsRequestDto request_dto(req, user_id);
    Json::Value response_json;
    try {
        GetUserChatsResponseDto response_dto =
            co_await chat_service.getUserChats(request_dto);
        response_json = response_dto.toJson();
        RETURN_RESPONSE_CODE_200(response_json)
    } catch (const messenger::exceptions::ForbiddenException &e) {
        response_json["message"] = e.what();
        RETURN_RESPONSE_CODE_403(response_json)
    } catch (const messenger::exceptions::InternalServerErrorException &e) {
        response_json["messsage"] = e.what();
        RETURN_RESPONSE_CODE_500(response_json)
    } catch (const std::exception &e) {
        response_json["message"] =
            std::string("Internal server error: ") + e.what();
        RETURN_RESPONSE_CODE_500(response_json)
    }
}

Task<HttpResponsePtr> ChatController::createOrGetDirectChat(
    const HttpRequestPtr req
) {
    LOG_INFO << "Entered ChatController -> createOrGetDirectChat";
    auto request_json = req->getJsonObject();
    Json::Value response_json;
    if (utils::find_missed_fields(
            response_json, request_json, {"target_user_id"}
        )) {
        RETURN_RESPONSE_CODE_400(response_json)
    }
    CreateOrGetDirectRequestDto request_dto(req, request_json);
    try {
        CreateOrGetDirectResponseDto response_dto =
            co_await chat_service.createOrGetDirectChat(request_dto);
        response_json = response_dto.toJson();
        if (response_dto.was_created) {
            RETURN_RESPONSE_CODE_201(response_json)
        } else {
            RETURN_RESPONSE_CODE_200(response_json)
        }
    } catch (const messenger::exceptions::InternalServerErrorException &e) {
        response_json["messsage"] = e.what();
        RETURN_RESPONSE_CODE_500(response_json)
    } catch (const std::exception &e) {
        response_json["message"] =
            std::string("Internal server error: ") + e.what();
        RETURN_RESPONSE_CODE_500(response_json)
    }
}

Task<HttpResponsePtr>
ChatController::getChatMessages(const HttpRequestPtr req, int64_t chat_id) {
    LOG_INFO << "Entered ChatController -> getChatMessages";
    GetChatMessagesRequestDto request_dto(req, chat_id);
    Json::Value response_json;
    try {
        GetChatMessagesResponseDto response_dto =
            co_await chat_service.getChatMessages(request_dto);
        response_json = response_dto.toJson();
        RETURN_RESPONSE_CODE_200(response_json)
    } catch (const messenger::exceptions::ForbiddenException &e) {
        response_json["message"] = e.what();
        RETURN_RESPONSE_CODE_403(response_json)
    } catch (const messenger::exceptions::InternalServerErrorException &e) {
        response_json["messsage"] = e.what();
        RETURN_RESPONSE_CODE_500(response_json)
    } catch (const std::exception &e) {
        response_json["message"] =
            std::string("Internal server error: ") + e.what();
        RETURN_RESPONSE_CODE_500(response_json)
    }
}

bool ChatController::validateMessageType(const std::string &message_type) {
    if (message_type == messenger::models::MessageType::Text) {
        return true;
    }
    if (message_type == messenger::models::MessageType::Voice) {
        return true;
    }
    if (message_type == messenger::models::MessageType::Round) {
        return true;
    }
    if (message_type == messenger::models::MessageType::Sticker) {
        return true;
    }
    if (message_type == messenger::models::MessageType::Media) {
        return true;
    }
    return false;
}

Task<HttpResponsePtr>
ChatController::sendMessage(const HttpRequestPtr req, int64_t chat_id) {
    LOG_INFO << "Entered ChatController -> sendMessage";
    auto request_json = req->getJsonObject();
    Json::Value response_json;
    if (utils::find_missed_fields(
            response_json, request_json, {"text", "type"}
        )) {
        RETURN_RESPONSE_CODE_400(response_json)
    }
    if (request_json->isMember("attachment_tokens") &&
        !(*request_json)["attachment_tokens"].isArray()) {
        response_json["message"] = "attachment_tokens is not array";
        RETURN_RESPONSE_CODE_400(response_json)
    }
    SendMessageRequestDto request_dto(req, request_json, chat_id);
    if (!validateMessageType(request_dto.message_type)) {
        response_json["message"] = "Invalid message type";
        RETURN_RESPONSE_CODE_400(response_json);
    }

    try {
        SendMessageResponseDto response_dto =
            co_await chat_service.sendMessage(request_dto);
        response_json = response_dto.toJson();
        RETURN_RESPONSE_CODE_201(response_json)
    } catch (const messenger::exceptions::BadRequestException &e) {
        response_json["message"] = e.what();
        RETURN_RESPONSE_CODE_400(response_json)
    } catch (const messenger::exceptions::ForbiddenException &e) {
        response_json["message"] = e.what();
        RETURN_RESPONSE_CODE_403(response_json)
    } catch (const messenger::exceptions::InternalServerErrorException &e) {
        response_json["messsage"] = e.what();
        RETURN_RESPONSE_CODE_500(response_json)
    } catch (const std::exception &e) {
        response_json["message"] =
            std::string("Internal server error: ") + e.what();
        RETURN_RESPONSE_CODE_500(response_json)
    }
}

Task<HttpResponsePtr>
ChatController::readMessages(const HttpRequestPtr req, int64_t chat_id) {
    LOG_INFO << "Entered ChatController -> readMessages";
    auto request_json = req->getJsonObject();
    Json::Value response_json;
    if (utils::find_missed_fields(
            response_json, request_json, {"last_read_message_id"}
        )) {
        RETURN_RESPONSE_CODE_400(response_json)
    }
    ReadMessagesRequestDto request_dto(req, request_json, chat_id);
    try {
        auto response_dto = co_await chat_service.readMessages(request_dto);
        response_json["message"] = response_dto.toJson();
        if (response_dto.success) {
            RETURN_RESPONSE_CODE_200(response_json)
        } else {
            RETURN_RESPONSE_CODE_500(response_json)
        }
    } catch (const messenger::exceptions::InternalServerErrorException &e) {
        response_json["messsage"] = e.what();
        RETURN_RESPONSE_CODE_500(response_json)
    } catch (const std::exception &e) {
        response_json["message"] =
            std::string("Internal server error: ") + e.what();
        RETURN_RESPONSE_CODE_500(response_json)
    }
}

Task<HttpResponsePtr>
ChatController::getMessageById(const HttpRequestPtr req, int64_t message_id) {
    LOG_INFO << "Entered ChatController -> getMessageById";
    GetMessageByIdRequestDto request_dto(req, message_id);
    Json::Value response_json;
    try {
        auto response_dto = co_await chat_service.getMessageById(request_dto);
        response_json = response_dto.toJson();
        RETURN_RESPONSE_CODE_200(response_json)
    } catch (const messenger::exceptions::NotFoundException &e) {
        response_json["message"] = e.what();
        RETURN_RESPONSE_CODE_404(response_json)
    } catch (const messenger::exceptions::ForbiddenException &e) {
        response_json["message"] = e.what();
        RETURN_RESPONSE_CODE_403(response_json)
    } catch (const messenger::exceptions::InternalServerErrorException &e) {
        response_json["messsage"] = e.what();
        RETURN_RESPONSE_CODE_500(response_json)
    } catch (const std::exception &e) {
        response_json["message"] =
            std::string("Internal server error: ") + e.what();
        RETURN_RESPONSE_CODE_500(response_json)
    }
}

Task<HttpResponsePtr> ChatController::getAttachmentLinks(
    const HttpRequestPtr req
) {
    LOG_INFO << "Entered ChatController -> getAttachmentLink";
    auto request_json = req->getJsonObject();
    Json::Value response_json;
    if (utils::find_missed_fields(
            response_json, request_json, {"chat_id", "files", "message_type"}
        )) {
        RETURN_RESPONSE_CODE_400(response_json)
    }
    Json::Value files_list = (*request_json)["files"];
    if (files_list.size() != 1 && (*request_json)["message_type"] ==
                                      messenger::models::MessageType::Voice) {
        response_json["message"] =
            "Mismatch message type and attachments count";
        RETURN_RESPONSE_CODE_400(response_json)
    }
    if (files_list.size() > 10) {
        response_json["message"] = "Too many attachments";
        RETURN_RESPONSE_CODE_400(response_json)
    }
    for (Json::ArrayIndex i = 0; i < files_list.size(); i++) {
        if (utils::find_missed_fields(
                response_json, std::make_shared<Json::Value>(files_list[i]),
                {"original_filename", "file_size_bytes"}
            )) {
            RETURN_RESPONSE_CODE_400(response_json)
        }
    }

    GetAttachmentLinksRequestDto request_dto(req, request_json);

    if (!validateMessageType(request_dto.message_type)) {
        response_json["message"] = "Invalid message type";
        RETURN_RESPONSE_CODE_400(response_json)
    }

    try {
        auto response_dto =
            co_await chat_service.getAttachmentLinks(request_dto);
        response_json = response_dto.toJson();
        RETURN_RESPONSE_CODE_200(response_json)
    } catch (const messenger::exceptions::BadRequestException &e) {
        response_json["message"] = e.what();
        RETURN_RESPONSE_CODE_400(response_json)
    } catch (const messenger::exceptions::ForbiddenException &e) {
        response_json["message"] = e.what();
        RETURN_RESPONSE_CODE_403(response_json)
    } catch (const messenger::exceptions::InternalServerErrorException &e) {
        response_json["messsage"] = e.what();
        RETURN_RESPONSE_CODE_500(response_json)
    } catch (const std::exception &e) {
        response_json["message"] =
            std::string("Internal server error: ") + e.what();
        RETURN_RESPONSE_CODE_500(response_json)
    }
}

Task<HttpResponsePtr> ChatController::createGroup(const HttpRequestPtr req) {
    LOG_INFO << "Entered ChatController -> createGroup";
    auto request_json = req->getJsonObject();
    Json::Value response_json;
    if (utils::find_missed_fields(response_json, request_json, {"chat_name"})) {
        RETURN_RESPONSE_CODE_400(response_json)
    }
    if ((*request_json)["chat_name"].asString() == "") {
        response_json["message"] = "Cannot create chat with empty name";
        RETURN_RESPONSE_CODE_400(response_json)
    }
    if (request_json->isMember("members")) {
        if (!(*request_json)["members"].isArray()) {
            response_json["message"] = "Members is not array";
            RETURN_RESPONSE_CODE_400(response_json)
        }
        if ((*request_json)["members"].size() > 49) {
            response_json["message"] =
                "Cannot create chat with more than 50 members";
            RETURN_RESPONSE_CODE_400(response_json)
        }
        for (const auto &el : (*request_json)["members"]) {
            if (!el.isInt64()) {
                response_json["message"] = el.asString() + " is not number";
                RETURN_RESPONSE_CODE_400(response_json)
            }
        }
    }
    CreateGroupRequestDto request_dto(req, request_json);

    try {
        CreateGroupResponseDto response_dto = co_await chat_service.createGroup(request_dto);
        response_json = response_dto.toJson();
        RETURN_RESPONSE_CODE_201(response_json)
    } catch (const messenger::exceptions::BadRequestException &e) {
        response_json["message"] = e.what();
        RETURN_RESPONSE_CODE_400(response_json)
    } catch (const messenger::exceptions::NotFoundException &e) {
        response_json["message"] = e.what();
        RETURN_RESPONSE_CODE_404(response_json)
    } catch (const messenger::exceptions::InternalServerErrorException &e) {
        response_json["messsage"] = e.what();
        RETURN_RESPONSE_CODE_500(response_json)
    } catch (const std::exception &e) {
        response_json["message"] =
            std::string("Internal server error: ") + e.what();
        RETURN_RESPONSE_CODE_500(response_json)
    }
}

std::set<std::string> ChatController::chat_roles = {
    messenger::models::ChatRole::Member, messenger::models::ChatRole::Admin,
    messenger::models::ChatRole::Owner
};

Task<HttpResponsePtr>
ChatController::addGroupChatMember(const HttpRequestPtr req, int64_t chat_id) {
    LOG_INFO << "Entered ChatController -> addGroupChatMember";
    auto request_json = req->getJsonObject();
    Json::Value response_json;
    if (utils::find_missed_fields(
            response_json, request_json, {"user_id", "role"}
        )) {
        RETURN_RESPONSE_CODE_400(response_json)
    }
    if (!(*request_json)["user_id"].isInt64()) {
        response_json["message"] = "user_id is not number";
        RETURN_RESPONSE_CODE_400(response_json)
    }
    if (chat_roles.find((*request_json)["role"].asString()) ==
        chat_roles.end()) {
        response_json["message"] = "This role doesn't exist";
        RETURN_RESPONSE_CODE_400(response_json)
    }
    AddGroupChatMemberRequestDto request_dto(req, chat_id);

    try {
        AddGroupChatMemberResponseDto response_dto = co_await chat_service.addGroupChatMember(request_dto);
        response_json = response_dto.toJson();
        RETURN_RESPONSE_CODE_201(response_json)
    } catch (const messenger::exceptions::ForbiddenException &e) {
        response_json["message"] = e.what();
        RETURN_RESPONSE_CODE_403(response_json)
    } catch (const messenger::exceptions::BadRequestException &e) {
        response_json["message"] = e.what();
        RETURN_RESPONSE_CODE_400(response_json)
    } catch (const messenger::exceptions::NotFoundException &e) {
        response_json["message"] = e.what();
        RETURN_RESPONSE_CODE_404(response_json)
    } catch (const messenger::exceptions::ConflictException &e) {
        response_json["message"] = e.what();
        RETURN_RESPONSE_CODE_409(response_json)
    } catch (const messenger::exceptions::InternalServerErrorException &e) {
        response_json["messsage"] = e.what();
        RETURN_RESPONSE_CODE_500(response_json)
    } catch (const std::exception &e) {
        response_json["message"] =
            std::string("Internal server error: ") + e.what();
        RETURN_RESPONSE_CODE_500(response_json)
    }
}

Task<HttpResponsePtr> ChatController::getChatMember(
    const HttpRequestPtr req,
    int64_t chat_id,
    int64_t member_id
) {
    LOG_INFO << "Entered ChatController -> getChatMember";
    GetChatMemberRequestDto request_dto(req, chat_id, member_id);

    Json::Value response_json;
    try {
        GetChatMemberResponseDto response_dto = co_await chat_service.getChatMember(request_dto);
        response_json = response_dto.toJson();
        RETURN_RESPONSE_CODE_200(response_json)
    } catch (const messenger::exceptions::ForbiddenException &e) {
        response_json["message"] = e.what();
        RETURN_RESPONSE_CODE_403(response_json)
    } catch (const messenger::exceptions::NotFoundException &e) {
        response_json["message"] = e.what();
        RETURN_RESPONSE_CODE_404(response_json)
    } catch (const messenger::exceptions::InternalServerErrorException &e) {
        response_json["messsage"] = e.what();
        RETURN_RESPONSE_CODE_500(response_json)
    } catch (const std::exception &e) {
        response_json["message"] =
            std::string("Internal server error: ") + e.what();
        RETURN_RESPONSE_CODE_500(response_json)
    }
}

Task<HttpResponsePtr>
ChatController::getChatMembers(const HttpRequestPtr req, int64_t chat_id) {
    LOG_INFO << "Entered ChatController -> getChatMembers";
    GetChatMembersRequestDto request_dto(req, chat_id);

    Json::Value response_json;
    try {
        GetChatMembersResponseDto response_dto = co_await chat_service.getChatMembers(request_dto);
        response_json = response_dto.toJson();
        RETURN_RESPONSE_CODE_200(response_json)
    } catch (const messenger::exceptions::ForbiddenException &e) {
        response_json["message"] = e.what();
        RETURN_RESPONSE_CODE_403(response_json)
    } catch (const messenger::exceptions::NotFoundException &e) {
        response_json["message"] = e.what();
        RETURN_RESPONSE_CODE_404(response_json)
    } catch (const messenger::exceptions::InternalServerErrorException &e) {
        response_json["messsage"] = e.what();
        RETURN_RESPONSE_CODE_500(response_json)
    } catch (const std::exception &e) {
        response_json["message"] =
            std::string("Internal server error: ") + e.what();
        RETURN_RESPONSE_CODE_500(response_json)
    }
}

Task<HttpResponsePtr> ChatController::removeMember(
    const HttpRequestPtr req,
    int64_t chat_id,
    int64_t member_id
) {
    LOG_INFO << "Entered ChatController -> removeMember";
    RemoveMemberRequestDto request_dto(req, chat_id, member_id);

    Json::Value response_json;
    try {
        RemoveMemberResponseDto response_dto = co_await chat_service.removeMember(request_dto);
        response_json = response_dto.toJson();
        RETURN_RESPONSE_CODE_200(response_json)
    } catch (const messenger::exceptions::ForbiddenException &e) {
        response_json["message"] = e.what();
        RETURN_RESPONSE_CODE_403(response_json)
    } catch (const messenger::exceptions::ConflictException &e) {
        response_json["message"] = e.what();
        RETURN_RESPONSE_CODE_409(response_json)
    } catch (const messenger::exceptions::InternalServerErrorException &e) {
        response_json["messsage"] = e.what();
        RETURN_RESPONSE_CODE_500(response_json)
    } catch (const std::exception &e) {
        response_json["message"] =
            std::string("Internal server error: ") + e.what();
        RETURN_RESPONSE_CODE_500(response_json)
    }
}

Task<HttpResponsePtr> ChatController::updateMemberRole(
    const HttpRequestPtr req,
    int64_t chat_id,
    int64_t member_id
) {
    LOG_INFO << "Entered ChatController -> updateMemberRole";
    auto request_json = req->getJsonObject();
    Json::Value response_json;
    if (utils::find_missed_fields(response_json, request_json, {"role"})) {
        RETURN_RESPONSE_CODE_400(response_json)
    }
    if (chat_roles.find((*request_json)["role"].asString()) ==
        chat_roles.end()) {
        response_json["message"] = "This role doesn't exist";
        RETURN_RESPONSE_CODE_400(response_json)
    }
    UpdateMemberRoleRequestDto request_dto(
        req, request_json, chat_id, member_id
    );

    try {
        UpdateMemberRoleResponseDto response_dto = co_await chat_service.updateMemberRole(request_dto);
        response_json = response_dto.toJson();
        RETURN_RESPONSE_CODE_200(response_json)
    } catch (const messenger::exceptions::ForbiddenException &e) {
        response_json["message"] = e.what();
        RETURN_RESPONSE_CODE_403(response_json)
    } catch (const messenger::exceptions::ConflictException &e) {
        response_json["message"] = e.what();
        RETURN_RESPONSE_CODE_409(response_json)
    } catch (const messenger::exceptions::InternalServerErrorException &e) {
        response_json["messsage"] = e.what();
        RETURN_RESPONSE_CODE_500(response_json)
    } catch (const std::exception &e) {
        response_json["message"] =
            std::string("Internal server error: ") + e.what();
        RETURN_RESPONSE_CODE_500(response_json)
    }
}

Task<HttpResponsePtr>
ChatController::updateChatInfo(const HttpRequestPtr req, int64_t chat_id) {
    LOG_INFO << "Entered ChatController -> updateChatInfo";

    auto request_json = req->getJsonObject();
    Json::Value response_json;
    if (request_json->isMember("name") &&
        (*request_json)["name"].asString() == "") {
        response_json["message"] = "Cannot set empty name for the chat";
        RETURN_RESPONSE_CODE_400(response_json)
    }

    UpdateChatInfoRequestDto request_dto(req, request_json, chat_id);

    try {
        UpdateChatInfoResponseDto response_dto = co_await chat_service.updateChatInfo(request_dto);
        response_json = response_dto.toJson();
        RETURN_RESPONSE_CODE_200(response_json)
    } catch (const messenger::exceptions::ForbiddenException &e) {
        response_json["message"] = e.what();
        RETURN_RESPONSE_CODE_403(response_json)
    } catch (const messenger::exceptions::InternalServerErrorException &e) {
        response_json["messsage"] = e.what();
        RETURN_RESPONSE_CODE_500(response_json)
    } catch (const std::exception &e) {
        response_json["message"] =
            std::string("Internal server error: ") + e.what();
        RETURN_RESPONSE_CODE_500(response_json)
    }
}