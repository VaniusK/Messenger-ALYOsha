#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>
#include "services/ChatService.hpp"
#include "tests/mocks/MockAttachmentRepository.hpp"
#include "tests/mocks/MockChatRepository.hpp"
#include "tests/mocks/MockMessageRepository.hpp"
#include "tests/mocks/MockUserRepository.hpp"

using ::testing::_;
using ::testing::Invoke;
using ::testing::Return;

template <typename T>
drogon::Task<T> createFakeTask(T data) {
    co_return data;
}

// struct CheckChatAccessTestCase {
//     std::string test_name;
//     int user_id;

//     bool is_member;
// };

// class ServiceCheckChatAccessTest
//     : public ::testing::TestWithParam<CheckChatAccessTestCase> {
// protected:
//     std::shared_ptr<MockChatRepository> mock_chat_repo;
//     std::shared_ptr<MockAttachmentRepository> mock_attachment_repo;

//     std::shared_ptr<api::v1::ChatService> chat_service;

//     void SetUp() override {
//         mock_attachment_repo = std::make_shared<MockAttachmentRepository>();

//         auto chat_msg_repo = std::make_unique<MockMessageRepository>();
//         auto chat_usr_repo = std::make_unique<MockUserRepository>();

//         mock_chat_repo = std::make_shared<MockChatRepository>(
//             std::move(chat_msg_repo), std::move(chat_usr_repo)
//         );
//         chat_service->setChatRepo(mock_chat_repo);
//         chat_service->setAttachmentRepo(mock_attachment_repo);
//     }
// };