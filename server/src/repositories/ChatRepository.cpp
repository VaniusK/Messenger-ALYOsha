#include "repositories/ChatRepository.hpp"
#include <drogon/orm/Criteria.h>
#include <stdexcept>
#include "repositories/MessageRepository.hpp"
#include "utils/Enum.hpp"

using Chat = drogon_model::messenger_db::Chats;
using Message = drogon_model::messenger_db::Messages;
using ChatMember = drogon_model::messenger_db::ChatMembers;
using ChatRepository = messenger::repositories::ChatRepository;

using namespace drogon;
using namespace drogon::orm;

Task<std::optional<Message>> ChatRepository::getMessageById(int64_t id) {
    auto result = co_await message_repo_->getById(id);
    co_return result;
}

Task<std::vector<Message>> ChatRepository::getAllMessages() {
    auto result = co_await message_repo_->getAll();
    co_return result;
}

Task<Message> ChatRepository::sendMessage(
    int64_t chat_id,
    int64_t sender_id,
    std::string text,
    std::optional<int64_t> reply_to_id,
    std::optional<int64_t> forwarded_from_id
) {
    auto result = co_await message_repo_->send(
        chat_id, sender_id, text, reply_to_id, forwarded_from_id
    );
    co_return result;
}

Task<std::vector<Message>> ChatRepository::getMessagesByChat(
    int64_t chat_id,
    std::optional<int64_t> before_id,
    int64_t limit
) {
    auto result = co_await message_repo_->getByChat(chat_id, before_id, limit);
    co_return result;
}

Task<bool> ChatRepository::editMessage(int64_t id, std::string text) {
    auto result = co_await message_repo_->edit(id, text);
    co_return result;
}

Task<bool> ChatRepository::removeMessage(int64_t id) {
    auto result = co_await message_repo_->remove(id);
    co_return result;
}

Task<Chat>
ChatRepository::getOrCreateDirect(int64_t user1_id, int64_t user2_id) {
    auto mapper = getMapper();
    auto chat_member_mapper = getChatMemberMapper();

    if (user1_id > user2_id) {
        std::swap(user1_id, user2_id);
    }

    try {
        std::vector<Chat> chats = co_await mapper.findBy(
            Criteria(
                Chat::Cols::_direct_user1_id, CompareOperator::EQ, user1_id
            ) &&
            Criteria(
                Chat::Cols::_direct_user2_id, CompareOperator::EQ, user2_id
            )
        );
        if (!chats.empty()) {
            co_return chats[0];
        }
        Chat chat;
        chat.setDirectUser1Id(user1_id);
        chat.setDirectUser2Id(user2_id);
        chat.setType(messenger::models::ChatType::Direct);
        chat = co_await mapper.insert(chat);
        ChatMember chat_member1;
        ChatMember chat_member2;
        chat_member1.setChatId(chat.getValueOfId());
        chat_member1.setUserId(user1_id);
        chat_member1.setRole(messenger::models::ChatRole::Member);
        co_await chat_member_mapper.insert(chat_member1);
        chat_member2.setChatId(chat.getValueOfId());
        chat_member2.setUserId(user2_id);
        chat_member2.setRole(messenger::models::ChatRole::Member);
        co_await chat_member_mapper.insert(chat_member2);
        co_return chat;
    } catch (const DrogonDbException &e) {
        throw std::runtime_error("Database error");
    }
}
