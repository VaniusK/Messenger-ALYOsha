#pragma once

#include <gmock/gmock.h>
#include "models/Attachments.h"
#include "repositories/AttachmentRepository.hpp"

using Attachment = drogon_model::messenger_db::Attachments;

class MockAttachmentRepository
    : public messenger::repositories::AttachmentRepositoryInterface {
public:
    MOCK_METHOD(
        drogon::Task<Attachment>,
        create,
        (int64_t,
         std::string,
         std::string,
         int64_t,
         std::string,
         std::shared_ptr<drogon::orm::Transaction>),
        (override)
    );
    MOCK_METHOD(
        drogon::Task<std::vector<Attachment>>,
        getByMessage,
        (int64_t),
        (override)
    );
    MOCK_METHOD(
        drogon::Task<std::vector<std::vector<Attachment>>>,
        getByMessages,
        (std::vector<int64_t>),
        (override)
    );
    MOCK_METHOD(
        drogon::Task<bool>,
        remove,
        (int64_t, std::shared_ptr<drogon::orm::Transaction>),
        (override)
    );
};