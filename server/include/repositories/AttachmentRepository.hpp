#pragma once
#include <drogon/drogon.h>
#include <drogon/orm/CoroMapper.h>
#include <drogon/utils/coroutine.h>
#include "models/Attachments.h"

namespace messenger::repositories {

using Attachment = drogon_model::messenger_db::Attachments;

class AttachmentRepositoryInterface {
public:
    virtual ~AttachmentRepositoryInterface() = default;
    virtual drogon::Task<Attachment> create(
        int64_t message_id,
        std::string file_name,
        std::string file_type,
        int64_t file_size_bytes,
        std::string s3_object_key,
        std::shared_ptr<drogon::orm::Transaction> transaction_ptr = nullptr
    ) = 0;
    virtual drogon::Task<std::vector<Attachment>> getByMessage(
        int64_t message_id
    ) = 0;
    virtual drogon::Task<std::vector<std::vector<Attachment>>> getByMessages(
        std::vector<int64_t> message_ids
    ) = 0;
    virtual drogon::Task<bool> remove(
        int64_t id,
        std::shared_ptr<drogon::orm::Transaction> transaction_ptr = nullptr
    ) = 0;
};

class AttachmentRepository : public AttachmentRepositoryInterface {
public:
    drogon::Task<Attachment> create(
        int64_t message_id,
        std::string file_name,
        std::string file_type,
        int64_t file_size_bytes,
        std::string s3_object_key,
        std::shared_ptr<drogon::orm::Transaction> transaction_ptr = nullptr
    ) override;
    drogon::Task<std::vector<Attachment>> getByMessage(int64_t message_id
    ) override;
    drogon::Task<std::vector<std::vector<Attachment>>> getByMessages(
        std::vector<int64_t> message_ids
    ) override;
    drogon::Task<bool> remove(
        int64_t id,
        std::shared_ptr<drogon::orm::Transaction> transaction_ptr = nullptr
    ) override;

private:
    drogon::orm::CoroMapper<Attachment> getMapper(
        std::shared_ptr<drogon::orm::Transaction> transaction_ptr = nullptr
    ) {
        if (transaction_ptr) {
            return drogon::orm::CoroMapper<Attachment>(transaction_ptr);
        }
        return drogon::orm::CoroMapper<Attachment>(drogon::app().getDbClient());
    }
};

}  // namespace messenger::repositories