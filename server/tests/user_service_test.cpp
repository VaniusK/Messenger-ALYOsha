#include <drogon/HttpTypes.h>
#include <drogon/drogon.h>
#include <drogon/utils/coroutine.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <json/value.h>
#include <cstdlib>
#include <memory>
#include <optional>
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "mocks/MockChatRepository.hpp"
#include "mocks/MockMessageRepository.hpp"
#include "mocks/MockUserRepository.hpp"
#include "services/UserService.hpp"

using User = drogon_model::messenger_db::Users;
using Chat = drogon_model::messenger_db::Chats;
using ::testing::_;
using ::testing::Invoke;
using ::testing::Return;

Json::Value makeJson(std::vector<std::pair<std::string, std::string>> &&fields
) {
    Json::Value j;
    for (auto &[field, value] : fields) {
        j[field] = value;
    }
    return j;
}

template <typename T>
drogon::Task<T> createFakeTask(T data) {
    co_return data;
}

drogon::Task<bool> returnFakeBool(bool result) {
    co_return result;
}

struct RegisterTestCase {
    std::string test_name;
    Json::Value request_json;

    bool is_user_exists;
    bool is_user_create_success;

    drogon::HttpStatusCode excpected_status;
};

class ServiceRegisterUserTest
    : public ::testing::TestWithParam<RegisterTestCase> {
protected:
    std::shared_ptr<MockUserRepository> mock_user_repo;
    std::shared_ptr<MockChatRepository> mock_chat_repo;
    std::shared_ptr<api::v1::UserService> user_service;

    void SetUp() override {
        mock_user_repo = std::make_shared<MockUserRepository>();

        auto chat_msg_repo = std::make_unique<MockMessageRepository>();
        auto chat_usr_repo = std::make_unique<MockUserRepository>();

        mock_chat_repo = std::make_shared<MockChatRepository>(
            std::move(chat_msg_repo), std::move(chat_usr_repo)
        );
        user_service = std::make_shared<api::v1::UserService>();
        user_service->setUserRepo(mock_user_repo);
        user_service->setChatRepo(mock_chat_repo);
    }
};

TEST_P(ServiceRegisterUserTest, RegisterUserTest) {
    auto param = GetParam();
    EXPECT_CALL(
        *mock_user_repo, getByHandle(param.request_json["handle"].asString())
    )
        .WillRepeatedly(Invoke(
            [param](const std::string &handle
            ) -> drogon::Task<std::optional<User>> {
                if (param.is_user_exists) {
                    User fake_user;
                    fake_user.setId(123);
                    fake_user.setHandle(handle);
                    fake_user.setPasswordHash("some_hash_bebebe");
                    return createFakeTask<std::optional<User>>(fake_user);
                }
                return createFakeTask<std::optional<User>>(std::nullopt);
            }
        ));
    if (!param.is_user_exists) {
        EXPECT_CALL(*mock_user_repo, create(_, _, _))
            .WillRepeatedly(Invoke(
                [param](const std::string &, const std::string &, const std::string &)
                    -> drogon::Task<bool> {
                    return createFakeTask<bool>(param.is_user_create_success);
                }
            ));
    }
    if (!param.is_user_exists && param.is_user_create_success) {
        EXPECT_CALL(*mock_chat_repo, createSaved(_))
            .WillRepeatedly(Invoke([param](int64_t) -> drogon::Task<Chat> {
                Chat fake_chat;
                return createFakeTask<Chat>(fake_chat);
            }));
    }

    auto request_json = std::make_shared<Json::Value>(param.request_json);
    auto response = drogon::sync_wait(user_service->registerUser(request_json));

    ASSERT_NE(response, nullptr);
    EXPECT_EQ(response->getStatusCode(), param.excpected_status)
        << "Failed test: " << param.test_name;
}

INSTANTIATE_TEST_SUITE_P(
    UserServiceTest,
    ServiceRegisterUserTest,
    ::testing::Values(
        RegisterTestCase{
            "Successful creation",
            makeJson(
                {{"handle", "PIDORAS"},
                 {"password", "pass_omg"},
                 {"display_name", "chmo"}}
            ),
            false, true, drogon::k201Created
        },
        RegisterTestCase{
            "User already exists",
            makeJson(
                {{"handle", "PIDORAS"},
                 {"password", "pass_omg"},
                 {"display_name", "chmo"}}
            ),
            true, false, drogon::k409Conflict
        },
        RegisterTestCase{
            "User creation failed",
            makeJson(
                {{"handle", "PIDORAS"},
                 {"password", "pass_omg"},
                 {"display_name", "chmo"}}
            ),
            false, false, drogon::k500InternalServerError
        }
    )
);

struct LoginTestCase {
    std::string test_name;
    Json::Value request_json;

    bool is_user_exists;
    bool is_password_correct;

    drogon::HttpStatusCode excpected_status;
};

class ServiceLoginUserTest : public ::testing::TestWithParam<LoginTestCase> {
protected:
    std::shared_ptr<MockUserRepository> mock_user_repo;
    std::shared_ptr<api::v1::UserService> user_service;
    std::shared_ptr<MockPasswordHasher> mock_password_hasher;

    void SetUp() override {
        setenv("JWT_KEY", "cool_key", 1);
        mock_user_repo = std::make_shared<MockUserRepository>();
        mock_password_hasher = std::make_shared<MockPasswordHasher>();

        user_service = std::make_shared<api::v1::UserService>();
        user_service->setUserRepo(mock_user_repo);
        user_service->setPasswordHasher(mock_password_hasher);
        user_service->setChatRepo(nullptr);
    }
};

TEST_P(ServiceLoginUserTest, LoginUserTest) {
    auto param = GetParam();
    EXPECT_CALL(
        *mock_user_repo, getByHandle(param.request_json["handle"].asString())
    )
        .WillRepeatedly(Invoke(
            [param](const std::string &handle
            ) -> drogon::Task<std::optional<User>> {
                if (param.is_user_exists) {
                    User fake_user;
                    fake_user.setId(123);
                    fake_user.setHandle(handle);
                    fake_user.setPasswordHash("some_hash_bebebe");
                    return createFakeTask<std::optional<User>>(fake_user);
                }
                return createFakeTask<std::optional<User>>(std::nullopt);
            }
        ));
    if (param.is_user_exists) {
        EXPECT_CALL(*mock_password_hasher, verifyPassword(_, _))
            .WillOnce(Return(param.is_password_correct));
    }

    auto request_json = std::make_shared<Json::Value>(param.request_json);
    auto response = drogon::sync_wait(user_service->loginUser(request_json));

    ASSERT_NE(response, nullptr);
    EXPECT_EQ(response->getStatusCode(), param.excpected_status)
        << "Failed test: " << param.test_name;
    ;
    if (param.is_user_exists && param.is_password_correct) {
        EXPECT_TRUE(response->getJsonObject()->isMember("token"))
            << "Failed test: " << param.test_name;
        ;
        EXPECT_FALSE((*response->getJsonObject())["token"].empty())
            << "Failed test: " << param.test_name;
        ;
    }
}

INSTANTIATE_TEST_SUITE_P(
    LoginServiceTest,
    ServiceLoginUserTest,
    ::testing::Values(
        LoginTestCase{
            "Successful login",
            makeJson({{"handle", "PIDOR"}, {"password", "cool_password"}}),
            true, true, drogon::k200OK
        },
        LoginTestCase{
            "User doesn't exist",
            makeJson({{"handle", "PIDOR"}, {"password", "cool_password"}}),
            false, false, drogon::k401Unauthorized
        },
        LoginTestCase{
            "Wrong password",
            makeJson({{"handle", "PIDOR"}, {"password", "cool_password"}}),
            true, false, drogon::k401Unauthorized
        }
    )
);

struct GetUserByIdTestCase {
    std::string test_name;
    int64_t user_id;
    bool is_user_exists;

    drogon::HttpStatusCode excpected_status;
};