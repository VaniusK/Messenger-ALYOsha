#pragma once

#include <drogon/HttpController.h>
#include <memory>
#include "repositories/ChatRepository.hpp"
#include "repositories/MessageRepository.hpp"
#include "repositories/UserRepository.hpp"
#include "services/ChatService.hpp"

using namespace drogon;

namespace api
{
namespace v1
{
class ChatController : public drogon::HttpController<ChatController>
{
  public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(ChatController::getUserChats, "/v1/chats/user/{1:user_id}", Get, "api::v1::AuthFilter");
    ADD_METHOD_TO(ChatController::createOrGetDirectChat, "/v1/chats/direct", Post, "api::v1::JsonValidatorFilter", "api::v1::AuthFilter");
    ADD_METHOD_TO(ChatController::getChatMessages, "/v1/chats/{1:chat_id}/messages", Get, "api::v1::AuthFilter");
    ADD_METHOD_TO(ChatController::sendMessage, "/v1/chats/{1:chat_id}/messages", Post, "api::v1::JsonValidatorFilter", "api::v1::AuthFilter");
    ADD_METHOD_TO(ChatController::readMessages, "/v1/chats/{1:chat_id}/read", Post, "api::v1::JsonValidatorFilter", "api::v1::AuthFilter");
    ADD_METHOD_TO(ChatController::getMessageById, "/v1/chats/messages/{1:message_id}", Get, "api::v1::AuthFilter");
    METHOD_LIST_END
    Task<HttpResponsePtr> getUserChats(const HttpRequestPtr req, int64_t user_id);
    Task<HttpResponsePtr> createOrGetDirectChat(const HttpRequestPtr req);
    Task<HttpResponsePtr> getChatMessages(const HttpRequestPtr req, int64_t chat_id);
    Task<HttpResponsePtr> sendMessage(const HttpRequestPtr req, int64_t chat_id);
    Task<HttpResponsePtr> readMessages(const HttpRequestPtr req, int64_t chat_id);
    Task<HttpResponsePtr> getMessageById(const HttpRequestPtr req, int64_t message_id);

    ChatController() {
      chat_service.setChatRepo(std::make_shared<messenger::repositories::ChatRepository>(std::make_unique<messenger::repositories::MessageRepository>(), std::make_unique<messenger::repositories::UserRepository>()));
    }
    void setRepo(std::shared_ptr<messenger::repositories::ChatRepositoryInterface> chat_repo) {
      this->chat_service.setChatRepo(chat_repo);
    }
  private:
    ChatService chat_service;
};
}
}
