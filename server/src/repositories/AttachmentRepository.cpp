#include "repositories/AttachmentRepository.hpp"
#include <drogon/orm/Criteria.h>
#include <stdexcept>

using Attachment = drogon_model::messenger_db::Attachments;
using AttachmentRepository = messenger::repositories::AttachmentRepository;

using namespace drogon;
using namespace drogon::orm;
using namespace std;

Task<Attachment> AttachmentRepository::create(
    int64_t message_id,
    std::string file_name,
    std::string file_type,
    int64_t file_size_bytes,
    std::string s3_object_key
) {
    auto mapper = getMapper();
    try {
        Attachment attachment;
        attachment.setMessageId(message_id);
        attachment.setFileName(file_name);
        attachment.setFileType(file_type);
        attachment.setFileSizeBytes(file_size_bytes);
        attachment.setS3ObjectKey(s3_object_key);
        attachment = co_await mapper.insert(attachment);
        co_return attachment;
    } catch (const DrogonDbException &e) {
        throw std::runtime_error("Database error");
    }
}

Task<vector<Attachment>> AttachmentRepository::getByMessage(int64_t message_id
) {
    auto mapper = getMapper();

    try {
        auto attachments = co_await mapper.findBy(Criteria(
            Attachment::Cols::_message_id, CompareOperator::EQ, message_id
        ));

        co_return attachments;
    } catch (const DrogonDbException &e) {
        throw std::runtime_error("Database error");
    }
}

Task<vector<Attachment>> AttachmentRepository::getByMessages(
    std::vector<int64_t> message_ids
) {
    auto mapper = getMapper();

    try {
        auto attachments = co_await mapper.findBy(Criteria(
            Attachment::Cols::_message_id, CompareOperator::In, message_ids
        ));

        co_return attachments;
    } catch (const DrogonDbException &e) {
        throw std::runtime_error("Database error");
    }
}

Task<bool> AttachmentRepository::remove(int64_t id) {
    auto mapper = getMapper();

    try {
        co_await mapper.deleteByPrimaryKey(id);
        co_return true;
    } catch (const DrogonDbException &e) {
        throw std::runtime_error("Database error");
    }
}