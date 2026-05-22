#pragma once

#include <gmock/gmock.h>
#include <cstdint>
#include <optional>
#include <string>
#include "gmock/gmock.h"
#include "repositories/MessageRepository.hpp"

using Message = drogon_model::messenger_db::Messages;
using Attachment = drogon_model::messenger_db::Attachments;

class MockMessageRepository
    : public messenger::repositories::MessageRepositoryInterface {
public:
    using MessageRepositoryInterface::MessageRepositoryInterface;
    MOCK_METHOD(
        drogon::Task<std::optional<Message>>,
        getById,
        (int64_t),
        (override)
    );
    MOCK_METHOD(drogon::Task<std::vector<Message>>, getAll, (), (override));
    MOCK_METHOD(
        (drogon::Task<std::pair<Message, std::vector<Attachment>>>),
        send,
        (int64_t,
         int64_t,
         std::string,
         std::optional<int64_t>,
         std::optional<int64_t>,
         std::string,
         std::vector<messenger::dto::AttachmentData>,
         std::shared_ptr<drogon::orm::Transaction>),
        (override)
    );
    MOCK_METHOD(
        drogon::Task<std::vector<Message>>,
        getByChat,
        (int64_t, std::optional<int64_t>, int64_t),
        (override)
    );
    MOCK_METHOD(
        drogon::Task<bool>,
        edit,
        (int64_t, std::string, std::shared_ptr<drogon::orm::Transaction>),
        (override)
    );
    MOCK_METHOD(
        drogon::Task<bool>,
        remove,
        (int64_t, std::shared_ptr<drogon::orm::Transaction>),
        (override)
    );
};