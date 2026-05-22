#include <drogon/HttpTypes.h>
#include <drogon/drogon.h>
#include <drogon/utils/coroutine.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <json/forwards.h>
#include <json/value.h>
#include <cmath>
#include <cstdlib>
#include <memory>
#include <optional>
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "services/ChatService.hpp"
#include "tests/mocks/MockChatRepository.hpp"
#include "tests/mocks/MockMessageRepository.hpp"
#include "tests/mocks/MockUserRepository.hpp"

using User = drogon_model::messenger_db::Users;
using Chat = drogon_model::messenger_db::Chats;
using Message = drogon_model::messenger_db::Messages;
using ::testing::_;
using ::testing::Invoke;
using ::testing::Return;

template <typename T>
drogon::Task<T> createFakeTask(T data) {
    co_return data;
}

Json::Value makeJsonWithOnlyUserId(int64_t user_id) {
    Json::Value j;
    j["user_id"] = user_id;
    return j;
}

struct GetMessageByIdTestCase {
    std::string test_name;
    int64_t message_id;
    int64_t chat_id;
    std::vector<int64_t> true_member_ids;

    bool is_message_exists;
    Json::Value request_json;

    drogon::HttpStatusCode expected_status;
};

class ServiceGetMessageByIdTest
    : public ::testing::TestWithParam<GetMessageByIdTestCase> {
protected:
    std::shared_ptr<MockChatRepository> mock_chat_repo;
    std::shared_ptr<api::v1::ChatService> chat_service;

    void SetUp() override {
        auto chat_msg_repo = std::make_unique<MockMessageRepository>();
        auto chat_usr_repo = std::make_unique<MockUserRepository>();

        mock_chat_repo = std::make_shared<MockChatRepository>(
            std::move(chat_msg_repo), std::move(chat_usr_repo)
        );
        chat_service = std::make_shared<api::v1::ChatService>();
        chat_service->setChatRepo(mock_chat_repo);
    }
};

TEST_P(ServiceGetMessageByIdTest, GetMessageByIdTest) {
    auto param = GetParam();
    EXPECT_CALL(*mock_chat_repo, getMessageById(param.message_id))
        .WillRepeatedly(Invoke(
            [param](int64_t message_id
            ) -> drogon::Task<std::optional<Message>> {
                if (param.is_message_exists) {
                    Message mes;
                    mes.setChatId(param.chat_id);
                    return createFakeTask<std::optional<Message>>(mes);
                }
                return createFakeTask<std::optional<Message>>(std::nullopt);
            }
        ));
    EXPECT_CALL(*mock_chat_repo, getMembers(param.chat_id))
        .WillRepeatedly(Invoke(
            [param](int64_t chat_id)
                -> drogon::Task<
                    std::vector<messenger::repositories::ChatMember>> {
                std::vector<messenger::repositories::ChatMember> chat_members;
                for (int64_t member_id : param.true_member_ids) {
                    messenger::repositories::ChatMember chat_member;
                    chat_member.setUserId(member_id);
                    chat_members.push_back(chat_member);
                }
                return createFakeTask<
                    std::vector<messenger::repositories::ChatMember>>(
                    chat_members
                );
            }
        ));
    auto response = drogon::sync_wait(chat_service->getMessageById(
        std::make_shared<Json::Value>(param.request_json), param.message_id
    ));
    ASSERT_NE(response, nullptr);
    EXPECT_EQ(response->getStatusCode(), param.expected_status)
        << "Failed test: " << param.test_name;
}

INSTANTIATE_TEST_SUITE_P(
    GetMessageByIdServiceTest,
    ServiceGetMessageByIdTest,
    ::testing::Values(
        GetMessageByIdTestCase{
            "Successfully get message by id",
            123,
            456,
            {1, 2, 3},
            true,
            makeJsonWithOnlyUserId(1),
            drogon::k200OK
        },
        GetMessageByIdTestCase{
            "Message not found",
            123,
            456,
            {1, 2, 3},
            false,
            makeJsonWithOnlyUserId(1),
            drogon::k404NotFound
        },
        GetMessageByIdTestCase{
            "User not member of chat",
            123,
            456,
            {1, 2, 3},
            true,
            makeJsonWithOnlyUserId(4),
            drogon::k403Forbidden
        }
    )
);

struct GetUserChatsTestCase {
    std::string test_name;
    int64_t user_id;

    Json::Value request_json;
    std::vector<ChatPreview> expected_chats;

    drogon::HttpStatusCode expected_status;
};

class ServiceGetUserChatsTest
    : public ::testing::TestWithParam<GetUserChatsTestCase> {
protected:
    std::shared_ptr<MockChatRepository> mock_chat_repo;
    std::shared_ptr<api::v1::ChatService> chat_service;

    void SetUp() override {
        auto chat_msg_repo = std::make_unique<MockMessageRepository>();
        auto chat_usr_repo = std::make_unique<MockUserRepository>();

        mock_chat_repo = std::make_shared<MockChatRepository>(
            std::move(chat_msg_repo), std::move(chat_usr_repo)
        );
        chat_service = std::make_shared<api::v1::ChatService>();
        chat_service->setChatRepo(mock_chat_repo);
    }
};

TEST_P(ServiceGetUserChatsTest, GetUserChatsTest) {
    auto param = GetParam();

    EXPECT_CALL(*mock_chat_repo, getByUser(param.user_id))
        .WillRepeatedly(Invoke(
            [param](int64_t user_id) -> drogon::Task<std::vector<ChatPreview>> {
                return createFakeTask<std::vector<ChatPreview>>(
                    param.expected_chats
                );
            }
        ));
    auto response = drogon::sync_wait(chat_service->getUserChats(
        std::make_shared<Json::Value>(param.request_json), param.user_id
    ));
    ASSERT_NE(response, nullptr);
    EXPECT_EQ(response->getStatusCode(), param.expected_status)
        << "Failed test: " << param.test_name;
    if (param.expected_status == drogon::k200OK) {
        const auto &chats_json = (*response->getJsonObject())["chats"];
        ASSERT_EQ(chats_json.size(), param.expected_chats.size());

        for (const auto &expected_chat : param.expected_chats) {
            bool found = false;

            for (Json::ArrayIndex i = 0; i < param.expected_chats.size(); i++) {
                if (chats_json[i]["chat_id"].asInt64() ==
                    expected_chat.chat_id) {
                    found = true;
                    EXPECT_EQ(
                        chats_json[i]["chat_id"].asInt64(),
                        expected_chat.chat_id
                    );
                    EXPECT_EQ(
                        chats_json[i]["title"].asString(), expected_chat.title
                    );
                    EXPECT_EQ(
                        chats_json[i]["avatar_path"].asString(),
                        expected_chat.avatar_path
                    );
                    EXPECT_EQ(
                        chats_json[i]["last_message"],
                        expected_chat.last_message->toJson()
                    );
                    EXPECT_EQ(
                        chats_json[i]["unread_count"].asInt64(),
                        expected_chat.unread_count
                    );
                    EXPECT_EQ(
                        chats_json[i]["type"].asString(), expected_chat.type
                    );
                }
            }

            EXPECT_TRUE(found) << "Chat not found in response";
        }
    }
}

Message createFakeMessage(int64_t id, int64_t chat_id, int64_t sender_id) {
    Message msg;
    msg.setId(id);
    msg.setChatId(chat_id);
    msg.setSenderId(sender_id);
    return msg;
}

INSTANTIATE_TEST_SUITE_P(
    GetUserChatsServiceTest,
    ServiceGetUserChatsTest,
    ::testing::Values(
        GetUserChatsTestCase{
            "Successfully get user chats - one chat",
            123,
            makeJsonWithOnlyUserId(123),
            {{1, "chat1", "avatar1", createFakeMessage(1, 1, 123), 123, "direct"
            }},
            drogon::k200OK
        },
        GetUserChatsTestCase{
            "Successfully get user chats - multiple chats",
            123,
            makeJsonWithOnlyUserId(123),
            {{1, "chat1", "avatar1", createFakeMessage(1, 1, 123), 123, "direct"
             },
             {2, "chat2", "avatar2", createFakeMessage(2, 2, 123), 234, "direct"
             },
             {3, "chat3", "", createFakeMessage(3, 3, 123), 345, "saved"}},
            drogon::k200OK
        },
        GetUserChatsTestCase{
            "Successfully get user chats - no chats",
            123,
            makeJsonWithOnlyUserId(123),
            {},
            drogon::k200OK
        },
        GetUserChatsTestCase{
            "Access denied",
            123,
            makeJsonWithOnlyUserId(124),
            {{1, "chat1", "avatar1", createFakeMessage(1, 1, 123), 123, "direct"
            }},
            drogon::k403Forbidden
        }
    )
);

struct CreateOrGetDirectChatTestCase {
    std::string test_name;

    Json::Value request_json;
    bool chat_exists;

    drogon::HttpStatusCode expected_status;
};

class ServiceCreateOrGetDirectChatTest
    : public ::testing::TestWithParam<CreateOrGetDirectChatTestCase> {
protected:
    std::shared_ptr<MockChatRepository> mock_chat_repo;
    std::shared_ptr<api::v1::ChatService> chat_service;

    void SetUp() override {
        auto chat_msg_repo = std::make_unique<MockMessageRepository>();
        auto chat_usr_repo = std::make_unique<MockUserRepository>();

        mock_chat_repo = std::make_shared<MockChatRepository>(
            std::move(chat_msg_repo), std::move(chat_usr_repo)
        );
        chat_service = std::make_shared<api::v1::ChatService>();
        chat_service->setChatRepo(mock_chat_repo);
    }
};

TEST_P(ServiceCreateOrGetDirectChatTest, CreateOrGetDirectChatTest) {
    auto param = GetParam();

    EXPECT_CALL(
        *mock_chat_repo, getDirect(
                             param.request_json["user_id"].asInt64(),
                             param.request_json["target_user_id"].asInt64()
                         )
    )
        .WillRepeatedly(Invoke(
            [param](
                int64_t user_id, int64_t other_user_id
            ) -> drogon::Task<std::optional<Chat>> {
                if (param.chat_exists) {
                    Chat chat;
                    chat.setId(1);
                    return createFakeTask<std::optional<Chat>>(chat);
                }
                return createFakeTask<std::optional<Chat>>(std::nullopt);
            }
        ));
    if (!param.chat_exists) {
        EXPECT_CALL(
            *mock_chat_repo, getOrCreateDirect(
                                 param.request_json["user_id"].asInt64(),
                                 param.request_json["target_user_id"].asInt64()
                             )
        )
            .WillRepeatedly(Invoke(
                [param](
                    int64_t user_id, int64_t other_user_id
                ) -> drogon::Task<Chat> {
                    Chat chat;
                    chat.setId(1);
                    return createFakeTask<Chat>(chat);
                }
            ));
    }
    auto response = drogon::sync_wait(chat_service->createOrGetDirectChat(
        std::make_shared<Json::Value>(param.request_json)
    ));
    ASSERT_NE(response, nullptr);
    EXPECT_EQ(response->getStatusCode(), param.expected_status);
}

Json::Value
makeJsonForGetOrCreateDirectChat(int64_t user_id, int64_t other_user_id) {
    Json::Value j;
    j["user_id"] = user_id;
    j["target_user_id"] = other_user_id;
    return j;
}

INSTANTIATE_TEST_SUITE_P(
    CreateOrGetDirectChatServiceTest,
    ServiceCreateOrGetDirectChatTest,
    ::testing::Values(
        CreateOrGetDirectChatTestCase{
            "Successfully get existing direct chat",
            makeJsonForGetOrCreateDirectChat(123, 456), true, drogon::k200OK
        },
        CreateOrGetDirectChatTestCase{
            "Successfully create new direct chat",
            makeJsonForGetOrCreateDirectChat(123, 456), false,
            drogon::k201Created
        }
    )
);
