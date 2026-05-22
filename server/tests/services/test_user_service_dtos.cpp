#include <drogon/HttpRequest.h>
#include <gtest/gtest.h>
#include <json/forwards.h>
#include <json/reader.h>
#include <json/value.h>
#include <memory>
#include <unordered_map>
#include <vector>
#include "dto/UserServiceDtos.hpp"
#include "gtest/gtest.h"

using namespace messenger::dto;

using User = drogon_model::messenger_db::Users;

// Request dtos

struct SearchUserRequestDtoTestCase {
    std::string test_name;
    std::unordered_map<std::string, std::string> query_params;

    std::string expected_query;
    int64_t expected_limit;
};

class SearchUserRequestDtoTest
    : public testing::TestWithParam<SearchUserRequestDtoTestCase> {};

TEST_P(SearchUserRequestDtoTest, CorrectlyParsesValidRequest) {
    auto param = GetParam();
    auto req = drogon::HttpRequest::newHttpRequest();
    for (const auto &[key, val] : param.query_params) {
        req->setParameter(key, val);
    }
    SearchUserRequestDto dto(req);

    EXPECT_EQ(dto.query, param.expected_query)
        << "Failed test: " << param.test_name;
    EXPECT_EQ(dto.limit, param.expected_limit)
        << "Failed test: " << param.test_name;
}

INSTANTIATE_TEST_SUITE_P(
    DtoTests,
    SearchUserRequestDtoTest,
    ::testing::Values(
        SearchUserRequestDtoTestCase{
            "Valid query and limit",
            {{"query", "pidor"}, {"limit", "67"}},
            "pidor",
            67
        },
        SearchUserRequestDtoTestCase{
            "Limit is greater than 100",
            {{"query", "pidor"}, {"limit", "228"}},
            "pidor",
            100
        },
        SearchUserRequestDtoTestCase{
            "Limit is less than 1",
            {{"query", "pidor"}, {"limit", "-100"}},
            "pidor",
            1
        },
        SearchUserRequestDtoTestCase{
            "No limit in params",
            {{"query", "pidor"}},
            "pidor",
            20
        },
        SearchUserRequestDtoTestCase{
            "Limit is not a number",
            {{"query", "pidor"}, {"limit", "bebebebe"}},
            "pidor",
            20
        }
    )
);

struct RegisterUserRequestDtoTestCase {
    std::string test_name;
    std::string str_json;

    std::string expected_handle;
    std::string expected_password;
    std::string expected_display_name;
};

class RegisterUserRequestDtoTest
    : public testing::TestWithParam<RegisterUserRequestDtoTestCase> {};

TEST_P(RegisterUserRequestDtoTest, CorrectlyParsesValidRequest) {
    auto param = GetParam();

    Json::Value json_body;
    std::istringstream s(param.str_json);
    ASSERT_TRUE(
        Json::parseFromStream(Json::CharReaderBuilder(), s, &json_body, nullptr)
    ) << "Wrong json";
    RegisterUserRequestDto dto(std::make_shared<Json::Value>(json_body));

    EXPECT_EQ(dto.handle, param.expected_handle)
        << "Failed test: " << param.test_name;
    EXPECT_EQ(dto.password, param.expected_password)
        << "Failed test: " << param.test_name;
    EXPECT_EQ(dto.display_name, param.expected_display_name)
        << "Failed test: " << param.test_name;
}

INSTANTIATE_TEST_SUITE_P(
    DtoTests,
    RegisterUserRequestDtoTest,
    ::testing::Values(
        RegisterUserRequestDtoTestCase{
            "Success",
            R"({"handle": "pidor", "password": "cool_pass", "display_name": "chmo"})",
            "pidor", "cool_pass", "chmo"
        },
        RegisterUserRequestDtoTestCase{
            "Missed field", R"({"handle": "pidor", "password": "cool_pass"})",
            "pidor", "cool_pass", ""
        }
    )
);

struct LoginUserRequestDtoTestCase {
    std::string test_name;
    std::string str_json;

    std::string expected_handle;
    std::string expected_password;
};

class LoginUserRequestDtoTest
    : public testing::TestWithParam<LoginUserRequestDtoTestCase> {};

TEST_P(LoginUserRequestDtoTest, CorrectlyParsesValidRequest) {
    auto param = GetParam();

    Json::Value json_body;
    std::istringstream s(param.str_json);
    ASSERT_TRUE(
        Json::parseFromStream(Json::CharReaderBuilder(), s, &json_body, nullptr)
    ) << "Wrong json";
    LoginUserRequestDto dto(std::make_shared<Json::Value>(json_body));

    EXPECT_EQ(dto.handle, param.expected_handle)
        << "Failed test: " << param.test_name;
    EXPECT_EQ(dto.password, param.expected_password)
        << "Failed test: " << param.test_name;
}

INSTANTIATE_TEST_SUITE_P(
    DtoTests,
    LoginUserRequestDtoTest,
    ::testing::Values(
        LoginUserRequestDtoTestCase{
            "Success", R"({"handle": "pidor", "password": "cool_pass"})",
            "pidor", "cool_pass"
        },
        LoginUserRequestDtoTestCase{
            "Missed field", R"({"handle": "pidor"})", "pidor", ""
        }
    )
);

// struct ***RequestDtoTestCase {
//     std::string test_name;
//     std::unordered_map<std::string, std::string> query_params;
//     std::string str_json;

//     *expected fields*
// };

// class ***RequestDtoTest
//     : public testing::TestWithParam<***RequestDtoTestCase> {};

// TEST_P(***RequestDtoTest, CorrectlyParsesValidRequest) {
//     auto param = GetParam();

//     *forming request/json*
//     auto req = drogon::HttpRequest::newHttpRequest();
//     for (const auto &[key, val] : param.query_params) {
//         req->setParameter(key, val);
//     }
//     Json::Value json_body;
//     std::istringstream s(param.str_json);
//     ASSERT_TRUE(Json::parseFromStream(Json::CharReaderBuilder(), s,
//     &json_body, nullptr))
//         << "Wrong json";
//     ***RequestDto dto(req);

//    *expections*
//     EXPECT_EQ(dto.query, param.expected_query)
//         << "Failed test: " << param.test_name;
// }

// INSTANTIATE_TEST_SUITE_P(
//     DtoTests,
//     ***RequestDtoTest,
//     ::testing::Values(
//         ***RequestDtoTestCase{
//             "Valid query and limit",
//             {{"query", "pidor"}, {"limit", "67"}},
//             "pidor",
//             67
//         }
//     )
// );

// Response dtos

struct UserData {
    std::string handle;
    std::string password_hash;
    std::string display_name;
    int64_t id;
};

struct SearchUserResponseDtoTestCase {
    std::string test_name;
    std::vector<UserData> users;
};

class SearchUserResponseDtoTest
    : public ::testing::TestWithParam<SearchUserResponseDtoTestCase> {};

TEST_P(SearchUserResponseDtoTest, CorrectlyBuildingJsonFromData) {
    auto param = GetParam();

    std::vector<User> users(param.users.size());
    for (std::size_t i = 0; i < users.size(); i++) {
        User user;
        user.setHandle(param.users[i].handle);
        user.setPasswordHash(param.users[i].password_hash);
        user.setDisplayName(param.users[i].display_name);
        user.setId(param.users[i].id);
        users[i] = user;
    }
    SearchUserResponseDto dto(std::move(users));
    Json::Value json_dto = dto.toJson();

    ASSERT_TRUE(json_dto.isArray()) << "Failed test: " << param.test_name;
    ASSERT_EQ(json_dto.size(), param.users.size())
        << "Failed test: " << param.test_name;
    for (Json::ArrayIndex i = 0; i < json_dto.size(); i++) {
        EXPECT_EQ(json_dto[i]["handle"], param.users[i].handle);
        EXPECT_FALSE(json_dto[i].isMember("password_hash"));
        EXPECT_EQ(json_dto[i]["display_name"], param.users[i].display_name);
        EXPECT_EQ(json_dto[i]["id"], param.users[i].id);
    }
}

INSTANTIATE_TEST_SUITE_P(
    DtoTests,
    SearchUserResponseDtoTest,
    ::testing::Values(
        SearchUserResponseDtoTestCase{
            "Success",
            {{"pidor1", "somehashbebebe", "CHMO1", 67},
             {"pidor2", "somehashbebebe2", "CHMO2", 69}}
        },
        SearchUserResponseDtoTestCase{"Empty list", {}}
    )
);

struct GetUserResponseDtoTestCase {
    std::string test_name;
    UserData user;
};

class GetUserResponseDtoTest
    : public ::testing::TestWithParam<GetUserResponseDtoTestCase> {};

TEST_P(GetUserResponseDtoTest, CorrectlyBuildingJsonFromData) {
    auto param = GetParam();

    User user;
    user.setHandle(param.user.handle);
    user.setPasswordHash(param.user.password_hash);
    user.setDisplayName(param.user.display_name);
    user.setId(param.user.id);
    GetUserResponseDto dto(std::move(user));
    Json::Value json_dto = dto.toJson();

    EXPECT_EQ(json_dto["handle"], param.user.handle);
    EXPECT_FALSE(json_dto.isMember("password_hash"));
    EXPECT_EQ(json_dto["display_name"], param.user.display_name);
    EXPECT_EQ(json_dto["id"], param.user.id);
}

INSTANTIATE_TEST_SUITE_P(
    DtoTests,
    GetUserResponseDtoTest,
    ::testing::Values(GetUserResponseDtoTestCase{
        "Success",
        {"pidor1", "somehashbebebe", "CHMO1", 67}
    })
);

TEST(RegisterUserResponseDtoTest, CorrectlyBuildingJson) {
    RegisterUserResponseDto dto;
    Json::Value json_dto = dto.toJson();
    EXPECT_EQ(json_dto["message"], "New user was successfully created");
}

struct LoginUserResponseDtoTestCase {
    std::string test_name;
    std::string token;
};

class LoginUserResponseDtoTest
    : public ::testing::TestWithParam<LoginUserResponseDtoTestCase> {};

TEST_P(LoginUserResponseDtoTest, CorrectlyBuildingJsonFromData) {
    auto param = GetParam();

    LoginUserResponseDto dto(param.token);
    Json::Value json_dto = dto.toJson();

    EXPECT_EQ(json_dto["message"], "Login successful");
    EXPECT_EQ(json_dto["token"], param.token);
}

INSTANTIATE_TEST_SUITE_P(
    DtoTests,
    LoginUserResponseDtoTest,
    ::testing::Values(LoginUserResponseDtoTestCase{
        "Success", "very_cool_token_bebebebe"
    })
);