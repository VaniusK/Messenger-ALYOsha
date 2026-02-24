#include "api_v1_chats.h"

using namespace api::v1;

Task<HttpResponsePtr> chats::getUserChats(const HttpRequestPtr req, std::string &&user_id){
    // ChatRepository repo;
    // some logic with co_await
    Json::Value ret;
    ret["status"] = user_id;
    auto resp = HttpResponse::newHttpJsonResponse(ret);
    co_return resp;
}

Task<HttpResponsePtr> chats::createChat(const HttpRequestPtr req){
    // ChatRepository repo;
    // some logic with co_await
    Json::Value ret;
    ret["status"] = "ok";
    auto resp = HttpResponse::newHttpJsonResponse(ret);
    co_return resp;
}

Task<HttpResponsePtr> chats::getChatMessages(const HttpRequestPtr req, std::string &&chat_id){
    // ChatRepository repo;
    // some logic with co_await
    Json::Value ret;
    ret["status"] = chat_id;
    auto resp = HttpResponse::newHttpJsonResponse(ret);
    co_return resp;
}

Task<HttpResponsePtr> chats::sendMessage(const HttpRequestPtr req, std::string &&chat_id){
    // ChatRepository repo;
    // some logic with co_await
    Json::Value ret;
    ret["status"] = chat_id;
    auto resp = HttpResponse::newHttpJsonResponse(ret);
    co_return resp;
}

Task<HttpResponsePtr> chats::readMessages(const HttpRequestPtr req, std::string &&chat_id){
    // ChatRepository repo;
    // some logic with co_await
    Json::Value ret;
    ret["status"] = chat_id;
    auto resp = HttpResponse::newHttpJsonResponse(ret);
    co_return resp;
}
