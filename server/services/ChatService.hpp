#pragma once

#include <drogon/HttpController.h>
#include <memory>
#include "repositories/ChatRepository.hpp"

using namespace drogon;

namespace api {
namespace v1 {
class ChatService {
public:
    static Task<HttpResponsePtr> getMessageById(
        const std::shared_ptr<Json::Value> request_json,
        int64_t message_id,
        const std::shared_ptr<messenger::repositories::ChatRepositoryInterface>
            chat_repo
    );
    static Task<HttpResponsePtr> getUserChats(
        const std::shared_ptr<Json::Value> request_json,
        int64_t user_id,
        const std::shared_ptr<messenger::repositories::ChatRepositoryInterface>
            chat_repo
    );
    static Task<HttpResponsePtr> createOrGetDirectChat(
        const std::shared_ptr<Json::Value> request_json,
        const std::shared_ptr<messenger::repositories::ChatRepositoryInterface>
            chat_repo
    );
    static Task<HttpResponsePtr> getChatMessages(
        const std::shared_ptr<Json::Value> request_json,
        int64_t chat_id,
        const std::shared_ptr<messenger::repositories::ChatRepositoryInterface>
            chat_repo
    );
    static Task<HttpResponsePtr> sendMessage(
        const std::shared_ptr<Json::Value> request_json,
        int64_t chat_id,
        const std::shared_ptr<messenger::repositories::ChatRepositoryInterface>
            chat_repo
    );
    static Task<HttpResponsePtr> readMessages(
        const std::shared_ptr<Json::Value> request_json,
        int64_t chat_id,
        const std::shared_ptr<messenger::repositories::ChatRepositoryInterface>
            chat_repo
    );
};
}  // namespace v1
}  // namespace api
