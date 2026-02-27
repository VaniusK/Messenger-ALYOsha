#pragma once
#include <drogon/drogon.h>
#include <drogon/orm/CoroMapper.h>
#include <drogon/utils/coroutine.h>
#include "models/ChatMembers.h"
#include "models/Chats.h"
#include "models/Messages.h"
#include "repositories/MessageRepository.hpp"
#include "repositories/UserRepository.hpp"

namespace messenger::repositories {

using Chat = drogon_model::messenger_db::Chats;
using ChatMember = drogon_model::messenger_db::ChatMembers;
using Message = drogon_model::messenger_db::Messages;

class ChatRepositoryInterface {
public:
    virtual ~ChatRepositoryInterface() = default;

    ChatRepositoryInterface(
        std::unique_ptr<MessageRepositoryInterface> message_repo
    )
        : message_repo_(std::move(message_repo)) {
    }

    virtual drogon::Task<Chat>
    getOrCreateDirect(int64_t user1_id, int64_t user2_id) = 0;
    virtual drogon::Task<std::optional<Message>> getMessageById(int64_t id) = 0;
    virtual drogon::Task<std::vector<Message>> getAllMessages() = 0;
    virtual drogon::Task<Message> sendMessage(
        int64_t chat_id,
        int64_t sender_id,
        std::string text,
        std::optional<int64_t> reply_to_id,
        std::optional<int64_t> forwarded_from_id
    ) = 0;
    virtual drogon::Task<std::vector<Message>> getMessagesByChat(
        int64_t chat_id,
        std::optional<int64_t> before_id,
        int64_t limit
    ) = 0;
    virtual drogon::Task<bool> editMessage(int64_t id, std::string text) = 0;
    virtual drogon::Task<bool> removeMessage(int64_t id) = 0;

private:
    std::unique_ptr<MessageRepositoryInterface> message_repo_;
};

class ChatRepository : public ChatRepositoryInterface {
public:
    using ChatRepositoryInterface::ChatRepositoryInterface;
    drogon::Task<Chat> getOrCreateDirect(int64_t user1_id, int64_t user2_id)
        override;

    drogon::Task<std::optional<Message>> getMessageById(int64_t id) override;
    drogon::Task<std::vector<Message>> getAllMessages() override;
    drogon::Task<Message> sendMessage(
        int64_t chat_id,
        int64_t sender_id,
        std::string text,
        std::optional<int64_t> reply_to_id,
        std::optional<int64_t> forwarded_from_id
    ) override;
    drogon::Task<std::vector<Message>> getMessagesByChat(
        int64_t chat_id,
        std::optional<int64_t> before_id,
        int64_t limit
    ) override;

    drogon::Task<bool> editMessage(int64_t id, std::string text) override;
    drogon::Task<bool> removeMessage(int64_t id) override;

private:
    drogon::orm::CoroMapper<Chat> getMapper() {
        return drogon::orm::CoroMapper<Chat>(drogon::app().getDbClient());
    }

    drogon::orm::CoroMapper<ChatMember> getChatMemberMapper() {
        return drogon::orm::CoroMapper<ChatMember>(drogon::app().getDbClient());
    }

    std::unique_ptr<MessageRepositoryInterface> message_repo_;
};

}  // namespace messenger::repositories