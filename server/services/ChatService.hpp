#pragma once

#include <drogon/HttpController.h>
#include <memory>
#include "repositories/ChatRepository.hpp"

using namespace drogon;

namespace api {
namespace v1 {
class ChatService {
public:
    Task<HttpResponsePtr> getMessageById(
        const std::shared_ptr<Json::Value> request_json,
        int64_t message_id
    );
    Task<HttpResponsePtr> getUserChats(
        const std::shared_ptr<Json::Value> request_json,
        int64_t user_id
    );
    Task<HttpResponsePtr> createOrGetDirectChat(
        const std::shared_ptr<Json::Value> request_json
    );
    Task<HttpResponsePtr> getChatMessages(
        const std::shared_ptr<Json::Value> request_json,
        int64_t chat_id
    );
    Task<HttpResponsePtr> sendMessage(
        const std::shared_ptr<Json::Value> request_json,
        int64_t chat_id
    );
    Task<HttpResponsePtr> readMessages(
        const std::shared_ptr<Json::Value> request_json,
        int64_t chat_id
    );

    void setChatRepo(
        std::shared_ptr<messenger::repositories::ChatRepositoryInterface>
            chat_repo
    ) {
        this->chat_repo = chat_repo;
    }

private:
    std::shared_ptr<messenger::repositories::ChatRepositoryInterface> chat_repo;
};
}  // namespace v1
}  // namespace api
