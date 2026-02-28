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

// TODO: Сейчас это 2N запросов. Можно написать очень страшный запрос на сыром
// SQL, чтобы получить 2 запроса. Выиграем немного задержки на пинге.

Task<ChatPreview> ChatRepository::buildChatPreview(
    Chat chat,
    ChatMember member,
    std::optional<User> other_user
) {
    auto message_mapper = getMessageMapper();
    Criteria find_crit(
        Message::Cols::_chat_id, CompareOperator::EQ, chat.getValueOfId()
    );

    auto messages =
        co_await message_mapper.orderBy(Message::Cols::_id, SortOrder::DESC)
            .limit(1)
            .findBy(find_crit);

    std::optional<Message> last_message;
    if (messages.size() > 0) {
        last_message = messages[0];
    }

    auto crit = Criteria(
        Message::Cols::_chat_id, CompareOperator::EQ, chat.getValueOfId()
    );
    if (member.getLastReadMessageId()) {
        crit = crit && Criteria(
                           Message::Cols::_id, CompareOperator::GT,
                           member.getValueOfLastReadMessageId()
                       );
    }
    std::string new_title;
    std::optional<std::string> new_avatar_path;
    if (chat.getValueOfType() == models::ChatType::Direct) {
        new_title = other_user.value().getValueOfDisplayName();
        if (other_user.value().getAvatarPath()) {
            new_avatar_path = other_user.value().getValueOfAvatarPath();
        }
    } else {
        new_title = chat.getValueOfName();
        if (chat.getAvatarPath()) {
            new_avatar_path = chat.getValueOfAvatarPath();
        }
    }
    int64_t unread_count =
        static_cast<int64_t>(co_await message_mapper.count(crit));

    ChatPreview preview;
    preview.chat_id = chat.getValueOfId();
    preview.title = new_title;
    preview.avatar_path = new_avatar_path;
    preview.last_message = last_message;
    preview.unread_count = unread_count;

    co_return preview;
}

Task<std::vector<ChatPreview>> ChatRepository::getByUser(int64_t user_id) {
    auto mapper = getMapper();
    auto chat_member_mapper = getChatMemberMapper();
    auto message_mapper = getMessageMapper();

    try {
        std::vector<ChatMember> members = co_await chat_member_mapper.findBy(
            Criteria(ChatMember::Cols::_user_id, CompareOperator::EQ, user_id)
        );
        if (members.empty()) {
            co_return std::vector<ChatPreview>{};
        }
        std::vector<int64_t> chat_ids;
        chat_ids.reserve(members.size());
        std::unordered_map<int64_t, ChatMember> chat_id_to_member;
        std::transform(
            members.begin(), members.end(), std::back_inserter(chat_ids),
            [&chat_id_to_member](const ChatMember &m) mutable {
                chat_id_to_member[m.getValueOfChatId()] = m;
                return m.getValueOfChatId();
            }
        );
        std::vector<Chat> chats = co_await mapper.findBy(
            Criteria(Chat::Cols::_id, CompareOperator::In, chat_ids)
        );
        std::unordered_map<int64_t, User> chat_id_to_other_user;
        std::unordered_map<int64_t, int64_t> other_user_id_to_chat_id;
        std::vector<int64_t> other_user_ids;
        for (const auto &chat : chats) {
            if (chat.getValueOfType() == messenger::models::ChatType::Direct) {
                int64_t other_user_id = chat.getValueOfDirectUser1Id();
                if (other_user_id == user_id) {
                    other_user_id = chat.getValueOfDirectUser2Id();
                }
                other_user_ids.push_back(other_user_id);
                other_user_id_to_chat_id[other_user_id] = chat.getValueOfId();
            }
        }
        std::vector<User> other_users;
        if (!other_user_ids.empty()) {
            other_users = co_await user_repo_->getByIds(other_user_ids);
        }
        for (const User &other_user : other_users) {
            chat_id_to_other_user
                [other_user_id_to_chat_id[other_user.getValueOfId()]] =
                    other_user;
        }
        std::vector<ChatPreview> previews;
        previews.reserve(chats.size());
        Chat chat_copy;
        ChatMember member_copy;
        std::optional<User> other_user_opt;
        for (size_t i = 0; i < chats.size(); ++i) {
            chat_copy = chats[i];
            member_copy = chat_id_to_member[chat_copy.getValueOfId()];
            other_user_opt = std::nullopt;
            auto it = chat_id_to_other_user.find(chat_copy.getValueOfId());
            if (it != chat_id_to_other_user.end()) {
                other_user_opt = it->second;
            }
            previews.push_back(co_await buildChatPreview(
                chat_copy, member_copy, other_user_opt
            ));
        }
        co_return previews;
    } catch (const DrogonDbException &e) {
        throw std::runtime_error("Database error");
    }
}

Task<bool> ChatRepository::markAsRead(
    int64_t chat_id,
    int64_t user_id,
    int64_t last_read_message_id
) {
    auto mapper = getMapper();
    auto chat_member_mapper = getChatMemberMapper();

    try {
        ChatMember member = co_await chat_member_mapper.findOne(
            Criteria(
                ChatMember::Cols::_chat_id, CompareOperator::EQ, chat_id
            ) &&
            Criteria(ChatMember::Cols::_user_id, CompareOperator::EQ, user_id)
        );
        member.setLastReadMessageId(last_read_message_id);
        co_await chat_member_mapper.update(member);
        co_return true;
    } catch (UnexpectedRows &e) {
        co_return false;
    } catch (const DrogonDbException &e) {
        throw std::runtime_error("Database error");
    }
}
