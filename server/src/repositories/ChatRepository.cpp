#include "repositories/ChatRepository.hpp"
#include <drogon/orm/Criteria.h>
#include <drogon/orm/Exception.h>
#include <algorithm>
#include <iterator>
#include <stdexcept>
#include "repositories/MessageRepository.hpp"
#include "repositories/UserRepository.hpp"
#include "utils/Enum.hpp"

using Chat = drogon_model::messenger_db::Chats;
using User = drogon_model::messenger_db::Users;
using Message = drogon_model::messenger_db::Messages;
using ChatMember = drogon_model::messenger_db::ChatMembers;
using ChatRepository = messenger::repositories::ChatRepository;
using UserRepository = messenger::repositories::UserRepository;
using ChatPreview = messenger::dto::ChatPreview;

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

Task<std::optional<Chat>>
ChatRepository::getDirect(int64_t user1_id, int64_t user2_id) {
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
        co_return std::nullopt;
    } catch (const DrogonDbException &e) {
        throw std::runtime_error("Database error");
    }
}

Task<std::optional<Chat>> ChatRepository::getById(int64_t id) {
    auto mapper = getMapper();

    try {
        Chat chat = co_await mapper.findByPrimaryKey(id);
        co_return chat;
    } catch (UnexpectedRows &e) {
        co_return std::nullopt;
    } catch (const DrogonDbException &e) {
        throw std::runtime_error("Database error");
    }
}

Task<std::vector<ChatPreview>> ChatRepository::getByUser(int64_t user_id) {
    auto mapper = getMapper();
    auto chat_member_mapper = getChatMemberMapper();
    auto message_mapper = getMessageMapper();
    auto user_mapper = getUserMapper();

    try {
        User user = co_await user_mapper.findByPrimaryKey(user_id);
        std::vector<ChatMember> members = co_await chat_member_mapper.findBy(
            Criteria(ChatMember::Cols::_user_id, CompareOperator::EQ, user_id)
        );
        std::vector<int64_t> chat_ids;
        chat_ids.reserve(members.size());
        std::transform(
            members.begin(), members.end(), std::back_inserter(chat_ids),
            [](const ChatMember &m) { return m.getValueOfChatId(); }
        );
        std::vector<Chat> chats = co_await mapper.findBy(
            Criteria(Chat::Cols::_id, CompareOperator::In, chat_ids)
        );
        std::vector<ChatPreview> previews;
        previews.reserve(chats.size());
        for (const auto &chat : chats) {
            ChatMember member = co_await chat_member_mapper.findOne(
                Criteria(
                    ChatMember::Cols::_chat_id, CompareOperator::EQ,
                    chat.getValueOfId()
                ) &&
                Criteria(
                    ChatMember::Cols::_user_id, CompareOperator::EQ, user_id
                )
            );
            auto messages = co_await message_mapper
                                .orderBy(Message::Cols::_id, SortOrder::DESC)
                                .limit(1)
                                .findBy(Criteria(
                                    Message::Cols::_chat_id,
                                    CompareOperator::EQ, chat.getValueOfId()
                                ));
            std::optional<Message> last_message;
            if (messages.size() > 0) {
                last_message = messages[0];
            }
            previews.push_back(ChatPreview{
                .chat_id = chat.getValueOfId(),
                .title = chat.getValueOfName(),
                .avatar_path = chat.getValueOfAvatarPath(),
                .last_message = last_message,
                .unread_count = static_cast<int64_t>(
                    (co_await message_mapper.limit(999)
                         .orderBy(Message::Cols::_id, SortOrder::DESC)
                         .findBy(
                             Criteria(
                                 Message::Cols::_chat_id, CompareOperator::EQ,
                                 chat.getValueOfId()
                             ) &&
                             Criteria(
                                 Message::Cols::_id, CompareOperator::GT,
                                 member.getValueOfLastReadMessageId()
                             )
                         ))
                        .size()
                )  // Логику оставляю тебе
            });
        }
        co_return previews;
    } catch (const DrogonDbException &e) {
        throw std::runtime_error("Database error");
    }
}
