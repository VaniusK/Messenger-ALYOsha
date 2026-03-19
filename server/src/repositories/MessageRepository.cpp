#include "repositories/MessageRepository.hpp"
#include <drogon/orm/Criteria.h>
#include <drogon/orm/Exception.h>
#include <stdexcept>
#include "repositories/UserRepository.hpp"

using Message = drogon_model::messenger_db::Messages;
using User = drogon_model::messenger_db::Users;
using MessageRepository = messenger::repositories::MessageRepository;
using UserRepository = messenger::repositories::UserRepository;

using namespace drogon;
using namespace drogon::orm;

Task<std::optional<Message>> MessageRepository::getById(int64_t id) {
    auto mapper = getMapper();

    try {
        Message message = co_await mapper.findByPrimaryKey(id);

        co_return message;
    } catch (const UnexpectedRows &e) {
        co_return std::nullopt;
    } catch (const DrogonDbException &e) {
        throw std::runtime_error("Database error");
    }
}

Task<std::vector<Message>> MessageRepository::getAll() {
    auto mapper = getMapper();

    try {
        std::vector<Message> messages = co_await mapper.findAll();

        co_return messages;
    } catch (const DrogonDbException &e) {
        throw std::runtime_error("Database error");
    }
}

Task<Message> MessageRepository::send(
    int64_t chat_id,
    int64_t sender_id,
    std::string text,
    std::optional<int64_t> reply_to_id,
    std::optional<int64_t> forwarded_from_id
) {
    auto mapper = getMapper();

    try {
        Message message;
        message.setChatId(chat_id);
        message.setSenderId(sender_id);
        message.setText(text);
        if (reply_to_id.has_value()) {
            message.setReplyToMessageId(reply_to_id.value());
        }
        if (forwarded_from_id.has_value()) {
            message.setForwardedFromUserId(forwarded_from_id.value());
            UserRepository user_repo = UserRepository();
            User forwarded_from_user =
                (co_await user_repo.getById(forwarded_from_id.value())).value();
            message.setForwardedFromUserName(
                forwarded_from_user.getValueOfDisplayName()
            );
        }
        message = co_await mapper.insert(message);
        co_return message;
    } catch (const DrogonDbException &e) {
        throw std::runtime_error("Database error");
    }
}

Task<std::vector<Message>> MessageRepository::getByChat(
    int64_t chat_id,
    std::optional<int64_t> before_id,
    int64_t limit
) {
    auto mapper = getMapper();
    try {
        mapper.limit(limit);
        mapper.orderBy(Message::Cols::_id, SortOrder::DESC);
        auto crit =
            Criteria(Message::Cols::_chat_id, CompareOperator::EQ, chat_id);
        if (before_id.has_value()) {
            crit = crit &&
                   Criteria(Message::Cols::_id, CompareOperator::LE, before_id);
        }
        std::vector<Message> messages = co_await mapper.findBy(crit

        );
        co_return messages;
    } catch (const DrogonDbException &e) {
        throw std::runtime_error("Database error");
    }
}

Task<bool> MessageRepository::edit(int64_t id, std::string text) {
    auto mapper = getMapper();
    try {
        Message message = co_await mapper.findByPrimaryKey(id);
        message.setText(text);
        trantor::Date now = trantor::Date::date();
        message.setEditedAt(now);
        co_await mapper.update(message);
        co_return true;
    } catch (const UnexpectedRows &e) {
        co_return false;
    } catch (const DrogonDbException &e) {
        throw std::runtime_error("Database error");
    }
}

Task<bool> MessageRepository::remove(int64_t id) {
    auto mapper = getMapper();
    try {
        Message message = co_await mapper.findByPrimaryKey(id);
        co_await mapper.deleteByPrimaryKey(id);
        co_return true;
    } catch (const UnexpectedRows &e) {
        co_return false;
    } catch (const DrogonDbException &e) {
        throw std::runtime_error("Database error");
    }
}
