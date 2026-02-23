#include <drogon/HttpTypes.h>
#include <drogon/drogon.h>
#include <drogon/utils/coroutine.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <json/value.h>
#include <optional>
#include "controllers/api_v1_auth.h"
#include "controllers/api_v1_users.h"
#include "mocks/MockUserRepository.hpp"

using User = drogon_model::messenger_db::Users;
using ::testing::Invoke;

class UsersControllerTest : public ::testing::Test {
protected:
    std::shared_ptr<MockUserRepository> mock_user_repo;
    api::v1::users user_controller;
    api::v1::auth auth_controller;

    void SetUp() override {
        mock_user_repo = std::make_shared<MockUserRepository>();
        user_controller.setRepo(mock_user_repo);
        auth_controller.setRepo(mock_user_repo);
    }
};

TEST_F(UsersControllerTest, RegisterUserSuccess) {
    /*If user with given handle doesn't exist,
    create() will make new one and return true. Controller will return status
    code 201*/
    EXPECT_CALL(*mock_user_repo, getByHandle(testing::_))
        .WillOnce(Invoke([](std::string) -> drogon::Task<std::optional<User>> {
            co_return std::nullopt;
        }));
    EXPECT_CALL(*mock_user_repo, create(testing::_, testing::_, testing::_))
        .WillOnce(Invoke(
            [](std::string, std::string, std::string) -> drogon::Task<bool> {
                co_return true;
            }
        ));
    Json::Value request_json;
    request_json["handle"] = "pidoras";
    request_json["display_name"] = "gnusniy";
    request_json["password"] = "loh123321pidor";
    auto req = drogon::HttpRequest::newHttpJsonRequest(request_json);
    auto resp = drogon::sync_wait(auth_controller.registerUser(req));
    ASSERT_NE(resp, nullptr)
        << "Controller returned nullptr(just didn't return HttpResp)";
    EXPECT_EQ(resp->getStatusCode(), drogon::k201Created);

    auto expected_response_json = resp->getJsonObject();
    ASSERT_NE(expected_response_json, nullptr) << "Response is not valid json";
    EXPECT_EQ(
        (*expected_response_json)["message"].asString(),
        "New user was successfully created"
    );
}

TEST_F(UsersControllerTest, RegisterUserAlreadyExists) {
    /*If user with given handle already exists,
    create() will return false. Controller will return status code 400*/
    EXPECT_CALL(*mock_user_repo, getByHandle(testing::_))
        .WillOnce(Invoke([](std::string) -> drogon::Task<std::optional<User>> {
            co_return User();
        }));
    Json::Value request_json;
    request_json["handle"] = "pidoras";
    request_json["display_name"] = "gnusniy";
    request_json["password"] = "loh123321pidor";
    auto req = drogon::HttpRequest::newHttpJsonRequest(request_json);
    auto resp = drogon::sync_wait(auth_controller.registerUser(req));
    ASSERT_NE(resp, nullptr)
        << "Controller returned nullptr(just didn't return HttpResp)";
    EXPECT_EQ(resp->getStatusCode(), drogon::k409Conflict);

    auto expected_response_json = resp->getJsonObject();
    ASSERT_NE(expected_response_json, nullptr) << "Response is not valid json";
    EXPECT_EQ(
        (*expected_response_json)["message"].asString(), "User already exists"
    );
}

TEST_F(UsersControllerTest, RegisterUserMissedFields) {
    /*If request doesn't contain all required fields,
    controller should return status code 400*/
    Json::Value request_json;
    request_json["handle"] = "pidoras";
    request_json["display_name"] = "gnusniy";
    auto req = drogon::HttpRequest::newHttpJsonRequest(request_json);
    auto resp = drogon::sync_wait(auth_controller.registerUser(req));
    ASSERT_NE(resp, nullptr)
        << "Controller returned nullptr(just didn't return HttpResp)";
    EXPECT_EQ(resp->getStatusCode(), drogon::k400BadRequest);

    auto expected_response_json = resp->getJsonObject();
    ASSERT_NE(expected_response_json, nullptr) << "Response is not valid json";
    EXPECT_EQ(
        (*expected_response_json)["message"].asString(),
        "Invalid JSON: couldn't find some fields. Missing fields: password"
    );
}