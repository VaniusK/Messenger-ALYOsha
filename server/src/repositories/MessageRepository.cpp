#include "repositories/MessageRepository.hpp"
#include <drogon/orm/Criteria.h>
#include <stdexcept>

using Message = drogon_model::messenger_db::Messages;
using MessageRepository = messenger::repositories::MessageRepository;

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
        }
        co_await mapper.insert(message);
    } catch (const DrogonDbException &e) {
        throw std::runtime_error("Database error");
    }
}
