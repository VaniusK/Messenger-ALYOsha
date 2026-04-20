#pragma once
#include <drogon/drogon.h>
#include <drogon/orm/CoroMapper.h>
#include <drogon/orm/DbClient.h>
#include <drogon/utils/coroutine.h>
#include "dto/ChatPreview.hpp"
#include "models/ChatMembers.h"
#include "models/Chats.h"
#include "models/Messages.h"
#include "models/Users.h"
#include "repositories/MessageRepository.hpp"
#include "repositories/UserRepository.hpp"

namespace messenger::repositories {

using Chat = drogon_model::messenger_db::Chats;
using User = drogon_model::messenger_db::Users;
using ChatMember = drogon_model::messenger_db::ChatMembers;
using Message = drogon_model::messenger_db::Messages;
using ChatPreview = messenger::dto::ChatPreview;

class ChatRepositoryInterface {
public:
    virtual ~ChatRepositoryInterface() = default;

    ChatRepositoryInterface(
        std::unique_ptr<MessageRepositoryInterface> message_repo,
        std::unique_ptr<UserRepositoryInterface> user_repo
    )
        : message_repo_(std::move(message_repo)),
          user_repo_(std::move(user_repo)) {
    }

    virtual drogon::Task<std::optional<Message>> getMessageById(int64_t id) = 0;
    virtual drogon::Task<std::vector<Message>> getAllMessages() = 0;
    virtual drogon::Task<std::pair<Message, std::vector<Attachment>>>
    sendMessage(
        int64_t chat_id,
        int64_t sender_id,
        std::string text,
        std::optional<int64_t> reply_to_id,
        std::optional<int64_t> forwarded_from_id,
        std::string type,
        std::vector<dto::AttachmentData> attachments = {},
        std::shared_ptr<drogon::orm::Transaction> transaction_ptr = nullptr
    ) = 0;
    virtual drogon::Task<std::vector<Message>> getMessagesByChat(
        int64_t chat_id,
        std::optional<int64_t> before_id,
        int64_t limit
    ) = 0;
    virtual drogon::Task<bool> editMessage(
        int64_t id,
        std::string text,
        std::shared_ptr<drogon::orm::Transaction> transaction_ptr = nullptr
    ) = 0;
    virtual drogon::Task<bool> removeMessage(
        int64_t id,
        std::shared_ptr<drogon::orm::Transaction> transaction_ptr = nullptr
    ) = 0;
    virtual drogon::Task<Chat> getOrCreateDirect(
        int64_t user1_id,
        int64_t user2_id,
        std::shared_ptr<drogon::orm::Transaction> transaction_ptr = nullptr
    ) = 0;
    virtual drogon::Task<std::optional<Chat>>
    getDirect(int64_t user1_id, int64_t user2_id) = 0;
    virtual drogon::Task<std::optional<Chat>> getById(int64_t id) = 0;
    virtual drogon::Task<std::vector<ChatPreview>> getByUser(int64_t user_id
    ) = 0;
    virtual drogon::Task<bool> markAsRead(
        int64_t chat_id,
        int64_t user_id,
        int64_t last_read_message_id,
        std::shared_ptr<drogon::orm::Transaction> transaction_ptr = nullptr
    ) = 0;
    virtual drogon::Task<Chat> createGroup(
        std::string name,
        int64_t creator_id,
        std::vector<int64_t> member_ids,
        std::shared_ptr<drogon::orm::Transaction> transaction_ptr = nullptr
    ) = 0;
    virtual drogon::Task<std::vector<ChatMember>> getMembers(int64_t chat_id
    ) = 0;
    drogon::Task<ChatMember> virtual addMember(
        int64_t chat_id,
        int64_t user_id,
        std::string role,
        std::shared_ptr<drogon::orm::Transaction> transaction_ptr = nullptr
    ) = 0;
    drogon::Task<bool> virtual removeMember(
        int64_t chat_id,
        int64_t user_id,
        std::shared_ptr<drogon::orm::Transaction> transaction_ptr = nullptr
    ) = 0;
    drogon::Task<bool> virtual updateMemberRole(
        int64_t chat_id,
        int64_t user_id,
        std::string new_role,
        std::shared_ptr<drogon::orm::Transaction> transaction_ptr = nullptr
    ) = 0;
    drogon::Task<bool> virtual updateInfo(
        int64_t chat_id,
        std::optional<std::string> name,
        std::optional<std::string> avatar,
        std::optional<std::string> description,
        std::shared_ptr<drogon::orm::Transaction> transaction_ptr = nullptr
    ) = 0;
    drogon::Task<Chat> virtual createSaved(
        int64_t user_id,
        std::shared_ptr<drogon::orm::Transaction> transaction_ptr = nullptr
    ) = 0;
    drogon::Task<Chat> virtual getSaved(int64_t user_id) = 0;

protected:
    std::unique_ptr<MessageRepositoryInterface> message_repo_;
    std::unique_ptr<UserRepositoryInterface> user_repo_;
};

class ChatRepository : public ChatRepositoryInterface {
public:
    using ChatRepositoryInterface::ChatRepositoryInterface;

    drogon::Task<std::optional<Message>> getMessageById(int64_t id) override;
    drogon::Task<std::vector<Message>> getAllMessages() override;
    drogon::Task<std::pair<Message, std::vector<Attachment>>> sendMessage(
        int64_t chat_id,
        int64_t sender_id,
        std::string text,
        std::optional<int64_t> reply_to_id,
        std::optional<int64_t> forwarded_from_id,
        std::string type,
        std::vector<dto::AttachmentData> attachments = {},
        std::shared_ptr<drogon::orm::Transaction> transaction_ptr = nullptr
    ) override;
    drogon::Task<std::vector<Message>> getMessagesByChat(
        int64_t chat_id,
        std::optional<int64_t> before_id,
        int64_t limit
    ) override;

    drogon::Task<bool> editMessage(
        int64_t id,
        std::string text,
        std::shared_ptr<drogon::orm::Transaction> transaction_ptr = nullptr
    ) override;
    drogon::Task<bool> removeMessage(
        int64_t id,
        std::shared_ptr<drogon::orm::Transaction> transaction_ptr = nullptr
    ) override;
    drogon::Task<Chat> getOrCreateDirect(
        int64_t user1_id,
        int64_t user2_id,
        std::shared_ptr<drogon::orm::Transaction> transaction_ptr = nullptr
    ) override;
    drogon::Task<std::optional<Chat>>
    getDirect(int64_t user1_id, int64_t user2_id) override;
    drogon::Task<std::optional<Chat>> getById(int64_t id) override;
    drogon::Task<std::vector<ChatPreview>> getByUser(int64_t user_id) override;
    drogon::Task<bool> markAsRead(
        int64_t chat_id,
        int64_t user_id,
        int64_t last_read_message_id,
        std::shared_ptr<drogon::orm::Transaction> transaction_ptr = nullptr
    ) override;
    drogon::Task<Chat> createGroup(
        std::string name,
        int64_t creator_id,
        std::vector<int64_t> member_ids,
        std::shared_ptr<drogon::orm::Transaction> transaction_ptr = nullptr
    ) override;
    drogon::Task<std::vector<ChatMember>> getMembers(int64_t chat_id) override;
    drogon::Task<ChatMember> addMember(
        int64_t chat_id,
        int64_t user_id,
        std::string role,
        std::shared_ptr<drogon::orm::Transaction> transaction_ptr = nullptr
    ) override;
    drogon::Task<bool> removeMember(
        int64_t chat_id,
        int64_t user_id,
        std::shared_ptr<drogon::orm::Transaction> transaction_ptr = nullptr
    ) override;
    drogon::Task<bool> updateMemberRole(
        int64_t chat_id,
        int64_t user_id,
        std::string new_role,
        std::shared_ptr<drogon::orm::Transaction> transaction_ptr = nullptr
    ) override;
    drogon::Task<bool> updateInfo(
        int64_t chat_id,
        std::optional<std::string> name,
        std::optional<std::string> avatar,
        std::optional<std::string> description,
        std::shared_ptr<drogon::orm::Transaction> transaction_ptr = nullptr
    ) override;
    drogon::Task<Chat> createSaved(
        int64_t user_id,
        std::shared_ptr<drogon::orm::Transaction> transaction_ptr = nullptr
    ) override;
    drogon::Task<Chat> virtual getSaved(int64_t user_id) override;

private:
    drogon::orm::CoroMapper<Chat> getMapper(
        std::shared_ptr<drogon::orm::Transaction> transaction_ptr = nullptr
    ) {
        if (transaction_ptr) {
            return drogon::orm::CoroMapper<Chat>(transaction_ptr);
        }
        return drogon::orm::CoroMapper<Chat>(drogon::app().getDbClient());
    }

    drogon::orm::CoroMapper<ChatMember> getChatMemberMapper(
        std::shared_ptr<drogon::orm::Transaction> transaction_ptr = nullptr
    ) {
        if (transaction_ptr) {
            return drogon::orm::CoroMapper<ChatMember>(transaction_ptr);
        }
        return drogon::orm::CoroMapper<ChatMember>(drogon::app().getDbClient());
    }

    drogon::orm::CoroMapper<Message> getMessageMapper(
        std::shared_ptr<drogon::orm::Transaction> transaction_ptr = nullptr
    ) {
        if (transaction_ptr) {
            return drogon::orm::CoroMapper<Message>(transaction_ptr);
        }
        return drogon::orm::CoroMapper<Message>(drogon::app().getDbClient());
    }

    drogon::orm::CoroMapper<User> getUserMapper(
        std::shared_ptr<drogon::orm::Transaction> transaction_ptr = nullptr
    ) {
        if (transaction_ptr) {
            return drogon::orm::CoroMapper<User>(transaction_ptr);
        }
        return drogon::orm::CoroMapper<User>(drogon::app().getDbClient());
    }

    drogon::Task<messenger::dto::ChatPreview> buildChatPreview(
        Chat chat,
        ChatMember member,
        std::optional<User> other_user
    );
};

}  // namespace messenger::repositories