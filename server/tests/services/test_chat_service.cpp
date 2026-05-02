#include <drogon/HttpResponse.h>
#include <drogon/utils/coroutine.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <json/value.h>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include "dto/ChatPreview.hpp"
#include "dto/ChatServiceDtos.hpp"
#include "gtest/gtest.h"
#include "services/ChatService.hpp"
#include "tests/mocks/MockAttachmentRepository.hpp"
#include "tests/mocks/MockChatRepository.hpp"
#include "tests/mocks/MockMessageRepository.hpp"
#include "tests/mocks/MockS3Service.hpp"
#include "tests/mocks/MockUserRepository.hpp"
#include "utils/Enum.hpp"
#include "utils/server_exceptions.hpp"

using ::testing::_;
using ::testing::Invoke;

using namespace messenger::dto;
using ChatPreview = messenger::dto::ChatPreview;

template <typename T>
drogon::Task<T> createFakeTask(T data) {
    co_return data;
}

class BaseChatServiceTest : public ::testing::Test {
protected:
    std::shared_ptr<MockChatRepository> mock_chat_repo;
    std::shared_ptr<MockAttachmentRepository> mock_attachment_repo;
    std::shared_ptr<MockS3Service> mock_s3_service;

    std::shared_ptr<api::v1::ChatService> chat_service;

    void SetUp() override {
        setenv("JWT_KEY", "cool_key", 1);
        mock_attachment_repo = std::make_shared<MockAttachmentRepository>();
        mock_s3_service = std::make_shared<MockS3Service>();

        auto chat_msg_repo = std::make_unique<MockMessageRepository>(
            std::make_unique<MockAttachmentRepository>()
        );
        auto chat_usr_repo = std::make_unique<MockUserRepository>();

        mock_chat_repo = std::make_shared<MockChatRepository>(
            std::move(chat_msg_repo), std::move(chat_usr_repo)
        );
        chat_service = std::make_shared<api::v1::ChatService>();
        chat_service->setChatRepo(mock_chat_repo);
        chat_service->setAttachmentRepo(mock_attachment_repo);
        chat_service->setS3Service(mock_s3_service);
    }

    void TearDown() override {
        unsetenv("JWT_KEY");
    }
};

struct GetMessageByIdTestCase {
    std::string test_name;
    GetMessageByIdRequestDto request_dto;

    bool message_found;
    bool is_member;
    std::size_t attachments_count;
    int chat_id;
    int message_id;
};

class ServiceGetMessageByIdTest
    : public BaseChatServiceTest,
      public ::testing::WithParamInterface<GetMessageByIdTestCase> {};

TEST_P(ServiceGetMessageByIdTest, GetMessageByIdTest) {
    auto param = GetParam();
    EXPECT_CALL(*mock_chat_repo, getMessageById(param.request_dto.message_id))
        .WillRepeatedly(Invoke(
            [param](int64_t message_id
            ) -> drogon::Task<std::optional<Message>> {
                if (param.message_found) {
                    Message fake_msg;
                    fake_msg.setId(param.message_id);
                    fake_msg.setChatId(param.chat_id);
                    return createFakeTask<std::optional<Message>>(fake_msg);
                }
                return createFakeTask<std::optional<Message>>(std::nullopt);
            }
        ));
    if (param.message_found) {
        EXPECT_CALL(*mock_chat_repo, getMembers(param.chat_id))
            .WillRepeatedly(Invoke(
                [param](int64_t chat_id
                ) -> drogon::Task<std::vector<ChatMember>> {
                    std::vector<ChatMember> fake_members;
                    if (param.is_member) {
                        ChatMember fake_member;
                        fake_member.setUserId(param.request_dto.user_id);
                        fake_members.push_back(fake_member);
                    }
                    return createFakeTask<std::vector<ChatMember>>(fake_members
                    );
                }
            ));
        if (param.is_member) {
            EXPECT_CALL(*mock_attachment_repo, getByMessage(param.message_id))
                .WillRepeatedly(Invoke(
                    [param](int64_t message_id
                    ) -> drogon::Task<std::vector<Attachment>> {
                        std::vector<Attachment> attachments;
                        for (std::size_t i = 0; i < param.attachments_count;
                             i++) {
                            Attachment att;
                            att.setS3ObjectKey("key_pidor" + std::to_string(i));
                            att.setFileName("pidor" + std::to_string(i));
                            attachments.push_back(att);
                        }
                        return createFakeTask(attachments);
                    }
                ));
            EXPECT_CALL(*mock_s3_service, generateDownloadUrl(_, _))
                .WillRepeatedly(Invoke(
                    [param](
                        const std::string &s3_key, const std::string &filename
                    ) -> std::optional<std::string> {
                        return s3_key + '/' + filename;
                    }
                ));
        }
    }
    if (!param.message_found) {
        EXPECT_THROW(
            drogon::sync_wait(chat_service->getMessageById(param.request_dto)),
            messenger::exceptions::NotFoundException
        ) << "Failed test: "
          << param.test_name;
    } else if (!param.is_member) {
        EXPECT_THROW(
            drogon::sync_wait(chat_service->getMessageById(param.request_dto)),
            messenger::exceptions::ForbiddenException
        ) << "Failed test: "
          << param.test_name;
    } else {
        GetMessageByIdResponseDto response_dto;
        EXPECT_NO_THROW(
            response_dto =
                drogon::sync_wait(chat_service->getMessageById(param.request_dto
                ))
        ) << "Failed test: "
          << param.test_name;
        EXPECT_EQ(response_dto.message.getValueOfId(), param.message_id);
        EXPECT_EQ(response_dto.attachments.size(), param.attachments_count);
        EXPECT_EQ(
            response_dto.attachments_download_urls.size(),
            param.attachments_count
        );
    }
}

INSTANTIATE_TEST_SUITE_P(
    GetMessageByIdTest,
    ServiceGetMessageByIdTest,
    ::testing::Values(
        GetMessageByIdTestCase{
            "Successfully got message",
            {67, 52},
            true,
            true,
            4,
            123,
            321
        },
        GetMessageByIdTestCase{
            "Message not found",
            {67, 52},
            false,
            false,
            4,
            123,
            321
        },
        GetMessageByIdTestCase{
            "User is not member of this chat",
            {67, 52},
            true,
            false,
            4,
            123,
            321
        }
    )
);

struct GetUserChatsTestCase {
    std::string test_name;

    GetUserChatsRequestDto request_dto;

    std::size_t chats_count;
    std::set<int64_t> chats_with_last_messages;
};

class ServiceGetUserChatsTest
    : public BaseChatServiceTest,
      public ::testing::WithParamInterface<GetUserChatsTestCase> {};

TEST_P(ServiceGetUserChatsTest, GetUserChatsTest) {
    auto param = GetParam();
    if (param.request_dto.from_request_user_id ==
        param.request_dto.from_token_user_id) {
        EXPECT_CALL(
            *mock_chat_repo, getByUser(param.request_dto.from_request_user_id)
        )
            .WillRepeatedly(Invoke(
                [param](int64_t user_id
                ) -> drogon::Task<std::vector<ChatPreview>> {
                    std::vector<ChatPreview> fake_previews(param.chats_count);
                    for (int64_t el : param.chats_with_last_messages) {
                        Message fake_msg;
                        fake_msg.setId(el);
                        fake_previews[el - 1].last_message = fake_msg;
                    }
                    return createFakeTask(fake_previews);
                }
            ));
        EXPECT_CALL(*mock_attachment_repo, getByMessages(_))
            .WillRepeatedly(Invoke(
                [param](std::vector<int64_t> message_ids
                ) -> drogon::Task<std::vector<std::vector<Attachment>>> {
                    std::vector<std::vector<Attachment>> fake_attachments(
                        message_ids.size(), std::vector<Attachment>(1)
                    );
                    return createFakeTask(fake_attachments);
                }
            ));
    }

    if (param.request_dto.from_request_user_id !=
        param.request_dto.from_token_user_id) {
        EXPECT_THROW(
            drogon::sync_wait(chat_service->getUserChats(param.request_dto)),
            messenger::exceptions::ForbiddenException
        ) << "Failed test: "
          << param.test_name;
    } else {
        GetUserChatsResponseDto response_dto;
        EXPECT_NO_THROW(
            response_dto =
                drogon::sync_wait(chat_service->getUserChats(param.request_dto))
        ) << "Failed test: "
          << param.test_name;
        EXPECT_EQ(
            response_dto.chats_previews.size(),
            response_dto.last_message_attachments.size()
        ) << "Failed test: "
          << param.test_name;
        for (std::size_t i = 0; i < param.chats_count; i++) {
            if (param.chats_with_last_messages.contains(i + 1)) {
                EXPECT_FALSE(response_dto.last_message_attachments[i].empty())
                    << "Failed test: " << param.test_name;
            } else {
                EXPECT_TRUE(response_dto.last_message_attachments[i].empty())
                    << "Failed test: " << param.test_name;
            }
        }
    }
}

INSTANTIATE_TEST_SUITE_P(
    GetUserChatsTest,
    ServiceGetUserChatsTest,
    ::testing::Values(
        GetUserChatsTestCase{
            "Success - user doesn't have chats",
            {67, 67},
            0,
            {}
        },
        GetUserChatsTestCase{
            "Success - user has chats. Some has last message with attachment",
            {67, 67},
            10,
            {3, 4, 7, 10}
        },
        GetUserChatsTestCase{"Access denied", {67, 52}, 1, {}}
    )
);

struct CreateOrGetDirectChatTestCase {
    std::string test_name;

    CreateOrGetDirectRequestDto request_dto;

    bool chat_exists;
};

class ServiceCreateOrGetDirectChatTest
    : public BaseChatServiceTest,
      public ::testing::WithParamInterface<CreateOrGetDirectChatTestCase> {};

TEST_P(ServiceCreateOrGetDirectChatTest, CreateOrGetDirectChatTest) {
    auto param = GetParam();

    if (param.request_dto.target_user_id == param.request_dto.user_id) {
        EXPECT_CALL(*mock_chat_repo, getSaved(param.request_dto.user_id))
            .WillRepeatedly(
                Invoke([param](int64_t user_id) -> drogon::Task<Chat> {
                    Chat fake_chat;
                    fake_chat.setType(messenger::models::ChatType::Saved);
                    return createFakeTask(fake_chat);
                })
            );
    } else {
        EXPECT_CALL(
            *mock_chat_repo,
            getDirect(
                param.request_dto.user_id, param.request_dto.target_user_id
            )
        )
            .WillRepeatedly(Invoke(
                [param](
                    int64_t user_id, int64_t other_user_id
                ) -> drogon::Task<std::optional<Chat>> {
                    if (param.chat_exists) {
                        Chat fake_chat;
                        fake_chat.setType(messenger::models::ChatType::Direct);
                        return createFakeTask<std::optional<Chat>>(fake_chat);
                    }
                    return createFakeTask<std::optional<Chat>>(std::nullopt);
                }
            ));
    }
    if (!param.chat_exists) {
        EXPECT_CALL(
            *mock_chat_repo,
            getOrCreateDirect(
                param.request_dto.user_id, param.request_dto.target_user_id, _
            )
        )
            .WillRepeatedly(Invoke(
                [param](
                    int64_t user_id, int64_t other_user_id, ...
                ) -> drogon::Task<Chat> {
                    Chat fake_chat;
                    fake_chat.setType(messenger::models::ChatType::Direct);
                    return createFakeTask(fake_chat);
                }
            ));
    }

    CreateOrGetDirectResponseDto response_dto =
        drogon::sync_wait(chat_service->createOrGetDirectChat(param.request_dto)
        );
    EXPECT_EQ(!param.chat_exists, response_dto.was_created)
        << "Failed test: " << param.test_name;
    if (param.request_dto.target_user_id == param.request_dto.user_id) {
        EXPECT_EQ(
            response_dto.chat.getValueOfType(),
            messenger::models::ChatType::Saved
        ) << "Failed test: "
          << param.test_name;
    } else {
        EXPECT_EQ(
            response_dto.chat.getValueOfType(),
            messenger::models::ChatType::Direct
        ) << "Failed test: "
          << param.test_name;
    }
}

INSTANTIATE_TEST_SUITE_P(
    CreateOrGetDirectChatTest,
    ServiceCreateOrGetDirectChatTest,
    ::testing::Values(
        CreateOrGetDirectChatTestCase{
            "Getting chat with yourself",
            {67, 67},
            true
        },
        CreateOrGetDirectChatTestCase{"Getting existed chat", {67, 52}, true},
        CreateOrGetDirectChatTestCase{
            "Chat doesn't exist -> creating",
            {67, 52},
            false
        }
    )
);

// struct ***TestCase {
//     std::string test_name;

//     ***RequestDto request_dto;
// };

// class Service***Test
//     : public BaseChatServiceTest,
//       public ::testing::WithParamInterface<***TestCase> {};

// TEST_P(Service***Test, ***Test) {
//     auto param = GetParam();

//         EXPECT_CALL(
//             *mock_chat_repo,
//             getByUser(param.request_dto.from_request_user_id)
//         )
//             .WillRepeatedly(
//                 [param](
//                     int64_t user_id
//                 ) -> drogon::Task<std::vector<ChatPreview>> {
//                     std::vector<ChatPreview>
//                     fake_previews(param.chats_count); for (int64_t el :
//                     param.chats_with_last_messages) {
//                         Message fake_msg;
//                         fake_msg.setId(el);
//                         fake_previews[el - 1].last_message = fake_msg;
//                     }
//                     return createFakeTask(fake_previews);
//                 }
//             );

//     if (param.request_dto.from_request_user_id !=
//         param.request_dto.from_token_user_id) {
//         EXPECT_THROW(
//             drogon::sync_wait(chat_service->getUserChats(param.request_dto)),
//             messenger::exceptions::ForbiddenException
//         ) << "Failed test: "
//           << param.test_name;
//     } else {
//         GetUserChatsResponseDto response_dto;
//         EXPECT_NO_THROW(
//             response_dto =
//                 drogon::sync_wait(chat_service->getUserChats(param.request_dto))
//         );
//         EXPECT_EQ(response_dto.chats_previews.size(),
//         response_dto.last_message_attachments.size()); for (std::size_t i =
//         0; i < param.chats_count; i++){
//             if (param.chats_with_last_messages.contains(i + 1)){
//                 EXPECT_FALSE(response_dto.last_message_attachments[i].empty());
//             }
//             else{
//                 EXPECT_TRUE(response_dto.last_message_attachments[i].empty());
//             }
//         }
//     }
// }

// INSTANTIATE_TEST_SUITE_P(
//     ***Test,
//     Service***Test,
//     ::testing::Values(
//         ***TestCase{
//         }
//     )
// );