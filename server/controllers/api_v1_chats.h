#pragma once

#include <drogon/HttpController.h>
#include "filters/api_v1_AuthFilter.h"

using namespace drogon;

namespace api
{
namespace v1
{
class chats : public drogon::HttpController<chats>
{
  public:
    METHOD_LIST_BEGIN
    METHOD_ADD(chats::getUserChats, "/user/{1:user_id}", Get, "api::v1::AuthFilter");
    METHOD_ADD(chats::createChat, "/direct", Post);
    METHOD_ADD(chats::getChatMessages, "/{1:chat_id}/messages", Get);
    METHOD_ADD(chats::sendMessage, "/{1:chat_id}/messages", Post);
    METHOD_ADD(chats::readMessages, "/{1:chat_id}/read", Post);
    METHOD_LIST_END
    Task<HttpResponsePtr> getUserChats(const HttpRequestPtr req, std::string &&user_id);
    Task<HttpResponsePtr> createChat(const HttpRequestPtr req);
    Task<HttpResponsePtr> getChatMessages(const HttpRequestPtr req, std::string &&chat_id);
    Task<HttpResponsePtr> sendMessage(const HttpRequestPtr req, std::string &&chat_id);
    Task<HttpResponsePtr> readMessages(const HttpRequestPtr req, std::string &&chat_id);
};
}
}
