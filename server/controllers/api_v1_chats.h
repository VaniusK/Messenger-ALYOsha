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
class chats : public drogon::HttpController<chats>
{
  public:
    METHOD_LIST_BEGIN
    METHOD_ADD(chats::getUserChats, "/user/{1:user_id}", Get, "api::v1::IpFilter", "api::v1::AuthFilter");
    METHOD_ADD(chats::createOrGetDirectChat, "/direct", Post, "api::v1::IpFilter", "api::v1::JsonValidatorFilter", "api::v1::AuthFilter");
    METHOD_ADD(chats::getChatMessages, "/{1:chat_id}/messages", Get, "api::v1::IpFilter", "api::v1::AuthFilter");
    METHOD_ADD(chats::sendMessage, "/{1:chat_id}/messages", Post, "api::v1::IpFilter", "api::v1::JsonValidatorFilter", "api::v1::AuthFilter");
    METHOD_ADD(chats::readMessages, "/{1:chat_id}/read", Post, "api::v1::IpFilter", "api::v1::JsonValidatorFilter", "api::v1::AuthFilter");
    METHOD_ADD(chats::getMessageById, "/messages/{1:message_id}", Get, "api::v1::IpFilter", "api::v1::AuthFilter");
    METHOD_LIST_END
    Task<HttpResponsePtr> getUserChats(const HttpRequestPtr req, int64_t user_id);
    Task<HttpResponsePtr> createOrGetDirectChat(const HttpRequestPtr req);
    Task<HttpResponsePtr> getChatMessages(const HttpRequestPtr req, int64_t chat_id);
    Task<HttpResponsePtr> sendMessage(const HttpRequestPtr req, int64_t chat_id);
    Task<HttpResponsePtr> readMessages(const HttpRequestPtr req, int64_t chat_id);
    Task<HttpResponsePtr> getMessageById(const HttpRequestPtr req, int64_t message_id);

    chats() {
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
