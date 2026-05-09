#include <drogon/HttpTypes.h>
#include <drogon/drogon.h>
#include <drogon/utils/coroutine.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <json/value.h>
#include <cstddef>
#include <cstdlib>
#include <memory>
#include <optional>
#include <vector>
#include "../mocks/MockChatRepository.hpp"
#include "../mocks/MockMessageRepository.hpp"
#include "../mocks/MockUserRepository.hpp"
#include "dto/UserServiceDtos.hpp"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "services/UserService.hpp"
#include "utils/server_exceptions.hpp"

using User = drogon_model::messenger_db::Users;
using Chat = drogon_model::messenger_db::Chats;
using ::testing::_;
using ::testing::Invoke;
using ::testing::Return;

template <typename T>
drogon::Task<T> createFakeTask(T data) {
    co_return data;
}

struct RegisterTestCase {
    std::string test_name;
    RegisterUserRequestDto request_dto;

    bool is_user_create_success;
};

class ServiceRegisterUserTest
    : public ::testing::TestWithParam<RegisterTestCase> {
protected:
    std::shared_ptr<MockUserRepository> mock_user_repo;
    std::shared_ptr<MockChatRepository> mock_chat_repo;
    std::shared_ptr<api::v1::UserService> user_service;

    void SetUp() override {
        mock_user_repo = std::make_shared<MockUserRepository>();

        auto chat_msg_repo = std::make_unique<MockMessageRepository>(
            std::make_unique<messenger::repositories::AttachmentRepository>()
        );
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

    EXPECT_CALL(*mock_user_repo, getByHandle(param.request_dto.handle))
        .WillRepeatedly(Invoke(
            [param](const std::string &handle
            ) -> drogon::Task<std::optional<User>> {
                User fake_user;
                fake_user.setId(123);
                fake_user.setHandle(handle);
                fake_user.setPasswordHash("some_hash_bebebe");
                return createFakeTask<std::optional<User>>(fake_user);
            }
        ));

    EXPECT_CALL(*mock_user_repo, create(_, _, _))
        .WillRepeatedly(Invoke(
            [param](const std::string &, const std::string &, const std::string &)
                -> drogon::Task<bool> {
                return createFakeTask<bool>(param.is_user_create_success);
            }
        ));
    if (param.is_user_create_success) {
        EXPECT_CALL(*mock_chat_repo, createSaved(_, _))
            .WillRepeatedly(Invoke(
                [param](int64_t, std::shared_ptr<drogon::orm::Transaction>)
                    -> drogon::Task<Chat> {
                    Chat fake_chat;
                    return createFakeTask<Chat>(fake_chat);
                }
            ));
    }

    if (!param.is_user_create_success) {
        EXPECT_THROW(
            drogon::sync_wait(user_service->registerUser(param.request_dto)),
            messenger::exceptions::ConflictException
        ) << "Failed test: "
          << param.test_name;
    } else {
        EXPECT_NO_THROW(
            drogon::sync_wait(user_service->registerUser(param.request_dto))
        ) << "Failed test: "
          << param.test_name;
    }
}

INSTANTIATE_TEST_SUITE_P(
    UserServiceTest,
    ServiceRegisterUserTest,
    ::testing::Values(
        RegisterTestCase{
            "Successful creation",
            {"PIDORAS", "pass_omg", "chmo"},
            true
        },
        RegisterTestCase{
            "User already exists",
            {"PIDORAS", "pass_omg", "chmo"},
            false
        }
    )
);

struct LoginTestCase {
    std::string test_name;
    LoginUserRequestDto request_dto;

    bool is_user_exists;
    bool is_password_correct;
    bool is_jwt_key_set;

    LoginUserResponseDto expected_response_dto;
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
    EXPECT_CALL(*mock_user_repo, getByHandle(param.request_dto.handle))
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

    if (!param.is_user_exists) {
        EXPECT_THROW(
            drogon::sync_wait(user_service->loginUser(param.request_dto)),
            messenger::exceptions::UnauthorizedException
        );
    } else if (param.is_user_exists && !param.is_password_correct) {
        EXPECT_THROW(
            drogon::sync_wait(user_service->loginUser(param.request_dto)),
            messenger::exceptions::UnauthorizedException
        );
    } else if (!param.is_jwt_key_set) {
        unsetenv("JWT_KEY");
        EXPECT_THROW(
            drogon::sync_wait(user_service->loginUser(param.request_dto)),
            messenger::exceptions::InternalServerErrorException
        ) << "Failed test: "
          << param.test_name;
        setenv("JWT_KEY", "cool_key", 1);
    } else {
        LoginUserResponseDto response_dto;
        EXPECT_NO_THROW(
            response_dto =
                drogon::sync_wait(user_service->loginUser(param.request_dto))
        ) << "Failed test: "
          << param.test_name;
        EXPECT_NE(response_dto.token, "") << "Failed test: " << param.test_name;
    }
}

INSTANTIATE_TEST_SUITE_P(
    LoginServiceTest,
    ServiceLoginUserTest,
    ::testing::Values(
        LoginTestCase{
            "Successful login",
            {"PIDOR", "cool_password"},
            true,
            true,
            true,
            {"tokenwtf"}
        },
        LoginTestCase{
            "User doesn't exist",
            {"PIDOR", "cool_password"},
            false,
            false,
            false,
            {""}
        },
        LoginTestCase{
            "Wrong password",
            {"PIDOR", "cool_password"},
            true,
            false,
            false,
            {""}
        },
        LoginTestCase{
            "JWT_KEY not set",
            {"PIDOR", "cool_password"},
            true,
            true,
            false,
            {""}
        }
    )
);

struct GetUserByIdTestCase {
    std::string test_name;
    int64_t user_id;
    std::string user_handle;
    std::string password_hash;
    std::string display_name;

    bool is_user_exists;
};

class ServiceGetUserByIdTest
    : public ::testing::TestWithParam<GetUserByIdTestCase> {
protected:
    std::shared_ptr<MockUserRepository> mock_user_repo;
    std::shared_ptr<api::v1::UserService> user_service;
    std::shared_ptr<MockPasswordHasher> mock_password_hasher;

    void SetUp() override {
        mock_user_repo = std::make_shared<MockUserRepository>();

        user_service = std::make_shared<api::v1::UserService>();
        user_service->setUserRepo(mock_user_repo);
        user_service->setChatRepo(nullptr);
    }
};

TEST_P(ServiceGetUserByIdTest, GetUserByIdTest) {
    auto param = GetParam();
    EXPECT_CALL(*mock_user_repo, getById(param.user_id))
        .WillRepeatedly(Invoke(
            [param](int64_t user_id) -> drogon::Task<std::optional<User>> {
                if (param.is_user_exists) {
                    User fake_user;
                    fake_user.setId(user_id);
                    fake_user.setHandle(param.user_handle);
                    fake_user.setPasswordHash(param.password_hash);
                    fake_user.setDisplayName(param.display_name);
                    return createFakeTask<std::optional<User>>(fake_user);
                }
                return createFakeTask<std::optional<User>>(std::nullopt);
            }
        ));
    if (!param.is_user_exists) {
        EXPECT_THROW(
            drogon::sync_wait(user_service->getUserById(param.user_id)),
            messenger::exceptions::NotFoundException
        ) << "Failed test: "
          << param.test_name;
    } else {
        GetUserResponseDto response_dto;
        EXPECT_NO_THROW(
            response_dto =
                drogon::sync_wait(user_service->getUserById(param.user_id))
        ) << "Failed test: "
          << param.test_name;
        EXPECT_EQ(response_dto.user.getValueOfId(), param.user_id)
            << "Failed test: " << param.test_name;
        EXPECT_EQ(response_dto.user.getValueOfDisplayName(), param.display_name)
            << "Failed test: " << param.test_name;
        EXPECT_EQ(response_dto.user.getValueOfHandle(), param.user_handle)
            << "Failed test: " << param.test_name;
        EXPECT_EQ(
            response_dto.user.getValueOfPasswordHash(), param.password_hash
        ) << "Failed test: "
          << param.test_name;
    }
}

INSTANTIATE_TEST_SUITE_P(
    GetUserByIdTest,
    ServiceGetUserByIdTest,
    ::testing::Values(
        GetUserByIdTestCase{"User found", 67, "PIDOR", "AAAA", "chmo", true},
        GetUserByIdTestCase{
            "User not found", 67, "PIDOR", "AAAA", "chmo", false
        }
    )
);

struct GetUserByHandleTestCase {
    std::string test_name;
    int64_t user_id;
    std::string user_handle;
    std::string password_hash;
    std::string display_name;

    bool is_user_exists;
};

class ServiceGetUserByHandleTest
    : public ::testing::TestWithParam<GetUserByHandleTestCase> {
protected:
    std::shared_ptr<MockUserRepository> mock_user_repo;
    std::shared_ptr<api::v1::UserService> user_service;
    std::shared_ptr<MockPasswordHasher> mock_password_hasher;

    void SetUp() override {
        mock_user_repo = std::make_shared<MockUserRepository>();

        user_service = std::make_shared<api::v1::UserService>();
        user_service->setUserRepo(mock_user_repo);
        user_service->setChatRepo(nullptr);
    }
};

TEST_P(ServiceGetUserByHandleTest, GetUserByHandleTest) {
    auto param = GetParam();
    EXPECT_CALL(*mock_user_repo, getByHandle(param.user_handle))
        .WillRepeatedly(Invoke(
            [param](const std::string &user_handle
            ) -> drogon::Task<std::optional<User>> {
                if (param.is_user_exists) {
                    User fake_user;
                    fake_user.setId(param.user_id);
                    fake_user.setHandle(user_handle);
                    fake_user.setPasswordHash(param.password_hash);
                    fake_user.setDisplayName(param.display_name);
                    return createFakeTask<std::optional<User>>(fake_user);
                }
                return createFakeTask<std::optional<User>>(std::nullopt);
            }
        ));
    if (!param.is_user_exists) {
        EXPECT_THROW(
            drogon::sync_wait(user_service->getUserByHandle(param.user_handle)),
            messenger::exceptions::NotFoundException
        ) << "Failed test: "
          << param.test_name;
    } else {
        GetUserResponseDto response_dto;
        EXPECT_NO_THROW(
            response_dto = drogon::sync_wait(
                user_service->getUserByHandle(param.user_handle)
            )
        ) << "Failed test: "
          << param.test_name;
        EXPECT_EQ(response_dto.user.getValueOfId(), param.user_id)
            << "Failed test: " << param.test_name;
        EXPECT_EQ(response_dto.user.getValueOfDisplayName(), param.display_name)
            << "Failed test: " << param.test_name;
        EXPECT_EQ(response_dto.user.getValueOfHandle(), param.user_handle)
            << "Failed test: " << param.test_name;
        EXPECT_EQ(
            response_dto.user.getValueOfPasswordHash(), param.password_hash
        ) << "Failed test: "
          << param.test_name;
    }
}

INSTANTIATE_TEST_SUITE_P(
    GetUserByHandleTest,
    ServiceGetUserByHandleTest,
    ::testing::Values(
        GetUserByHandleTestCase{
            "User found", 67, "PIDOR", "AAAA", "chmo", true
        },
        GetUserByHandleTestCase{
            "User not found", 67, "PIDOR", "AAAA", "chmo", false
        }
    )
);

struct SearchUserTestCase {
    std::string test_name;
    SearchUserRequestDto request_dto;

    std::size_t correct_size;
};

class ServiceSearchUserTest
    : public ::testing::TestWithParam<SearchUserTestCase> {
protected:
    std::shared_ptr<MockUserRepository> mock_user_repo;
    std::shared_ptr<api::v1::UserService> user_service;
    std::shared_ptr<MockPasswordHasher> mock_password_hasher;

    void SetUp() override {
        mock_user_repo = std::make_shared<MockUserRepository>();

        user_service = std::make_shared<api::v1::UserService>();
        user_service->setUserRepo(mock_user_repo);
        user_service->setChatRepo(nullptr);
    }
};

TEST_P(ServiceSearchUserTest, SearchUserTest) {
    auto param = GetParam();
    EXPECT_CALL(
        *mock_user_repo,
        search(param.request_dto.query, param.request_dto.limit)
    )
        .WillRepeatedly(Invoke(
            [param](
                const std::string &query, int64_t limit
            ) -> drogon::Task<std::vector<User>> {
                std::vector<User> users(limit);
                return createFakeTask<std::vector<User>>(users);
            }
        ));
    auto response_dto =
        drogon::sync_wait(user_service->searchUser(param.request_dto));
    EXPECT_EQ(response_dto.users.size(), param.correct_size)
        << "Failed test: " << param.test_name;
}

INSTANTIATE_TEST_SUITE_P(
    SearchUserTest,
    ServiceSearchUserTest,
    ::testing::Values(SearchUserTestCase{"Correct limit", {"aaa", 67}, 67})
);