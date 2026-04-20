#pragma once
#include <drogon/drogon.h>
#include <drogon/orm/CoroMapper.h>
#include <drogon/utils/coroutine.h>
#include "dto/AttachmentData.hpp"
#include "models/Messages.h"
#include "repositories/AttachmentRepository.hpp"

namespace messenger::repositories {

using Message = drogon_model::messenger_db::Messages;

class MessageRepositoryInterface {
public:
    virtual ~MessageRepositoryInterface() = default;

    MessageRepositoryInterface(
        std::unique_ptr<AttachmentRepositoryInterface> attachment_repo
    )
        : attachment_repo_(std::move(attachment_repo)) {
    }

    virtual drogon::Task<std::optional<Message>> getById(int64_t id) = 0;
    virtual drogon::Task<std::vector<Message>> getAll() = 0;
    virtual drogon::Task<std::pair<Message, std::vector<Attachment>>> send(
        int64_t chat_id,
        int64_t sender_id,
        std::string text,
        std::optional<int64_t> reply_to_id,
        std::optional<int64_t> forwarded_from_id,
        std::string type,
        std::vector<dto::AttachmentData> attachments = {},
        std::shared_ptr<drogon::orm::Transaction> transaction_ptr = nullptr
    ) = 0;
    virtual drogon::Task<std::vector<Message>> getByChat(
        int64_t chat_id,
        std::optional<int64_t> before_id,
        int64_t limit
    ) = 0;
    virtual drogon::Task<bool> edit(
        int64_t id,
        std::string text,
        std::shared_ptr<drogon::orm::Transaction> transaction_ptr = nullptr
    ) = 0;
    virtual drogon::Task<bool> remove(
        int64_t id,
        std::shared_ptr<drogon::orm::Transaction> transaction_ptr = nullptr
    ) = 0;

protected:
    std::unique_ptr<AttachmentRepositoryInterface> attachment_repo_;
};

class MessageRepository : public MessageRepositoryInterface {
public:
    using MessageRepositoryInterface::MessageRepositoryInterface;

    drogon::Task<std::optional<Message>> getById(int64_t id) override;
    drogon::Task<std::vector<Message>> getAll() override;
    drogon::Task<std::pair<Message, std::vector<Attachment>>> send(
        int64_t chat_id,
        int64_t sender_id,
        std::string text,
        std::optional<int64_t> reply_to_id,
        std::optional<int64_t> forwarded_from_id,
        std::string type,
        std::vector<dto::AttachmentData> attachments = {},
        std::shared_ptr<drogon::orm::Transaction> transaction_ptr = nullptr
    ) override;
    drogon::Task<std::vector<Message>> getByChat(
        int64_t chat_id,
        std::optional<int64_t> before_id,
        int64_t limit
    ) override;

    drogon::Task<bool> edit(
        int64_t id,
        std::string text,
        std::shared_ptr<drogon::orm::Transaction> transaction_ptr = nullptr
    ) override;
    drogon::Task<bool> remove(
        int64_t id,
        std::shared_ptr<drogon::orm::Transaction> transaction_ptr = nullptr
    ) override;

private:
    drogon::orm::CoroMapper<Message> getMapper(
        std::shared_ptr<drogon::orm::Transaction> transaction_ptr = nullptr
    ) {
        if (transaction_ptr) {
            return drogon::orm::CoroMapper<Message>(transaction_ptr);
        }
        return drogon::orm::CoroMapper<Message>(drogon::app().getDbClient());
    }
};

}  // namespace messenger::repositories