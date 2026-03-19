#pragma once
#include <drogon/drogon.h>
#include <drogon/orm/CoroMapper.h>
#include <drogon/utils/coroutine.h>
#include "models/Messages.h"

namespace messenger::repositories {

using Message = drogon_model::messenger_db::Messages;

class MessageRepositoryInterface {
public:
    virtual ~MessageRepositoryInterface() = default;
    virtual drogon::Task<std::optional<Message>> getById(int64_t id) = 0;
    virtual drogon::Task<std::vector<Message>> getAll() = 0;
    virtual drogon::Task<Message> send(
        int64_t chat_id,
        int64_t sender_id,
        std::string text,
        std::optional<int64_t> reply_to_id,
        std::optional<int64_t> forwarded_from_id
    ) = 0;
    virtual drogon::Task<std::vector<Message>> getByChat(
        int64_t chat_id,
        std::optional<int64_t> before_id,
        int64_t limit
    ) = 0;
    virtual drogon::Task<bool> edit(int64_t id, std::string text) = 0;
    virtual drogon::Task<bool> remove(int64_t id) = 0;
};

class MessageRepository : public MessageRepositoryInterface {
public:
    drogon::Task<std::optional<Message>> getById(int64_t id) override;
    drogon::Task<std::vector<Message>> getAll() override;
    drogon::Task<Message> send(
        int64_t chat_id,
        int64_t sender_id,
        std::string text,
        std::optional<int64_t> reply_to_id,
        std::optional<int64_t> forwarded_from_id
    ) override;
    drogon::Task<std::vector<Message>> getByChat(
        int64_t chat_id,
        std::optional<int64_t> before_id,
        int64_t limit
    ) override;

    drogon::Task<bool> edit(int64_t id, std::string text) override;
    drogon::Task<bool> remove(int64_t id) override;

private:
    drogon::orm::CoroMapper<Message> getMapper() {
        return drogon::orm::CoroMapper<Message>(drogon::app().getDbClient());
    }
};

}  // namespace messenger::repositories