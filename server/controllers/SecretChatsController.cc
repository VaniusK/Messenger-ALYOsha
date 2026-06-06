#include "controllers/SecretChatsController.h"
#include <drogon/HttpRequest.h>
#include <json/value.h>
#include "dto/SecretChatsServiceDtos.hpp"
#include "utils/controller_utils.hpp"
#include "utils/server_response_macro.hpp"
#include "utils/server_exceptions.hpp"


namespace api::v1 {

Task<HttpResponsePtr> SecretChatsController::chatInit(
    const HttpRequestPtr req
) {
    LOG_INFO << "Entered SecretChatsController -> chatInit";
    Json::Value response_json;
    auto request_json = req->getJsonObject();
    if (utils::find_missed_fields(
            response_json, request_json, {"target_user_id", "public_key", "chat_id"}
        )) {
        RETURN_RESPONSE_CODE_400(response_json)
    }

    messenger::dto::SecretChatInitRequestDto request_dto(req, request_json);

    try {
        auto secret_chats_service = drogon::app().getPlugin<api::v1::SecretChatsService>();
        auto response_dto = co_await secret_chats_service->chatInit(std::move(request_dto));
        response_json = response_dto.toJson();
        RETURN_RESPONSE_CODE_201(response_json)
    }
    catch (messenger::exceptions::BadRequestException &e) {
        response_json["message"] = e.what();
        RETURN_RESPONSE_CODE_400(response_json)
    }
    catch (messenger::exceptions::NotFoundException &e){
        response_json["message"] = e.what();
        RETURN_RESPONSE_CODE_404(response_json)
    }
    catch (messenger::exceptions::InternalServerErrorException &e) {
        response_json["message"] = e.what();
        RETURN_RESPONSE_CODE_500(response_json)
    }
    catch (std::exception &e) {
        response_json["message"] = std::string("Internal server error: ") + e.what();
        RETURN_RESPONSE_CODE_500(response_json)
    }
}

Task<HttpResponsePtr> SecretChatsController::chatAccept(
    const HttpRequestPtr req
) {
    LOG_INFO << "Entered SecretChatsController -> chatAccept";
    Json::Value response_json;
    auto request_json = req->getJsonObject();
    if (utils::find_missed_fields(
            response_json, request_json, {"target_user_id", "public_key", "chat_id"}
        )) {
        RETURN_RESPONSE_CODE_400(response_json)
    }

    messenger::dto::SecretChatAcceptRequestDto request_dto(req, request_json);

    try {
        auto secret_chats_service = drogon::app().getPlugin<api::v1::SecretChatsService>();
        auto response_dto = co_await secret_chats_service->chatAccept(std::move(request_dto));
        response_json = response_dto.toJson();
        RETURN_RESPONSE_CODE_201(response_json)
    }
    catch (messenger::exceptions::NotFoundException &e){
        response_json["message"] = e.what();
        RETURN_RESPONSE_CODE_404(response_json)
    }
    catch (messenger::exceptions::InternalServerErrorException &e) {
        response_json["message"] = e.what();
        RETURN_RESPONSE_CODE_500(response_json)
    }
    catch (std::exception &e) {
        response_json["message"] = std::string("Internal server error: ") + e.what();
        RETURN_RESPONSE_CODE_500(response_json)
    }
}

Task<HttpResponsePtr> SecretChatsController::deleteChat(const HttpRequestPtr req) {
    LOG_INFO << "Entered SecretChatsController -> chatAccept";
    Json::Value response_json;
    auto request_json = req->getJsonObject();
    if (utils::find_missed_fields(
            response_json, request_json, {"target_user_id", "chat_id"}
        )) {
        RETURN_RESPONSE_CODE_400(response_json)
    }

    messenger::dto::DeleteSecretChatRequestDto request_dto(req, request_json);
    try {
        auto secret_chats_service = drogon::app().getPlugin<api::v1::SecretChatsService>();
        auto response_dto = co_await secret_chats_service->deleteChat(std::move(request_dto));
        response_json = response_dto.toJson();
        RETURN_RESPONSE_CODE_201(response_json)
    }
    catch (messenger::exceptions::NotFoundException &e){
        response_json["message"] = e.what();
        RETURN_RESPONSE_CODE_404(response_json)
    }
    catch (messenger::exceptions::InternalServerErrorException &e) {
        response_json["message"] = e.what();
        RETURN_RESPONSE_CODE_500(response_json)
    }
    catch (std::exception &e) {
        response_json["message"] = std::string("Internal server error: ") + e.what();
        RETURN_RESPONSE_CODE_500(response_json)
    }
}

Task<HttpResponsePtr> SecretChatsController::sendMessage(
    const HttpRequestPtr req
) {
    LOG_INFO << "Entered SecretChatsController -> sendMessage";
    Json::Value response_json;
    auto request_json = req->getJsonObject();
    if (utils::find_missed_fields(
            response_json, request_json, {"target_user_id", "encrypted_payload"}
        )) {
        RETURN_RESPONSE_CODE_400(response_json)
    }

    messenger::dto::SendSecretMessageRequestDto request_dto(req, request_json);

    try {
        auto secret_chats_service = drogon::app().getPlugin<api::v1::SecretChatsService>();
        auto response_dto = co_await secret_chats_service->sendMessage(std::move(request_dto));
        response_json = response_dto.toJson();
        RETURN_RESPONSE_CODE_201(response_json)
    }
    catch (messenger::exceptions::NotFoundException &e){
        response_json["message"] = e.what();
        RETURN_RESPONSE_CODE_404(response_json)
    }
    catch (messenger::exceptions::InternalServerErrorException &e) {
        response_json["message"] = e.what();
        RETURN_RESPONSE_CODE_500(response_json)
    }
    catch (std::exception &e) {
        response_json["message"] = std::string("Internal server error: ") + e.what();
        RETURN_RESPONSE_CODE_500(response_json)
    }
}

Task<HttpResponsePtr> SecretChatsController::readMessage(const HttpRequestPtr req) {
    LOG_INFO << "Entered SecretChatsController -> readMessage";
    Json::Value response_json;
    auto request_json = req->getJsonObject();
    if (utils::find_missed_fields(
            response_json, request_json, {"target_user_id", "encrypted_payload"}
        )) {
        RETURN_RESPONSE_CODE_400(response_json)
    }

    messenger::dto::ReadSecretMessageRequestDto request_dto(req, request_json);

    try {
        auto secret_chats_service = drogon::app().getPlugin<api::v1::SecretChatsService>();
        auto response_dto = co_await secret_chats_service->readMessage(std::move(request_dto));
        response_json = response_dto.toJson();
        RETURN_RESPONSE_CODE_201(response_json)
    }
    catch (messenger::exceptions::NotFoundException &e){
        response_json["message"] = e.what();
        RETURN_RESPONSE_CODE_404(response_json)
    }
    catch (messenger::exceptions::InternalServerErrorException &e) {
        response_json["message"] = e.what();
        RETURN_RESPONSE_CODE_500(response_json)
    }
    catch (std::exception &e) {
        response_json["message"] = std::string("Internal server error: ") + e.what();
        RETURN_RESPONSE_CODE_500(response_json)
    }
}

Task<HttpResponsePtr> SecretChatsController::getAttachmentLink(const HttpRequestPtr req) {
    LOG_INFO << "Entered SecretChatsController -> getAttachmentLink";
    Json::Value response_json;
    if (utils::find_missed_queries(
            response_json, req, {"count"}
        )) {
        RETURN_RESPONSE_CODE_400(response_json)
    }
    int32_t count;
    try {
        count = std::stoi(req->getParameter("count"));
    }
    catch (...){
        response_json["message"] = "Failed to parse \"count\" from parameters";
        RETURN_RESPONSE_CODE_400(response_json)
    }
    if (count > 10 || count <= 0){
        response_json["message"] = "Count should be more than 0 and less or equal than 10";
        RETURN_RESPONSE_CODE_400(response_json)
    }

    messenger::dto::GetSecretUploadUrlsRequestDto request_dto(count);

    try {
        auto secret_chats_service = drogon::app().getPlugin<api::v1::SecretChatsService>();
        auto response_dto = co_await secret_chats_service->getAttachmentLinks(std::move(request_dto));
        response_json = response_dto.toJson();
        RETURN_RESPONSE_CODE_200(response_json);
    }
    catch (messenger::exceptions::InternalServerErrorException &e) {
        response_json["message"] = e.what();
        RETURN_RESPONSE_CODE_500(response_json)
    }
    catch (std::exception &e) {
        response_json["message"] = std::string("Internal server error: ") + e.what();
        RETURN_RESPONSE_CODE_500(response_json)
    }
}

Task<HttpResponsePtr> SecretChatsController::getDownloadAttachmentLink(const HttpRequestPtr req) {
    LOG_INFO << "Entered SecretChatsController -> getDownloadAttachmentLink";
    Json::Value response_json;
    auto request_json = req->getJsonObject();
    if (utils::find_missed_fields(
            response_json, request_json, {"attachment_keys"}
        )) {
        RETURN_RESPONSE_CODE_400(response_json)
    }
    if (!(*request_json)["attachment_keys"].isArray()){
        response_json["message"] = "attachment_keys should be array";
        RETURN_RESPONSE_CODE_400(response_json)
    }

    GetSecretDownloadUrlRequestDto request_dto(request_json);

    try {
        auto secret_chats_service = drogon::app().getPlugin<api::v1::SecretChatsService>();
        auto response_dto = co_await secret_chats_service->getDownloadAttachmentLinks(std::move(request_dto));
        response_json = response_dto.toJson();
        RETURN_RESPONSE_CODE_200(response_json)
    }
    catch (messenger::exceptions::InternalServerErrorException &e) {
        response_json["message"] = e.what();
        RETURN_RESPONSE_CODE_500(response_json)
    }
    catch (std::exception &e) {
        response_json["message"] = std::string("Internal server error: ") + e.what();
        RETURN_RESPONSE_CODE_500(response_json)
    }
}

}  // namespace api::v1
