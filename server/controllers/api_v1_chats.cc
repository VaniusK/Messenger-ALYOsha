#include "api_v1_chats.h"
#include "services/ChatService.hpp"
#include "utils/controller_utils.hpp"
#include "utils/server_response_macro.hpp"

using namespace api::v1;

Task<HttpResponsePtr> chats::getUserChats(const HttpRequestPtr req, int64_t user_id){
    auto request_json = std::make_shared<Json::Value>();
    (*request_json)["user_id"] = req->getAttributes()->get<int64_t>("user_id");
    co_return co_await ChatService::getUserChats(request_json, user_id, chat_repo);
}

Task<HttpResponsePtr> chats::createOrGetDirectChat(const HttpRequestPtr req){
    auto request_json = req->getJsonObject();
    (*request_json)["user_id"] = req->getAttributes()->get<int64_t>("user_id");
    Json::Value response_json;
    if (utils::find_missed_fields(
            response_json, request_json, {"target_user_id"}
        )) {
        RETURN_RESPONSE_CODE_400(response_json)
    }
    co_return co_await ChatService::createOrGetDirectChat(request_json, chat_repo);
}

Task<HttpResponsePtr> chats::getChatMessages(const HttpRequestPtr req, int64_t chat_id){
    auto request_json = req->getJsonObject() ? req->getJsonObject() : std::make_shared<Json::Value>();
    (*request_json)["user_id"] = req->getAttributes()->get<int64_t>("user_id");
    co_return co_await ChatService::getChatMessages(request_json, chat_id, chat_repo);
}

Task<HttpResponsePtr> chats::sendMessage(const HttpRequestPtr req, int64_t chat_id){
    auto request_json = req->getJsonObject();
    (*request_json)["user_id"] = req->getAttributes()->get<int64_t>("user_id");
    Json::Value response_json;
    if (utils::find_missed_fields(
            response_json, request_json, {"text"}
        )) {
        RETURN_RESPONSE_CODE_400(response_json)
    }
    co_return co_await ChatService::sendMessage(request_json, chat_id, chat_repo);
}

Task<HttpResponsePtr> chats::readMessages(const HttpRequestPtr req, int64_t chat_id){
    // ChatRepository repo;
    // some logic with co_await
    Json::Value ret;
    ret["status"] = chat_id;
    auto resp = HttpResponse::newHttpJsonResponse(ret);
    co_return resp;
}

Task<HttpResponsePtr> chats::getMessageById(const HttpRequestPtr req, int64_t message_id){
    auto request_json = std::make_shared<Json::Value>();
    (*request_json)["user_id"] = req->getAttributes()->get<int64_t>("user_id");
    co_return co_await ChatService::getMessageById(request_json, message_id, chat_repo);
}
