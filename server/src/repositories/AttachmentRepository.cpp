#include "repositories/AttachmentRepository.hpp"
#include <drogon/orm/Criteria.h>
#include <map>
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
    std::string s3_object_key,
    std::shared_ptr<drogon::orm::Transaction> transaction_ptr
) {
    auto mapper = getMapper(transaction_ptr);
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

Task<vector<vector<Attachment>>> AttachmentRepository::getByMessages(
    std::vector<int64_t> message_ids
) {
    auto mapper = getMapper();

    try {
        auto attachments = co_await mapper.findBy(Criteria(
            Attachment::Cols::_message_id, CompareOperator::In, message_ids
        ));
        vector<vector<Attachment>> attachments_by_message(message_ids.size());
        map<int, int> message_id_to_vector_id;
        for (int i = 0; i < message_ids.size(); i++) {
            message_id_to_vector_id[message_ids[i]] = i;
        }
        for (auto &attachment : attachments) {
            attachments_by_message
                [message_id_to_vector_id[attachment.getValueOfMessageId()]]
                    .push_back(std::move(attachment));
        }

        co_return attachments_by_message;
    } catch (const DrogonDbException &e) {
        throw std::runtime_error("Database error");
    }
}

Task<bool> AttachmentRepository::remove(
    int64_t id,
    std::shared_ptr<drogon::orm::Transaction> transaction_ptr
) {
    auto mapper = getMapper(transaction_ptr);

    try {
        co_await mapper.deleteByPrimaryKey(id);
        co_return true;
    } catch (const DrogonDbException &e) {
        throw std::runtime_error("Database error");
    }
}