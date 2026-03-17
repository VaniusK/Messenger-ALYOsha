#include "ChatController.h"
#include "services/ChatService.hpp"
#include "utils/controller_utils.hpp"
#include "utils/server_response_macro.hpp"

using namespace api::v1;

Task<HttpResponsePtr> ChatController::getUserChats(const HttpRequestPtr req, int64_t user_id){
    auto request_json = std::make_shared<Json::Value>();
    LOG_INFO << "Entered ChatController -> getUserChats";
    (*request_json)["user_id"] = req->getAttributes()->get<int64_t>("user_id");
    co_return co_await chat_service.getUserChats(request_json, user_id);
}

Task<HttpResponsePtr> ChatController::createOrGetDirectChat(const HttpRequestPtr req){
    LOG_INFO << "Entered ChatController -> createOrGetDirectChat";
    auto request_json = req->getJsonObject();
    (*request_json)["user_id"] = req->getAttributes()->get<int64_t>("user_id");
    Json::Value response_json;
    if (utils::find_missed_fields(
            response_json, request_json, {"target_user_id"}
        )) {
        RETURN_RESPONSE_CODE_400(response_json)
    }
    co_return co_await chat_service.createOrGetDirectChat(request_json);
}

Task<HttpResponsePtr> ChatController::getChatMessages(const HttpRequestPtr req, int64_t chat_id){
    LOG_INFO << "Entered ChatController -> getChatMessages";
    auto request_json = req->getJsonObject() ? req->getJsonObject() : std::make_shared<Json::Value>(); 
    (*request_json)["user_id"] = req->getAttributes()->get<int64_t>("user_id");
    co_return co_await chat_service.getChatMessages(request_json, chat_id);
}

Task<HttpResponsePtr> ChatController::sendMessage(const HttpRequestPtr req, int64_t chat_id){
    LOG_INFO << "Entered ChatController -> sendMessage";
    auto request_json = req->getJsonObject();
    (*request_json)["user_id"] = req->getAttributes()->get<int64_t>("user_id");
    Json::Value response_json;
    if (utils::find_missed_fields(
            response_json, request_json, {"text"}
        )) {
        RETURN_RESPONSE_CODE_400(response_json)
    }
    co_return co_await chat_service.sendMessage(request_json, chat_id);
}

Task<HttpResponsePtr> ChatController::readMessages(const HttpRequestPtr req, int64_t chat_id){
    LOG_INFO << "Entered ChatController -> readMessages";
    auto request_json = req->getJsonObject();
    (*request_json)["user_id"] = req->getAttributes()->get<int64_t>("user_id");
    Json::Value response_json;
    if (utils::find_missed_fields(
            response_json, request_json, {"last_read_message_id"}
        )) {
        RETURN_RESPONSE_CODE_400(response_json)
    }
    co_return co_await chat_service.readMessages(request_json, chat_id);
}

Task<HttpResponsePtr> ChatController::getMessageById(const HttpRequestPtr req, int64_t message_id){
    LOG_INFO << "Entered ChatController -> getMessageById";
    auto request_json = std::make_shared<Json::Value>();
    (*request_json)["user_id"] = req->getAttributes()->get<int64_t>("user_id");
    co_return co_await chat_service.getMessageById(request_json, message_id);
}
