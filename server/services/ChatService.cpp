#include "services/ChatService.hpp"
#include <drogon/HttpController.h>
#include <json/value.h>
#include <cstdint>
#include "controllers/ServerWebSocketController.h"
#include "include/repositories/ChatRepository.hpp"
#include "models/Messages.h"
#include "utils/server_response_macro.hpp"

using namespace drogon;
using namespace api::v1;

using ChatRepo = messenger::repositories::ChatRepository;
using Message = drogon_model::messenger_db::Messages;
using Chat = drogon_model::messenger_db::Chats;
using ChatPreview = messenger::dto::ChatPreview;

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

Task<HttpResponsePtr> ChatService::getMessageById(
    const std::shared_ptr<Json::Value> request_json,
    int64_t message_id
) {
    Json::Value response_json;
    std::optional<Message> message =
        co_await chat_repo->getMessageById(message_id);
    if (!message.has_value()) {
        LOG_WARN << "Message with id " << message_id << " doesn't exist";
        response_json["message"] =
            "Message with id " + std::to_string(message_id) + " doesn't exist";
        RETURN_RESPONSE_CODE_404(response_json)
    }
    bool is_member = co_await checkChatAccess(
        (*request_json)["user_id"].asInt64(), message->getValueOfChatId()
    );
    if (!is_member) {
        response_json["message"] = "Access denied";
        RETURN_RESPONSE_CODE_403(response_json);
    }
    response_json["message"] = message->getValueOfText();
    RETURN_RESPONSE_CODE_200(response_json)
}

Task<HttpResponsePtr> ChatService::getUserChats(
    const std::shared_ptr<Json::Value> request_json,
    int64_t user_id
) {
    Json::Value response_json;
    if ((*request_json)["user_id"].asInt64() != user_id) {
        response_json["message"] = "Access denied";
        RETURN_RESPONSE_CODE_403(response_json)
    }
    std::vector<ChatPreview> chats_previews =
        co_await chat_repo->getByUser(user_id);
    Json::Value jsonArray(Json::arrayValue);
    for (const auto &chat_preview : chats_previews) {
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
        chat_json["unread_count"] = chat_preview.unread_count;
        chat_json["type"] = chat_preview.type;
        jsonArray.append(chat_json);
    }
    response_json["chats"] = jsonArray;
    RETURN_RESPONSE_CODE_200(response_json)
}

Task<HttpResponsePtr> ChatService::createOrGetDirectChat(
    const std::shared_ptr<Json::Value> request_json
) {
    Json::Value response_json;
    int64_t user_id = (*request_json)["user_id"].asInt64();
    int64_t other_user_id = (*request_json)["target_user_id"].asInt64();
    std::optional<Chat> chat;
    if (user_id == other_user_id) {
        chat = co_await chat_repo->getSaved(user_id);
        response_json["chat"] = chat->toJson();
        RETURN_RESPONSE_CODE_200(response_json)
    }
    chat = co_await chat_repo->getDirect(user_id, other_user_id);
    if (chat.has_value()) {
        response_json["chat"] = chat->toJson();
        RETURN_RESPONSE_CODE_200(response_json)
    }
    chat = co_await chat_repo->getOrCreateDirect(user_id, other_user_id);
    response_json["chat"] = chat->toJson();
    RETURN_RESPONSE_CODE_201(response_json)
}

Task<HttpResponsePtr> ChatService::getChatMessages(
    const std::shared_ptr<Json::Value> request_json,
    int64_t chat_id
) {
    Json::Value response_json;
    int64_t user_id = (*request_json)["user_id"].asInt64();
    std::optional<int64_t> before_message_id =
        request_json->isMember("before_id")
            ? std::optional<int64_t>((*request_json)["before_id"].asInt64())
            : std::nullopt;
    int64_t limit = request_json->isMember("limit")
                        ? (*request_json)["limit"].asInt64()
                        : 50;
    if (limit > 100) {
        limit = 100;
    }
    if (limit < 1) {
        limit = 1;
    }
    bool is_member =
        co_await checkChatAccess((*request_json)["user_id"].asInt64(), chat_id);
    if (!is_member) {
        response_json["message"] = "Access denied";
        RETURN_RESPONSE_CODE_403(response_json);
    }

    std::vector<Message> chat_messages = co_await chat_repo->getMessagesByChat(
        chat_id, before_message_id, limit
    );
    Json::Value jsonArray(Json::arrayValue);
    for (const auto &message : chat_messages) {
        jsonArray.append(message.toJson());
    }
    response_json["messages"] = jsonArray;
    RETURN_RESPONSE_CODE_200(response_json)
}

Task<HttpResponsePtr> ChatService::sendMessage(
    const std::shared_ptr<Json::Value> request_json,
    int64_t chat_id
) {
    Json::Value response_json;
    int64_t user_id = (*request_json)["user_id"].asInt64();

    bool is_member =
        co_await checkChatAccess((*request_json)["user_id"].asInt64(), chat_id);
    if (!is_member) {
        response_json["message"] = "Access denied";
        RETURN_RESPONSE_CODE_403(response_json);
    }

    std::string text = (*request_json)["text"].asString();
    std::optional<int64_t> reply_to_id =
        request_json->isMember("reply_to_id")
            ? std::optional<int64_t>((*request_json)["reply_to_id"].asInt64())
            : std::nullopt;
    std::optional<int64_t> forward_from_id =
        request_json->isMember("forward_from_id")
            ? std::optional<int64_t>((*request_json)["forward_from_id"].asInt64(
              ))
            : std::nullopt;
    Message message = co_await chat_repo->sendMessage(
        chat_id, user_id, text, reply_to_id, forward_from_id
    );
    bool successfully_read_sended_message = co_await chat_repo->markAsRead(
        message.getValueOfChatId(), user_id, message.getValueOfId()
    );
    if (!successfully_read_sended_message) {
        LOG_WARN << "Couldnt't mark message as read";
        response_json["warn"] =
            "Internal server error: failed to mark message as read";
    }
    Json::Value websocket_message_json;
    websocket_message_json["event_type"] = "NEW_MESSAGE";
    websocket_message_json["data"]["message"] = message.toJson();
    std::vector<messenger::repositories::ChatMember> chat_members =
        co_await chat_repo->getMembers(chat_id);
    for (auto &chat_member : chat_members) {
        if (chat_member.getValueOfUserId() != user_id) {
            ServerWebSocketController::notifyUser(
                chat_member.getValueOfUserId(),
                websocket_message_json.toStyledString()
            );
        }
    }
    response_json["message"] = message.toJson();
    RETURN_RESPONSE_CODE_201(response_json)
}

Task<HttpResponsePtr> ChatService::readMessages(
    const std::shared_ptr<Json::Value> request_json,
    int64_t chat_id
) {
    Json::Value response_json;
    int64_t user_id = (*request_json)["user_id"].asInt64();

    bool success = co_await chat_repo->markAsRead(
        chat_id, (*request_json)["user_id"].asInt64(),
        (*request_json)["last_read_message_id"].asInt64()
    );
    if (success) {
        response_json["message"] = "Succussfully read last messages";
        RETURN_RESPONSE_CODE_200(response_json)
    } else {
        response_json["message"] = "Something went wrong";
        RETURN_RESPONSE_CODE_500(response_json)
    }
}