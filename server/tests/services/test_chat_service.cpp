#include <drogon/HttpResponse.h>
#include <drogon/utils/coroutine.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <json/value.h>
#include <sys/select.h>
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
#include "jwt-cpp/jwt.h"
#include "services/ChatService.hpp"
#include "services/S3Service.hpp"
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

drogon::Task<void> createFakeVoidTask() {
    co_return;
}

class BaseChatServiceTest : public ::testing::Test {
protected:
    std::shared_ptr<MockChatRepository> mock_chat_repo;
    std::shared_ptr<MockAttachmentRepository> mock_attachment_repo;
    std::shared_ptr<MockS3Service> mock_s3_service;
    std::shared_ptr<MockUserRepository> mock_user_repo;

    std::shared_ptr<api::v1::ChatService> chat_service;

    void SetUp() override {
        setenv("JWT_KEY", "cool_key", 1);
        mock_attachment_repo = std::make_shared<MockAttachmentRepository>();
        mock_s3_service = std::make_shared<MockS3Service>();
        mock_user_repo = std::make_shared<MockUserRepository>();

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
        chat_service->setUserRepo(mock_user_repo);
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
                    fake_msg.setSenderId(777);
                    return createFakeTask<std::optional<Message>>(fake_msg);
                }
                return createFakeTask<std::optional<Message>>(std::nullopt);
            }
        ));
    if (param.message_found) {
        EXPECT_CALL(*mock_chat_repo, getMembers(param.chat_id, _))
            .WillRepeatedly(Invoke(
                [param](
                    int64_t chat_id,
                    std::shared_ptr<drogon::orm::Transaction> transaction_ptr
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
            EXPECT_CALL(*mock_user_repo, getById(777))
                .WillRepeatedly(
                    Invoke([](int64_t id) -> drogon::Task<std::optional<User>> {
                        User fake_user;
                        fake_user.setId(id);
                        fake_user.setDisplayName("Test Sender");
                        return createFakeTask<std::optional<User>>(fake_user);
                    })
                );
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
    bool has_duplicate_senders;
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
                        if (param.has_duplicate_senders) {
                            fake_msg.setSenderId(1);
                        } else {
                            fake_msg.setSenderId(el * 10);
                        }
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
        EXPECT_CALL(*mock_user_repo, getByIds(_))
            .WillRepeatedly(Invoke(
                [param](std::vector<int64_t> ids
                ) -> drogon::Task<std::vector<User>> {
                    if (param.has_duplicate_senders &&
                        param.chats_with_last_messages.size() > 0) {
                        EXPECT_EQ(ids.size(), 1);
                    }
                    std::vector<User> fake_users;
                    for (auto id : ids) {
                        User fake_user;
                        fake_user.setId(id);
                        fake_user.setDisplayName("User_" + std::to_string(id));
                        fake_users.push_back(fake_user);
                    }
                    return createFakeTask(fake_users);
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
            response_dto.last_messages_senders.size()
        ) << "Failed test: "
          << param.test_name;
        EXPECT_EQ(
            response_dto.chats_previews.size(),
            response_dto.last_messages_attachments.size()
        ) << "Failed test: "
          << param.test_name;
        for (std::size_t i = 0; i < param.chats_count; i++) {
            if (param.chats_with_last_messages.contains(i + 1)) {
                EXPECT_FALSE(response_dto.last_messages_attachments[i].empty())
                    << "Failed test: " << param.test_name;
            } else {
                EXPECT_TRUE(response_dto.last_messages_attachments[i].empty())
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
            {},
            false
        },
        GetUserChatsTestCase{
            "Success - user has chats. Some has last message with attachment",
            {67, 67},
            10,
            {3, 4, 7, 10},
            false
        },
        GetUserChatsTestCase{"Access denied", {67, 52}, 1, {}, false},
        GetUserChatsTestCase{
            "Success with duplicate sender_ids in last messages",
            {67, 67},
            5,
            {1, 2, 3, 4, 5},
            true
        }
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

struct SendMessageTestCase {
    std::string test_name;

    SendMessageRequestDto request_dto;

    int64_t message_id;
    bool is_member;
    bool jwt_key_set;
    bool is_token_valid;
    bool is_type_match;
};

class ServiceSendMessageTest
    : public BaseChatServiceTest,
      public ::testing::WithParamInterface<SendMessageTestCase> {
public:
    static std::string make_token(
        std::string message_type,
        std::string file_name,
        std::string file_type,
        std::string file_size_bytes,
        std::string attachment_key
    ) {
        const std::string jwt_key = "cool_key";
        auto token =
            jwt::create()
                .set_issuer("alesha_messenger")
                .set_type("JWT")
                .set_issued_at(std::chrono::system_clock::now())
                .set_expires_at(
                    std::chrono::system_clock::now() + std::chrono::hours(2)
                )
                .set_payload_claim("object_key", jwt::claim(attachment_key))
                .set_payload_claim("file_type", jwt::claim(file_type))
                .set_payload_claim("file_name", jwt::claim(file_name))
                .set_payload_claim(
                    "file_size_bytes", jwt::claim(file_size_bytes)
                )
                .set_payload_claim("message_type", jwt::claim(message_type))
                .sign(jwt::algorithm::hs256{jwt_key});
        return token;
    }
};

TEST_P(ServiceSendMessageTest, SendMessageTest) {
    auto param = GetParam();

    EXPECT_CALL(*mock_chat_repo, getMembers(param.request_dto.chat_id, _))
        .WillRepeatedly(
            [param](
                int64_t chat_id,
                std::shared_ptr<drogon::orm::Transaction> transaction_ptr
            ) -> drogon::Task<std::vector<ChatMember>> {
                std::vector<ChatMember> fake_members;
                if (param.is_member) {
                    ChatMember fake_member;
                    fake_member.setUserId(param.request_dto.user_id);
                    fake_members.push_back(fake_member);
                }
                return createFakeTask(fake_members);
            }
        );

    if (param.is_member && param.is_token_valid && param.is_type_match &&
        param.jwt_key_set) {
        EXPECT_CALL(
            *mock_chat_repo,
            sendMessage(
                param.request_dto.chat_id, param.request_dto.user_id,
                param.request_dto.text, param.request_dto.reply_to_id,
                param.request_dto.forward_from_id,
                param.request_dto.message_type, _, _
            )
        )
            .WillRepeatedly(
                [param](
                    int64_t chat_id, int64_t sender_id, std::string text,
                    std::optional<int64_t> reply_to_id,
                    std::optional<int64_t> forwarded_from_id, std::string type,
                    std::vector<messenger::dto::AttachmentData> attachments,
                    std::shared_ptr<drogon::orm::Transaction> transaction_ptr
                ) -> drogon::Task<std::pair<Message, std::vector<Attachment>>> {
                    Message fake_message;
                    fake_message.setChatId(param.request_dto.chat_id);
                    fake_message.setText(param.request_dto.text);
                    fake_message.setId(param.message_id);
                    std::vector<Attachment> fake_attachments;
                    for (auto att : attachments) {
                        Attachment fake_attachment;
                        fake_attachment.setFileName(att.file_name);
                        fake_attachment.setFileSizeBytes(att.file_size_bytes);
                        fake_attachment.setFileType(att.file_type);
                        fake_attachment.setMessageId(fake_message.getValueOfId()
                        );
                        fake_attachment.setS3ObjectKey(att.s3_object_key);
                        fake_attachments.push_back(fake_attachment);
                    }
                    return createFakeTask<
                        std::pair<Message, std::vector<Attachment>>>(
                        {fake_message, fake_attachments}
                    );
                }
            );

        EXPECT_CALL(*mock_s3_service, generateDownloadUrl(_, _))
            .WillRepeatedly(Invoke(
                [param](
                    const std::string &s3_key, const std::string &filename
                ) -> std::optional<std::string> { return s3_key + "/"; }
            ));
        EXPECT_CALL(
            *mock_chat_repo, markAsRead(
                                 param.request_dto.chat_id,
                                 param.request_dto.user_id, param.message_id, _
                             )
        )
            .WillRepeatedly(
                [param](
                    int64_t chat_id, int64_t user_id,
                    int64_t last_read_message_id,
                    std::shared_ptr<drogon::orm::Transaction> transaction_ptr
                ) -> drogon::Task<bool> { return createFakeTask(true); }
            );
        EXPECT_CALL(*mock_user_repo, getById(param.request_dto.user_id))
            .WillRepeatedly(
                [param](int64_t user_id) -> drogon::Task<std::optional<User>> {
                    User fake_user;
                    fake_user.setId(param.request_dto.user_id);
                    fake_user.setHandle("Pidorok");
                    return createFakeTask<std::optional<User>>(fake_user);
                }
            );
    }

    if (!param.is_member) {
        EXPECT_THROW(
            drogon::sync_wait(chat_service->sendMessage(param.request_dto)),
            messenger::exceptions::ForbiddenException
        ) << "Failed test: "
          << param.test_name;
    } else if (!param.jwt_key_set) {
        unsetenv("JWT_KEY");
        EXPECT_THROW(
            drogon::sync_wait(chat_service->sendMessage(param.request_dto)),
            messenger::exceptions::InternalServerErrorException
        ) << "Failed test: "
          << param.test_name;
        setenv("JWT_KEY", "cool_key", 1);
    } else if (!param.is_token_valid) {
        EXPECT_THROW(
            drogon::sync_wait(chat_service->sendMessage(param.request_dto)),
            messenger::exceptions::BadRequestException
        ) << "Failed test: "
          << param.test_name;
    } else if (!param.is_type_match) {
        EXPECT_THROW(
            drogon::sync_wait(chat_service->sendMessage(param.request_dto)),
            messenger::exceptions::BadRequestException
        ) << "Failed test: "
          << param.test_name;
    } else {
        SendMessageResponseDto response_dto;
        EXPECT_NO_THROW(
            response_dto =
                drogon::sync_wait(chat_service->sendMessage(param.request_dto))
        ) << "Failed test: "
          << param.test_name;
        EXPECT_EQ(response_dto.message.getValueOfId(), param.message_id)
            << "Failed test: " << param.test_name;
        EXPECT_EQ(
            response_dto.message.getValueOfChatId(), param.request_dto.chat_id
        ) << "Failed test: "
          << param.test_name;
        EXPECT_EQ(response_dto.message.getValueOfText(), param.request_dto.text)
            << "Failed test: " << param.test_name;
        EXPECT_EQ(
            response_dto.attachments.size(),
            param.request_dto.attachment_tokens.size()
        ) << "Failed test: "
          << param.test_name;
        EXPECT_EQ(
            response_dto.attachments.size(),
            response_dto.attachments_download_urls.size()
        ) << "Failed test: "
          << param.test_name;
        for (std::size_t i = 0; i < response_dto.attachments.size(); i++) {
            EXPECT_EQ(
                response_dto.attachments[i].getValueOfS3ObjectKey() + "/",
                response_dto.attachments_download_urls[i]
            ) << "Failed test: "
              << param.test_name;
        }
        EXPECT_EQ(
            response_dto.sender_info.getValueOfId(), param.request_dto.user_id
        );
        EXPECT_EQ(response_dto.sender_info.getValueOfHandle(), "Pidorok");
    }
}

INSTANTIATE_TEST_SUITE_P(
    SendMessageTest,
    ServiceSendMessageTest,
    ::testing::Values(
        SendMessageTestCase{
            "User is not in chat",
            {67, 69, "AAAA", "Text", std::nullopt, std::nullopt, {}},
            52,
            false,
            false,
            false,
            false
        },
        SendMessageTestCase{
            "JWT_KEY is not set",
            {67,
             69,
             "AAAA",
             "Text",
             std::nullopt,
             std::nullopt,
             {"bebebebebebe"}},
            52,
            true,
            false,
            false,
            false
        },
        SendMessageTestCase{
            "Invalid token",
            {67,
             69,
             "AAAA",
             "Text",
             std::nullopt,
             std::nullopt,
             {ServiceSendMessageTest::
                  make_token("Text", "A.png", "image/png", "228", "funny_key"),
              "bebebebe_bad_token"}},
            52,
            true,
            true,
            false,
            false
        },
        SendMessageTestCase{
            "Types mismatch",
            {67,
             69,
             "AAAA",
             "Text",
             std::nullopt,
             std::nullopt,
             {ServiceSendMessageTest::
                  make_token("Text", "A.png", "image/png", "228", "funny_key"),
              ServiceSendMessageTest::
                  make_token("Voice", "A.png", "image/png", "228", "funny_key")}
            },
            52,
            true,
            true,
            true,
            false
        },
        SendMessageTestCase{
            "Success: without attachments",
            {67, 69, "AAAA", "Text", std::nullopt, std::nullopt, {}},
            52,
            true,
            true,
            true,
            true
        },
        SendMessageTestCase{
            "Successful: with attachments",
            {67,
             69,
             "AAAA",
             "Text",
             std::nullopt,
             std::nullopt,
             {ServiceSendMessageTest::
                  make_token("Text", "A.png", "image/png", "228", "funny_key1"),
              ServiceSendMessageTest::
                  make_token("Text", "B.png", "image/png", "228", "funny_key2")}
            },
            52,
            true,
            true,
            true,
            true
        }
    )
);

struct ReadMessagesTestCase {
    std::string test_name;

    ReadMessagesRequestDto request_dto;
    bool is_successful;
};

class ServiceReadMessagesTest
    : public BaseChatServiceTest,
      public ::testing::WithParamInterface<ReadMessagesTestCase> {};

TEST_P(ServiceReadMessagesTest, ReadMessagesTest) {
    auto param = GetParam();

    EXPECT_CALL(
        *mock_chat_repo,
        markAsRead(
            param.request_dto.chat_id, param.request_dto.user_id,
            param.request_dto.last_read_message_id, _
        )
    )
        .WillRepeatedly(
            [param](
                int64_t chat_id, int64_t user_id, int64_t last_read_message_id,
                std::shared_ptr<drogon::orm::Transaction> transaction_ptr
            ) -> drogon::Task<bool> {
                return createFakeTask(param.is_successful);
            }
        );

    ReadMessagesResponseDto response_dto;
    EXPECT_NO_THROW(
        response_dto =
            drogon::sync_wait(chat_service->readMessages(param.request_dto))
    ) << "Failed test: "
      << param.test_name;
    EXPECT_EQ(response_dto.success, param.is_successful)
        << "Failed test: " << param.test_name;
}

INSTANTIATE_TEST_SUITE_P(
    ReadMessagesTest,
    ServiceReadMessagesTest,
    ::testing::Values(
        ReadMessagesTestCase{"Success", {67, 69, 52}, true},
        ReadMessagesTestCase{"Fail: message wasn't read", {67, 69, 52}, false}
    )
);

struct GetChatMessagesTestCase {
    std::string test_name;

    GetChatMessagesRequestDto request_dto;
    bool is_member;
    bool has_duplicate_senders;
};

class ServiceGetChatMessagesTest
    : public BaseChatServiceTest,
      public ::testing::WithParamInterface<GetChatMessagesTestCase> {};

TEST_P(ServiceGetChatMessagesTest, GetChatMessagesTest) {
    auto param = GetParam();
    EXPECT_CALL(*mock_chat_repo, getMembers(param.request_dto.chat_id, _))
        .WillRepeatedly(
            [param](
                int64_t chat_id,
                std::shared_ptr<drogon::orm::Transaction> transaction_ptr
            ) -> drogon::Task<std::vector<ChatMember>> {
                std::vector<ChatMember> fake_members;
                if (param.is_member) {
                    ChatMember fake_member;
                    fake_member.setUserId(param.request_dto.user_id);
                    fake_members.push_back(fake_member);
                }
                return createFakeTask(fake_members);
            }
        );

    if (param.is_member) {
        EXPECT_CALL(
            *mock_chat_repo,
            getMessagesByChat(
                param.request_dto.chat_id, param.request_dto.before_message_id,
                param.request_dto.limit
            )
        )
            .WillRepeatedly(
                [param](
                    int64_t chat_id, std::optional<int64_t> before_message_id,
                    int64_t limit
                ) -> drogon::Task<std::vector<Message>> {
                    std::vector<Message> fake_messages;
                    for (int64_t i = 1; i <= limit; i++) {
                        Message fake_message;
                        fake_message.setId(i);
                        fake_message.setChatId(chat_id);
                        if (param.has_duplicate_senders) {
                            fake_message.setSenderId((i % 2 == 0) ? 2 : 1);
                        } else {
                            fake_message.setSenderId(i);
                        }
                        fake_messages.push_back(fake_message);
                    }
                    return createFakeTask(fake_messages);
                }
            );
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
        EXPECT_CALL(*mock_s3_service, generateDownloadUrl(_, _))
            .WillRepeatedly(Invoke(
                [param](
                    const std::string &s3_key, const std::string &filename
                ) -> std::optional<std::string> {
                    return s3_key + '/' + filename;
                }
            ));
        EXPECT_CALL(*mock_user_repo, getByIds(_))
            .WillRepeatedly(Invoke(
                [param](std::vector<int64_t> ids
                ) -> drogon::Task<std::vector<User>> {
                    if (param.has_duplicate_senders &&
                        param.request_dto.limit > 2) {
                        EXPECT_EQ(ids.size(), 2);
                    } else {
                        EXPECT_EQ(ids.size(), param.request_dto.limit);
                    }
                    std::vector<User> fake_users;
                    for (auto id : ids) {
                        User fake_user;
                        fake_user.setId(id);
                        fake_user.setDisplayName("User_" + std::to_string(id));
                        fake_users.push_back(fake_user);
                    }
                    return createFakeTask(fake_users);
                }
            ));
    }

    if (!param.is_member) {
        EXPECT_THROW(
            drogon::sync_wait(chat_service->getChatMessages(param.request_dto)),
            messenger::exceptions::ForbiddenException
        ) << "Failed test: "
          << param.test_name;
    } else {
        GetChatMessagesResponseDto response_dto;
        EXPECT_NO_THROW(
            response_dto = drogon::sync_wait(
                chat_service->getChatMessages(param.request_dto)
            )
        ) << "Failed test: "
          << param.test_name;
        EXPECT_EQ(response_dto.messages.size(), param.request_dto.limit)
            << "Failed test: " << param.test_name;
        EXPECT_EQ(response_dto.messages.size(), response_dto.attachments.size())
            << "Failed test: " << param.test_name;
        EXPECT_EQ(response_dto.attachments_download_urls[0].size(), 1)
            << "Failed test: " << param.test_name;
        EXPECT_EQ(
            response_dto.messages.size(), response_dto.senders_info.size()
        ) << "Failed test: "
          << param.test_name;
    }
}

INSTANTIATE_TEST_SUITE_P(
    GetChatMessagesTest,
    ServiceGetChatMessagesTest,
    ::testing::Values(
        GetChatMessagesTestCase{
            "User is not in chat",
            {67, 69, std::nullopt, 42},
            false,
            false
        },
        GetChatMessagesTestCase{
            "Success",
            {67, 69, std::nullopt, 42},
            true,
            false
        },
        // НОВЫЙ ТЕСТ НА ДУБЛИКАТЫ:
        GetChatMessagesTestCase{
            "Success with duplicate sender_ids",
            {67, 69, std::nullopt, 10},
            true,
            true
        }
    )
);

struct GetAttachmentLinksTestCase {
    std::string test_name;

    GetAttachmentLinksRequestDto request_dto;
    bool is_member;
    bool is_type_correct;
    bool is_links_generating_success;
    bool is_jwt_key_set;
};

class ServiceGetAttachmentLinksTest
    : public BaseChatServiceTest,
      public ::testing::WithParamInterface<GetAttachmentLinksTestCase> {};

TEST_P(ServiceGetAttachmentLinksTest, GetAttachmentLinksTest) {
    auto param = GetParam();

    EXPECT_CALL(*mock_chat_repo, getMembers(param.request_dto.chat_id, _))
        .WillRepeatedly(
            [param](
                int64_t chat_id,
                std::shared_ptr<drogon::orm::Transaction> transaction_ptr
            ) -> drogon::Task<std::vector<ChatMember>> {
                std::vector<ChatMember> fake_members;
                if (param.is_member) {
                    ChatMember fake_member;
                    fake_member.setUserId(param.request_dto.user_id);
                    fake_members.push_back(fake_member);
                }
                return createFakeTask(fake_members);
            }
        );
    if (param.is_member && param.is_type_correct) {
        EXPECT_CALL(*mock_s3_service, getExtension(_))
            .WillRepeatedly(Invoke([](const std::string &filename) {
                return filename.substr(filename.length() - 3);
            }));

        EXPECT_CALL(*mock_s3_service, getMimeType(_))
            .WillRepeatedly(Invoke([](const std::string &ext) {
                if (ext == "png") {
                    return std::string("image/png");
                }
                if (ext == "mp4") {
                    return std::string("video/mp4");
                }
                return std::string("application/octet-stream");
            }));
        EXPECT_CALL(
            *mock_s3_service,
            generateUploadUrl(
                param.request_dto.chat_id, param.request_dto.message_type, _
            )
        )
            .WillRepeatedly(
                [param](
                    int64_t chat_id, std::string message_type,
                    std::vector<AttachmentFileInfo> files_info
                ) -> std::optional<std::vector<UploadPresignedResult>> {
                    if (!param.is_links_generating_success) {
                        return std::nullopt;
                    }
                    std::vector<UploadPresignedResult> fake_res(files_info.size(
                    ));
                    for (std::size_t i = 0; i < files_info.size(); i++) {
                        UploadPresignedResult fake_att;
                        fake_att.file_size_bytes =
                            files_info[i].file_size_bytes;
                        fake_att.file_name = files_info[i].file_name;
                        fake_att.content_type = files_info[i].mime_type;
                        fake_att.attachment_key = "test_key/" +
                                                  std::to_string(chat_id) +
                                                  "/" + files_info[i].file_name;
                        fake_att.upload_url =
                            "test_url/download/" + fake_att.attachment_key;
                        fake_res[i] = std::move(fake_att);
                    }
                    return fake_res;
                }
            );
    }

    if (!param.is_member) {
        EXPECT_THROW(
            drogon::sync_wait(chat_service->getAttachmentLinks(param.request_dto
            )),
            messenger::exceptions::ForbiddenException
        );
    } else if (!param.is_type_correct) {
        EXPECT_THROW(
            drogon::sync_wait(chat_service->getAttachmentLinks(param.request_dto
            )),
            messenger::exceptions::BadRequestException
        );
    } else if (!param.is_links_generating_success) {
        EXPECT_THROW(
            drogon::sync_wait(chat_service->getAttachmentLinks(param.request_dto
            )),
            messenger::exceptions::InternalServerErrorException
        );
    } else if (!param.is_jwt_key_set) {
        unsetenv("JWT_KEY");
        EXPECT_THROW(
            drogon::sync_wait(chat_service->getAttachmentLinks(param.request_dto
            )),
            messenger::exceptions::InternalServerErrorException
        );
        setenv("JWT_KEY", "cool_key", 1);
    } else {
        GetAttachmentLinksResponseDto response_dto;
        EXPECT_NO_THROW(
            response_dto = drogon::sync_wait(
                chat_service->getAttachmentLinks(param.request_dto)
            )
        );
        EXPECT_EQ(response_dto.attachments.size(), response_dto.tokens.size());
    }
}

INSTANTIATE_TEST_SUITE_P(
    GetAttachmentLinksTest,
    ServiceGetAttachmentLinksTest,
    ::testing::Values(
        GetAttachmentLinksTestCase{
            "User is not in chat",
            {67, 69, "Text", {}},
            false,
            false,
            false,
            false
        },
        GetAttachmentLinksTestCase{
            "Types mismatch : voice",
            {67,
             69,
             messenger::models::MessageType::Voice,
             {{"badfile.exe", 228}}},
            true,
            false,
            false,
            false
        },
        GetAttachmentLinksTestCase{
            "Types mismatch : media",
            {67,
             69,
             messenger::models::MessageType::Media,
             {{"goodfile1.png", 228},
              {"goodfile2.mp4", 228},
              {"badfile.exe", 132}}},
            true,
            false,
            false,
            false
        },
        GetAttachmentLinksTestCase{
            "Failed to generate links",
            {67,
             69,
             messenger::models::MessageType::Text,
             {{"goodfile1.png", 228},
              {"goodfile2.mp4", 228},
              {"goodfile3.exe", 132}}},
            true,
            true,
            false,
            false
        },
        GetAttachmentLinksTestCase{
            "JWT_KEY is not set",
            {67,
             69,
             messenger::models::MessageType::Text,
             {{"goodfile1.png", 228},
              {"goodfile2.mp4", 228},
              {"goodfile3.exe", 132}}},
            true,
            true,
            true,
            false
        },
        GetAttachmentLinksTestCase{
            "Success",
            {67,
             69,
             messenger::models::MessageType::Text,
             {{"goodfile1.png", 228},
              {"goodfile2.mp4", 228},
              {"goodfile3.exe", 132}}},
            true,
            true,
            true,
            true
        }
    )
);

struct CreateGroupTestCase {
    std::string test_name;
    CreateGroupRequestDto request_dto;
    bool should_succeed;
};

class ServiceCreateGroupTest
    : public BaseChatServiceTest,
      public ::testing::WithParamInterface<CreateGroupTestCase> {};

TEST_P(ServiceCreateGroupTest, CreateGroupTest) {
    auto param = GetParam();

    if (param.should_succeed) {
        EXPECT_CALL(
            *mock_chat_repo,
            createGroup(
                param.request_dto.name, param.request_dto.creator_id,
                param.request_dto.members_ids,
                _  // транзакция по умолчанию nullptr
            )
        )
            .WillRepeatedly(Invoke(
                [](std::string name, int64_t creator_id,
                   std::vector<int64_t> member_ids,
                   auto transaction_ptr) -> drogon::Task<Chat> {
                    Chat fake_chat;
                    fake_chat.setId(42);
                    fake_chat.setName(name);
                    fake_chat.setType(messenger::models::ChatType::Group);
                    return createFakeTask(fake_chat);
                }
            ));
    }

    if (!param.should_succeed) {
        EXPECT_THROW(
            drogon::sync_wait(chat_service->createGroup(param.request_dto)),
            messenger::exceptions::BadRequestException
        ) << "Failed test: "
          << param.test_name;
    } else {
        CreateGroupResponseDto response_dto;
        EXPECT_NO_THROW(
            response_dto =
                drogon::sync_wait(chat_service->createGroup(param.request_dto))
        ) << "Failed test: "
          << param.test_name;

        EXPECT_EQ(response_dto.chat.getValueOfId(), 42);
        EXPECT_EQ(response_dto.chat.getValueOfName(), param.request_dto.name);
    }
}

INSTANTIATE_TEST_SUITE_P(
    CreateGroupTest,
    ServiceCreateGroupTest,
    ::testing::Values(
        CreateGroupTestCase{
            "Success: Creator is in members list",
            CreateGroupRequestDto(10, "C++ Chat", {10, 20, 30}), true
        },
        CreateGroupTestCase{
            "Fail: Creator is missing from members list",
            CreateGroupRequestDto(10, "Hackers", {20, 30}), false
        }
    )
);

enum class RemoveMemberExpectedResult {
    Success,
    Forbidden,
    Conflict,
    NotFound
};

struct RemoveMemberTestCase {
    std::string test_name;
    RemoveMemberRequestDto request_dto;

    std::string source_role;
    std::string target_role;
    bool is_target_in_chat;
    int total_members;

    RemoveMemberExpectedResult expected_result;
};

class ServiceRemoveMemberTest
    : public BaseChatServiceTest,
      public ::testing::WithParamInterface<RemoveMemberTestCase> {};

TEST_P(ServiceRemoveMemberTest, RemoveMemberTest) {
    auto param = GetParam();

    EXPECT_CALL(
        *mock_chat_repo,
        getMember(param.request_dto.chat_id, param.request_dto.user_id)
    )
        .WillRepeatedly(Invoke(
            [param](int64_t c_id, int64_t u_id) -> drogon::Task<ChatMember> {
                ChatMember fake_source;
                fake_source.setChatId(c_id);
                fake_source.setUserId(u_id);
                fake_source.setRole(param.source_role);
                return createFakeTask(fake_source);
            }
        ));

    EXPECT_CALL(*mock_chat_repo, getMembers(param.request_dto.chat_id, _))
        .WillRepeatedly(Invoke(
            [param](
                int64_t c_id, auto t_ptr
            ) -> drogon::Task<std::vector<ChatMember>> {
                std::vector<ChatMember> members;
                ChatMember source;
                source.setUserId(param.request_dto.user_id);
                source.setRole(param.source_role);
                members.push_back(source);

                if (param.request_dto.user_id != param.request_dto.member_id &&
                    param.is_target_in_chat) {
                    ChatMember target;
                    target.setUserId(param.request_dto.member_id);
                    target.setRole(param.target_role);
                    members.push_back(target);
                }

                int current_id = 1000;
                while (members.size() < param.total_members) {
                    ChatMember extra;
                    extra.setUserId(current_id++);
                    extra.setRole(messenger::models::ChatRole::Member);
                    members.push_back(extra);
                }
                return createFakeTask(members);
            }
        ));

    if (param.expected_result == RemoveMemberExpectedResult::Success) {
        EXPECT_CALL(
            *mock_chat_repo,
            removeMember(
                param.request_dto.chat_id, param.request_dto.member_id, _
            )
        )
            .WillRepeatedly(
                Invoke([](int64_t, int64_t, auto) -> drogon::Task<void> {
                    return createFakeVoidTask();
                })
            );
    }

    if (param.expected_result == RemoveMemberExpectedResult::Forbidden) {
        EXPECT_THROW(
            drogon::sync_wait(chat_service->removeMember(param.request_dto)),
            messenger::exceptions::ForbiddenException
        ) << param.test_name;
    } else if (param.expected_result == RemoveMemberExpectedResult::Conflict) {
        EXPECT_THROW(
            drogon::sync_wait(chat_service->removeMember(param.request_dto)),
            messenger::exceptions::ConflictException
        ) << param.test_name;
    } else if (param.expected_result == RemoveMemberExpectedResult::NotFound) {
        EXPECT_THROW(
            drogon::sync_wait(chat_service->removeMember(param.request_dto)),
            messenger::exceptions::NotFoundException
        ) << param.test_name;
    } else {
        EXPECT_NO_THROW(
            drogon::sync_wait(chat_service->removeMember(param.request_dto))
        ) << param.test_name;
    }
}

INSTANTIATE_TEST_SUITE_P(
    RemoveMemberTest,
    ServiceRemoveMemberTest,
    ::testing::Values(
        RemoveMemberTestCase{
            "Fail: Member tries to remove another Member",
            {10, 1, 99},
            messenger::models::ChatRole::Member,
            messenger::models::ChatRole::Member,
            true,
            3,
            RemoveMemberExpectedResult::Forbidden
        },
        RemoveMemberTestCase{
            "Fail: Admin tries to remove another Admin",
            {10, 1, 99},
            messenger::models::ChatRole::Admin,
            messenger::models::ChatRole::Admin,
            true,
            3,
            RemoveMemberExpectedResult::Forbidden
        },
        RemoveMemberTestCase{
            "Fail: Admin tries to remove Owner",
            {10, 1, 99},
            messenger::models::ChatRole::Admin,
            messenger::models::ChatRole::Owner,
            true,
            3,
            RemoveMemberExpectedResult::Forbidden
        },
        RemoveMemberTestCase{
            "Fail: Owner leaves chat with other people",
            {10, 1, 10},
            messenger::models::ChatRole::Owner,
            messenger::models::ChatRole::Owner,
            true,
            5,
            RemoveMemberExpectedResult::Conflict
        },
        RemoveMemberTestCase{
            "Fail: Target not found in chat",
            {10, 1, 99},
            messenger::models::ChatRole::Admin,
            messenger::models::ChatRole::Member,
            false,
            3,
            RemoveMemberExpectedResult::NotFound
        },
        RemoveMemberTestCase{
            "Success: Admin removes normal Member",
            {10, 1, 99},
            messenger::models::ChatRole::Admin,
            messenger::models::ChatRole::Member,
            true,
            3,
            RemoveMemberExpectedResult::Success
        },
        RemoveMemberTestCase{
            "Success: Owner removes Admin",
            {10, 1, 99},
            messenger::models::ChatRole::Owner,
            messenger::models::ChatRole::Admin,
            true,
            3,
            RemoveMemberExpectedResult::Success
        },
        RemoveMemberTestCase{
            "Success: Anyone leaves chat voluntarily",
            {10, 1, 10},
            messenger::models::ChatRole::Member,
            messenger::models::ChatRole::Member,
            true,
            5,
            RemoveMemberExpectedResult::Success
        }
    )
);

struct GetChatMemberTestCase {
    std::string test_name;
    GetChatMemberRequestDto request_dto;
    bool is_requester_in_chat;
    bool is_user_found;
};

class ServiceGetChatMemberTest
    : public BaseChatServiceTest,
      public ::testing::WithParamInterface<GetChatMemberTestCase> {};

TEST_P(ServiceGetChatMemberTest, GetChatMemberTest) {
    auto param = GetParam();

    EXPECT_CALL(*mock_chat_repo, getMembers(param.request_dto.chat_id, _))
        .WillRepeatedly(Invoke(
            [param](
                int64_t c_id, auto t
            ) -> drogon::Task<std::vector<ChatMember>> {
                std::vector<ChatMember> members;
                if (param.is_requester_in_chat) {
                    ChatMember m;
                    m.setUserId(param.request_dto.user_id);
                    members.push_back(m);
                }
                return createFakeTask(members);
            }
        ));

    if (param.is_requester_in_chat) {
        EXPECT_CALL(
            *mock_chat_repo,
            getMember(param.request_dto.chat_id, param.request_dto.member_id)
        )
            .WillRepeatedly(
                Invoke([](int64_t c, int64_t m) -> drogon::Task<ChatMember> {
                    ChatMember member;
                    member.setUserId(m);
                    member.setChatId(c);
                    return createFakeTask(member);
                })
            );

        EXPECT_CALL(*mock_user_repo, getById(param.request_dto.member_id))
            .WillRepeatedly(Invoke(
                [param](int64_t id) -> drogon::Task<std::optional<User>> {
                    if (param.is_user_found) {
                        User u;
                        u.setId(id);
                        return createFakeTask<std::optional<User>>(u);
                    }
                    return createFakeTask<std::optional<User>>(std::nullopt);
                }
            ));
    }

    if (!param.is_requester_in_chat) {
        EXPECT_THROW(
            drogon::sync_wait(chat_service->getChatMember(param.request_dto)),
            messenger::exceptions::ForbiddenException
        ) << param.test_name;
    } else if (!param.is_user_found) {
        EXPECT_THROW(
            drogon::sync_wait(chat_service->getChatMember(param.request_dto)),
            messenger::exceptions::NotFoundException
        ) << param.test_name;
    } else {
        GetChatMemberResponseDto response_dto;
        EXPECT_NO_THROW(
            response_dto =
                drogon::sync_wait(chat_service->getChatMember(param.request_dto)
                )
        ) << param.test_name;
        EXPECT_EQ(
            response_dto.member_info.getValueOfId(), param.request_dto.member_id
        );
    }
}

INSTANTIATE_TEST_SUITE_P(
    GetChatMemberTest,
    ServiceGetChatMemberTest,
    ::testing::Values(
        GetChatMemberTestCase{"Success", {10, 42, 99}, true, true},
        GetChatMemberTestCase{
            "Forbidden: Requester not in chat",
            {10, 42, 99},
            false,
            true
        },
        GetChatMemberTestCase{
            "Not Found: Target user missing",
            {10, 42, 99},
            true,
            false
        }
    )
);

struct GetChatMembersTestCase {
    std::string test_name;
    GetChatMembersRequestDto request_dto;
    bool is_requester_in_chat;
    int members_count;
};

class ServiceGetChatMembersTest
    : public BaseChatServiceTest,
      public ::testing::WithParamInterface<GetChatMembersTestCase> {};

TEST_P(ServiceGetChatMembersTest, GetChatMembersTest) {
    auto param = GetParam();

    EXPECT_CALL(*mock_chat_repo, getMembers(param.request_dto.chat_id, _))
        .WillRepeatedly(Invoke(
            [param](
                int64_t c_id, auto t
            ) -> drogon::Task<std::vector<ChatMember>> {
                std::vector<ChatMember> members;
                if (param.is_requester_in_chat) {
                    ChatMember m;
                    m.setUserId(param.request_dto.user_id);
                    members.push_back(m);
                }
                for (int i = 1; i < param.members_count; ++i) {
                    ChatMember m;
                    m.setUserId(param.request_dto.user_id + i);
                    members.push_back(m);
                }
                return createFakeTask(members);
            }
        ));

    if (param.is_requester_in_chat) {
        EXPECT_CALL(*mock_user_repo, getByIds(_))
            .WillRepeatedly(Invoke(
                [](std::vector<int64_t> ids
                ) -> drogon::Task<std::vector<User>> {
                    std::vector<User> users;
                    for (auto id : ids) {
                        User u;
                        u.setId(id);
                        users.push_back(u);
                    }
                    return createFakeTask(users);
                }
            ));
    }

    if (!param.is_requester_in_chat) {
        EXPECT_THROW(
            drogon::sync_wait(chat_service->getChatMembers(param.request_dto)),
            messenger::exceptions::ForbiddenException
        ) << param.test_name;
    } else {
        GetChatMembersResponseDto response_dto;
        EXPECT_NO_THROW(
            response_dto =
                drogon::sync_wait(chat_service->getChatMembers(param.request_dto
                ))
        ) << param.test_name;
        EXPECT_EQ(response_dto.members.size(), param.members_count);
        EXPECT_EQ(response_dto.members_info.size(), param.members_count);
    }
}

INSTANTIATE_TEST_SUITE_P(
    GetChatMembersTest,
    ServiceGetChatMembersTest,
    ::testing::Values(
        GetChatMembersTestCase{"Success: Fetch all members", {10, 42}, true, 5},
        GetChatMembersTestCase{
            "Forbidden: Requester not in chat",
            {10, 42},
            false,
            5
        }
    )
);

struct UpdateChatInfoTestCase {
    std::string test_name;
    UpdateChatInfoRequestDto request_dto;
    std::string requester_role;
};

class ServiceUpdateChatInfoTest
    : public BaseChatServiceTest,
      public ::testing::WithParamInterface<UpdateChatInfoTestCase> {};

TEST_P(ServiceUpdateChatInfoTest, UpdateChatInfoTest) {
    auto param = GetParam();

    EXPECT_CALL(
        *mock_chat_repo,
        getMember(param.request_dto.chat_id, param.request_dto.user_id)
    )
        .WillRepeatedly(
            Invoke([param](int64_t c, int64_t u) -> drogon::Task<ChatMember> {
                ChatMember m;
                m.setRole(param.requester_role);
                m.setChatId(c);
                m.setUserId(u);
                return createFakeTask(m);
            })
        );

    if (param.requester_role != messenger::models::ChatRole::Member) {
        EXPECT_CALL(
            *mock_chat_repo,
            updateInfo(
                param.request_dto.chat_id, param.request_dto.name,
                param.request_dto.avatar, param.request_dto.description, _
            )
        )
            .WillRepeatedly(Invoke([](...) -> drogon::Task<void> {
                return createFakeVoidTask();
            }));
    }

    if (param.requester_role == messenger::models::ChatRole::Member) {
        EXPECT_THROW(
            drogon::sync_wait(chat_service->updateChatInfo(param.request_dto)),
            messenger::exceptions::ForbiddenException
        ) << param.test_name;
    } else {
        EXPECT_NO_THROW(
            drogon::sync_wait(chat_service->updateChatInfo(param.request_dto))
        ) << param.test_name;
    }
}

INSTANTIATE_TEST_SUITE_P(
    UpdateChatInfoTest,
    ServiceUpdateChatInfoTest,
    ::testing::Values(
        UpdateChatInfoTestCase{
            "Success: Admin changes info",
            {10, 42, "New Name", std::nullopt, std::nullopt},
            messenger::models::ChatRole::Admin
        },
        UpdateChatInfoTestCase{
            "Success: Owner changes info",
            {10, 42, "New Name", std::nullopt, std::nullopt},
            messenger::models::ChatRole::Owner
        },
        UpdateChatInfoTestCase{
            "Forbidden: Member tries to change info",
            {10, 42, "New Name", std::nullopt, std::nullopt},
            messenger::models::ChatRole::Member
        }
    )
);

struct GetChatByIdTestCase {
    std::string test_name;
    GetChatByIdRequestDto request_dto;
    bool chat_exists;
};

class ServiceGetChatByIdTest
    : public BaseChatServiceTest,
      public ::testing::WithParamInterface<GetChatByIdTestCase> {};

TEST_P(ServiceGetChatByIdTest, GetChatByIdTest) {
    auto param = GetParam();

    EXPECT_CALL(*mock_chat_repo, getById(param.request_dto.chat_id))
        .WillRepeatedly(
            Invoke([param](int64_t id) -> drogon::Task<std::optional<Chat>> {
                if (param.chat_exists) {
                    Chat chat;
                    chat.setId(id);
                    chat.setName("Test Chat");
                    return createFakeTask<std::optional<Chat>>(chat);
                }
                return createFakeTask<std::optional<Chat>>(std::nullopt);
            })
        );

    if (!param.chat_exists) {
        EXPECT_THROW(
            drogon::sync_wait(chat_service->getChatById(param.request_dto)),
            messenger::exceptions::NotFoundException
        ) << param.test_name;
    } else {
        GetChatByIdResponseDto response_dto;
        EXPECT_NO_THROW(
            response_dto =
                drogon::sync_wait(chat_service->getChatById(param.request_dto))
        ) << param.test_name;
        EXPECT_EQ(response_dto.chat.getValueOfId(), param.request_dto.chat_id);
    }
}

INSTANTIATE_TEST_SUITE_P(
    GetChatByIdTest,
    ServiceGetChatByIdTest,
    ::testing::Values(
        GetChatByIdTestCase{"Success: Chat exists", {42, 10}, true},
        GetChatByIdTestCase{"Not Found: Chat doesn't exist", {42, 10}, false}
    )
);

// TODO: methods that creates transactions.

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
// }

// INSTANTIATE_TEST_SUITE_P(
//     ***Test,
//     Service***Test,
//     ::testing::Values(
//         ***TestCase{
//         }
//     )
// );