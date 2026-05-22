#include <drogon/HttpRequest.h>
#include <gtest/gtest.h>
#include <json/forwards.h>
#include <json/reader.h>
#include <json/value.h>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>
#include "dto/ChatServiceDtos.hpp"
#include "gtest/gtest.h"

using namespace messenger::dto;

using User = drogon_model::messenger_db::Users;

// Request dtos

struct GetUserChatsRequestDtoTestCase {
    std::string test_name;
    int64_t from_request_user_id;
    int64_t from_token_user_id;

    int64_t expected_from_request_user_id;
    int64_t expected_from_token_user_id;
};

class GetUserChatsRequestDtoTest
    : public testing::TestWithParam<GetUserChatsRequestDtoTestCase> {};

TEST_P(GetUserChatsRequestDtoTest, CorrectlyParsesValidRequest) {
    auto param = GetParam();

    auto req = drogon::HttpRequest::newHttpRequest();
    req->getAttributes()->insert("user_id", param.from_token_user_id);
    GetUserChatsRequestDto dto(req, param.from_request_user_id);

    EXPECT_EQ(dto.from_token_user_id, param.expected_from_token_user_id)
        << "Failed test: " << param.test_name;
    EXPECT_EQ(dto.from_request_user_id, param.expected_from_request_user_id)
        << "Failed test: " << param.test_name;
}

INSTANTIATE_TEST_SUITE_P(
    DtoTests,
    GetUserChatsRequestDtoTest,
    ::testing::Values(GetUserChatsRequestDtoTestCase{"Success", 67, 69, 67, 69})
);

struct CreateOrGetDirectRequestDtoTestCase {
    std::string test_name;
    int64_t attribute_user_id;
    std::string str_json;

    int64_t expected_user_id;
    int64_t expected_target_user_id;
};

class CreateOrGetDirectRequestDtoTest
    : public testing::TestWithParam<CreateOrGetDirectRequestDtoTestCase> {};

TEST_P(CreateOrGetDirectRequestDtoTest, CorrectlyParsesValidRequest) {
    auto param = GetParam();

    auto req = drogon::HttpRequest::newHttpRequest();
    req->getAttributes()->insert("user_id", param.attribute_user_id);
    Json::Value json_body;
    std::istringstream s(param.str_json);
    ASSERT_TRUE(
        Json::parseFromStream(Json::CharReaderBuilder(), s, &json_body, nullptr)
    ) << "Wrong json";
    CreateOrGetDirectRequestDto dto(
        req, std::make_shared<Json::Value>(json_body)
    );

    EXPECT_EQ(dto.user_id, param.expected_user_id)
        << "Failed test: " << param.test_name;
    EXPECT_EQ(dto.target_user_id, param.expected_target_user_id)
        << "Failed test: " << param.test_name;
}

INSTANTIATE_TEST_SUITE_P(
    DtoTests,
    CreateOrGetDirectRequestDtoTest,
    ::testing::Values(CreateOrGetDirectRequestDtoTestCase{
        "Success", 67, R"({"target_user_id": 69})", 67, 69
    })
);

struct GetChatMessagesRequestDtoTestCase {
    std::string test_name;
    int64_t attribute_user_id;
    std::unordered_map<std::string, std::string> query_params;
    int64_t chat_id;

    int64_t expected_chat_id;
    int64_t expected_user_id;
    std::optional<int64_t> expected_before_message_id;
    int64_t expected_limit;
};

class GetChatMessagesRequestDtoTest
    : public testing::TestWithParam<GetChatMessagesRequestDtoTestCase> {};

TEST_P(GetChatMessagesRequestDtoTest, CorrectlyParsesValidRequest) {
    auto param = GetParam();

    auto req = drogon::HttpRequest::newHttpRequest();
    for (const auto &[key, val] : param.query_params) {
        req->setParameter(key, val);
    }
    req->getAttributes()->insert("user_id", param.attribute_user_id);
    GetChatMessagesRequestDto dto(req, param.chat_id);

    EXPECT_EQ(dto.chat_id, param.expected_chat_id)
        << "Failed test: " << param.test_name;
    EXPECT_EQ(dto.user_id, param.expected_user_id)
        << "Failed test: " << param.test_name;
    EXPECT_EQ(dto.before_message_id, param.expected_before_message_id)
        << "Failed test: " << param.test_name;
    EXPECT_EQ(dto.limit, param.expected_limit)
        << "Failed test: " << param.test_name;
}

INSTANTIATE_TEST_SUITE_P(
    DtoTests,
    GetChatMessagesRequestDtoTest,
    ::testing::Values(
        GetChatMessagesRequestDtoTestCase{
            "Success",
            67,
            {{"limit", "23"}, {"before_id", "12"}},
            228,
            228,
            67,
            12,
            23
        },
        GetChatMessagesRequestDtoTestCase{
            "Limit is greater than 100",
            67,
            {{"limit", "123"}, {"before_id", "12"}},
            228,
            228,
            67,
            12,
            100
        },
        GetChatMessagesRequestDtoTestCase{
            "Limit is less than 1",
            67,
            {{"limit", "-123"}, {"before_id", "12"}},
            228,
            228,
            67,
            12,
            1
        },
        GetChatMessagesRequestDtoTestCase{
            "Limit is not number",
            67,
            {{"limit", "bebee"}, {"before_id", "12"}},
            228,
            228,
            67,
            12,
            50
        },
        GetChatMessagesRequestDtoTestCase{
            "Limit is not set",
            67,
            {{"before_id", "12"}},
            228,
            228,
            67,
            12,
            50
        },
        GetChatMessagesRequestDtoTestCase{
            "Before_id is not set",
            67,
            {{"limit", "23"}},
            228,
            228,
            67,
            std::nullopt,
            23
        },
        GetChatMessagesRequestDtoTestCase{
            "Before_id is not number",
            67,
            {{"limit", "23"}, {"before_id", "bebebe"}},
            228,
            228,
            67,
            std::nullopt,
            23
        }
    )
);

struct SendMessageRequestDtoTestCase {
    std::string test_name;
    int64_t attribute_user_id;
    std::string str_json;
    int64_t chat_id;

    int64_t expected_user_id;
    int64_t expected_chat_id;
    std::string expected_text;
    std::string expected_message_type;
    std::optional<int64_t> expected_reply_to_id;
    std::optional<int64_t> expected_forward_from_id;
    std::vector<std::string> expected_attachment_tokens;
};

class SendMessageRequestDtoTest
    : public testing::TestWithParam<SendMessageRequestDtoTestCase> {};

TEST_P(SendMessageRequestDtoTest, CorrectlyParsesValidRequest) {
    auto param = GetParam();

    auto req = drogon::HttpRequest::newHttpRequest();
    req->getAttributes()->insert("user_id", param.attribute_user_id);
    Json::Value json_body;
    std::istringstream s(param.str_json);
    ASSERT_TRUE(
        Json::parseFromStream(Json::CharReaderBuilder(), s, &json_body, nullptr)
    ) << "Wrong json";
    SendMessageRequestDto dto(
        req, std::make_shared<Json::Value>(json_body), param.chat_id
    );

    EXPECT_EQ(dto.user_id, param.expected_user_id)
        << "Failed test: " << param.test_name;
    EXPECT_EQ(dto.chat_id, param.expected_chat_id)
        << "Failed test: " << param.test_name;
    EXPECT_EQ(dto.text, param.expected_text)
        << "Failed test: " << param.test_name;
    EXPECT_EQ(dto.message_type, param.expected_message_type)
        << "Failed test: " << param.test_name;
    EXPECT_EQ(dto.reply_to_id, param.expected_reply_to_id)
        << "Failed test: " << param.test_name;
    EXPECT_EQ(dto.forward_from_id, param.expected_forward_from_id)
        << "Failed test: " << param.test_name;
    EXPECT_EQ(dto.attachment_tokens, param.expected_attachment_tokens)
        << "Failed test: " << param.test_name;
}

INSTANTIATE_TEST_SUITE_P(
    DtoTests,
    SendMessageRequestDtoTest,
    ::testing::Values(
        SendMessageRequestDtoTestCase{
            "All fields",

            67,
            R"({"text": "Some text", "reply_to_id": 1, "forward_from_id": 2, "type": "Text", "attachment_tokens": ["token1", "token2", "token3"]})",
            69,

            67,
            69,
            "Some text",
            "Text",
            1,
            2,
            {"token1", "token2", "token3"}
        },
        SendMessageRequestDtoTestCase{
            "Without tokens",

            67,
            R"({"text": "Some text", "reply_to_id": 1, "forward_from_id": 2, "type": "Text"})",
            69,

            67,
            69,
            "Some text",
            "Text",
            1,
            2,
            {}
        },
        SendMessageRequestDtoTestCase{
            "Attachment_tokens field is not array",

            67,
            R"({"text": "Some text", "reply_to_id": 1, "forward_from_id": 2, "type": "Text", "attachment_tokens": {"wtf": "bebebe"}})",
            69,

            67,
            69,
            "Some text",
            "Text",
            1,
            2,
            {}
        },
        SendMessageRequestDtoTestCase{
            "Reply_to_id is missed",

            67,
            R"({"text": "Some text", "forward_from_id": 2, "type": "Text", "attachment_tokens": ["token1", "token2", "token3"]})",
            69,

            67,
            69,
            "Some text",
            "Text",
            std::nullopt,
            2,
            {"token1", "token2", "token3"}
        },
        SendMessageRequestDtoTestCase{
            "Forward_from_id is missed",

            67,
            R"({"text": "Some text", "reply_to_id": 1, "type": "Text", "attachment_tokens": ["token1", "token2", "token3"]})",
            69,

            67,
            69,
            "Some text",
            "Text",
            1,
            std::nullopt,
            {"token1", "token2", "token3"}
        }
    )
);

struct ReadMessagesRequestDtoTestCase {
    std::string test_name;
    int64_t attribute_user_id;
    std::string str_json;
    int64_t chat_id;

    int64_t expected_user_id;
    int64_t expected_chat_id;
    int64_t expected_last_read_message_id;
};

class ReadMessagesRequestDtoTest
    : public testing::TestWithParam<ReadMessagesRequestDtoTestCase> {};

TEST_P(ReadMessagesRequestDtoTest, CorrectlyParsesValidRequest) {
    auto param = GetParam();

    auto req = drogon::HttpRequest::newHttpRequest();
    req->getAttributes()->insert("user_id", param.attribute_user_id);
    Json::Value json_body;
    std::istringstream s(param.str_json);
    ASSERT_TRUE(
        Json::parseFromStream(Json::CharReaderBuilder(), s, &json_body, nullptr)
    ) << "Wrong json";
    ReadMessagesRequestDto dto(
        req, std::make_shared<Json::Value>(json_body), param.chat_id
    );

    EXPECT_EQ(dto.user_id, param.expected_user_id)
        << "Failed test: " << param.test_name;
    EXPECT_EQ(dto.chat_id, param.expected_chat_id)
        << "Failed test: " << param.test_name;
    EXPECT_EQ(dto.last_read_message_id, param.expected_last_read_message_id)
        << "Failed test: " << param.test_name;
}

INSTANTIATE_TEST_SUITE_P(
    DtoTests,
    ReadMessagesRequestDtoTest,
    ::testing::Values(ReadMessagesRequestDtoTestCase{
        "Success", 67, R"({"last_read_message_id": 123})", 69, 67, 69, 123
    })
);

struct GetMessageByIdRequestDtoTestCase {
    std::string test_name;
    int64_t attribute_user_id;
    int64_t message_id;

    int64_t expected_message_id;
    int64_t expected_user_id;
};

class GetMessageByIdRequestDtoTest
    : public testing::TestWithParam<GetMessageByIdRequestDtoTestCase> {};

TEST_P(GetMessageByIdRequestDtoTest, CorrectlyParsesValidRequest) {
    auto param = GetParam();

    auto req = drogon::HttpRequest::newHttpRequest();
    req->getAttributes()->insert("user_id", param.attribute_user_id);
    GetMessageByIdRequestDto dto(req, param.message_id);

    EXPECT_EQ(dto.message_id, param.expected_message_id)
        << "Failed test: " << param.test_name;
    EXPECT_EQ(dto.user_id, param.expected_user_id)
        << "Failed test: " << param.test_name;
}

INSTANTIATE_TEST_SUITE_P(
    DtoTests,
    GetMessageByIdRequestDtoTest,
    ::testing::Values(GetMessageByIdRequestDtoTestCase{
        "Success", 67, 69, 69, 67
    })
);

struct GetAttachmentLinksRequestDtoTestCase {
    std::string test_name;
    int64_t attribute_user_id;
    std::string str_json;

    int64_t expected_user_id;
    int64_t expected_chat_id;
    std::string expected_message_type;
    std::vector<AttachmentRequestDto> expected_files;
};

class GetAttachmentLinksRequestDtoTest
    : public testing::TestWithParam<GetAttachmentLinksRequestDtoTestCase> {};

TEST_P(GetAttachmentLinksRequestDtoTest, CorrectlyParsesValidRequest) {
    auto param = GetParam();

    auto req = drogon::HttpRequest::newHttpRequest();
    req->getAttributes()->insert("user_id", param.attribute_user_id);
    Json::Value json_body;
    std::istringstream s(param.str_json);
    ASSERT_TRUE(
        Json::parseFromStream(Json::CharReaderBuilder(), s, &json_body, nullptr)
    ) << "Wrong json";
    GetAttachmentLinksRequestDto dto(
        req, std::make_shared<Json::Value>(json_body)
    );

    EXPECT_EQ(dto.user_id, param.expected_user_id)
        << "Failed test: " << param.test_name;
    EXPECT_EQ(dto.chat_id, param.expected_chat_id)
        << "Failed test: " << param.test_name;
    EXPECT_EQ(dto.message_type, param.expected_message_type)
        << "Failed test: " << param.test_name;

    ASSERT_EQ(dto.files.size(), param.expected_files.size());
    for (std::size_t i = 0; i < dto.files.size(); i++) {
        EXPECT_EQ(
            dto.files[i].file_size_bytes,
            param.expected_files[i].file_size_bytes
        ) << "Failed test: "
          << param.test_name;
        EXPECT_EQ(
            dto.files[i].original_filename,
            param.expected_files[i].original_filename
        ) << "Failed test: "
          << param.test_name;
    }
}

INSTANTIATE_TEST_SUITE_P(
    DtoTests,
    GetAttachmentLinksRequestDtoTest,
    ::testing::Values(GetAttachmentLinksRequestDtoTestCase{
        "Success",
        67,
        R"({"chat_id": 1, "message_type": "Text", "files": [{"original_filename": "file1.png", "file_size_bytes": 128}, {"original_filename": "file2.png", "file_size_bytes": 256}]})",
        67,
        1,
        "Text",
        {{"file1.png", 128}, {"file2.png", 256}}
    })
);

struct CreateGroupRequestDtoTestCase {
    std::string test_name;
    int64_t attribute_user_id;
    std::string str_json;

    int64_t expected_creator_id;
    std::string expected_name;
    std::vector<int64_t> expected_members_ids;
};

class CreateGroupRequestDtoTest
    : public testing::TestWithParam<CreateGroupRequestDtoTestCase> {};

TEST_P(CreateGroupRequestDtoTest, CorrectlyParsesValidRequest) {
    auto param = GetParam();

    auto req = drogon::HttpRequest::newHttpRequest();
    req->getAttributes()->insert("user_id", param.attribute_user_id);

    Json::Value json_body;
    std::istringstream s(param.str_json);
    ASSERT_TRUE(
        Json::parseFromStream(Json::CharReaderBuilder(), s, &json_body, nullptr)
    ) << "Wrong json in test: "
      << param.test_name;

    CreateGroupRequestDto dto(req, std::make_shared<Json::Value>(json_body));

    EXPECT_EQ(dto.creator_id, param.expected_creator_id)
        << "Failed test: " << param.test_name;
    EXPECT_EQ(dto.name, param.expected_name)
        << "Failed test: " << param.test_name;
    EXPECT_EQ(dto.members_ids.size(), param.expected_members_ids.size())
        << "Failed test: " << param.test_name;

    for (size_t i = 0; i < dto.members_ids.size(); ++i) {
        EXPECT_EQ(dto.members_ids[i], param.expected_members_ids[i])
            << "Failed test: " << param.test_name << " at index " << i;
    }
}

INSTANTIATE_TEST_SUITE_P(
    DtoTests,
    CreateGroupRequestDtoTest,
    ::testing::Values(
        CreateGroupRequestDtoTestCase{
            "Success with full members list",
            67,
            R"({"chat_name": "C++ Enjoyers", "members": [10, 20, 30]})",
            67,
            "C++ Enjoyers",
            {10, 20, 30}
        },
        CreateGroupRequestDtoTestCase{
            "Success with empty members list",
            67,
            R"({"chat_name": "Solo Chat", "members": []})",
            67,
            "Solo Chat",
            {}
        },
        CreateGroupRequestDtoTestCase{
            "Success without members field",
            67,
            R"({"chat_name": "Secret Chat"})",
            67,
            "Secret Chat",
            {}
        }
    )
);

struct AddGroupChatMemberRequestDtoTestCase {
    std::string test_name;
    int64_t attribute_user_id;
    int64_t chat_id;
    std::string str_json;

    int64_t expected_user_id;
    int64_t expected_chat_id;
    int64_t expected_new_member_id;
    std::string expected_role;
};

class AddGroupChatMemberRequestDtoTest
    : public testing::TestWithParam<AddGroupChatMemberRequestDtoTestCase> {};

TEST_P(AddGroupChatMemberRequestDtoTest, CorrectlyParsesValidRequest) {
    auto param = GetParam();

    Json::Value json_body;
    std::istringstream s(param.str_json);
    ASSERT_TRUE(
        Json::parseFromStream(Json::CharReaderBuilder(), s, &json_body, nullptr)
    );

    auto req = drogon::HttpRequest::newHttpJsonRequest(json_body);
    req->getAttributes()->insert("user_id", param.attribute_user_id);

    AddGroupChatMemberRequestDto dto(req, param.chat_id);

    EXPECT_EQ(dto.user_id, param.expected_user_id);
    EXPECT_EQ(dto.chat_id, param.expected_chat_id);
    EXPECT_EQ(dto.new_member_id, param.expected_new_member_id);
    EXPECT_EQ(dto.role, param.expected_role);
}

INSTANTIATE_TEST_SUITE_P(
    DtoTests,
    AddGroupChatMemberRequestDtoTest,
    ::testing::Values(AddGroupChatMemberRequestDtoTestCase{
        "Success parsing", 10, 42, R"({"user_id": 99, "role": "Admin"})", 10,
        42, 99, "Admin"
    })
);

struct UpdateChatInfoRequestDtoTestCase {
    std::string test_name;
    int64_t attribute_user_id;
    int64_t chat_id;
    std::string str_json;

    std::optional<std::string> expected_name;
    std::optional<std::string> expected_avatar;
    std::optional<std::string> expected_description;
};

class UpdateChatInfoRequestDtoTest
    : public testing::TestWithParam<UpdateChatInfoRequestDtoTestCase> {};

TEST_P(UpdateChatInfoRequestDtoTest, CorrectlyParsesOptionals) {
    auto param = GetParam();
    auto req = drogon::HttpRequest::newHttpRequest();
    req->getAttributes()->insert("user_id", param.attribute_user_id);

    Json::Value json_body;
    std::istringstream s(param.str_json);
    ASSERT_TRUE(
        Json::parseFromStream(Json::CharReaderBuilder(), s, &json_body, nullptr)
    );

    UpdateChatInfoRequestDto dto(
        req, std::make_shared<Json::Value>(json_body), param.chat_id
    );

    EXPECT_EQ(dto.user_id, param.attribute_user_id);
    EXPECT_EQ(dto.chat_id, param.chat_id);
    EXPECT_EQ(dto.name, param.expected_name);
    EXPECT_EQ(dto.avatar, param.expected_avatar);
    EXPECT_EQ(dto.description, param.expected_description);
}

INSTANTIATE_TEST_SUITE_P(
    DtoTests,
    UpdateChatInfoRequestDtoTest,
    ::testing::Values(
        UpdateChatInfoRequestDtoTestCase{
            "All fields provided", 1, 2,
            R"({"name": "New Name", "avatar": "path/to/ava.png", "description": "Cool chat"})",
            "New Name", "path/to/ava.png", "Cool chat"
        },
        UpdateChatInfoRequestDtoTestCase{
            "Only description provided", 1, 2,
            R"({"description": "Only desc"})", std::nullopt, std::nullopt,
            "Only desc"
        },
        UpdateChatInfoRequestDtoTestCase{
            "Empty JSON", 1, 2, R"({})", std::nullopt, std::nullopt,
            std::nullopt
        }
    )
);

TEST(GetChatMembersRequestDtoTest, CorrectlyParsesPathAndAttributes) {
    auto req = drogon::HttpRequest::newHttpRequest();
    req->getAttributes()->insert("user_id", (int64_t)777);

    GetChatMembersRequestDto dto(req, 42);

    EXPECT_EQ(dto.user_id, 777);
    EXPECT_EQ(dto.chat_id, 42);
}

TEST(GetChatMemberRequestDtoTest, CorrectlyParsesPathAndAttributes) {
    auto req = drogon::HttpRequest::newHttpRequest();
    req->getAttributes()->insert("user_id", (int64_t)777);

    GetChatMemberRequestDto dto(req, 42, 99);

    EXPECT_EQ(dto.user_id, 777);
    EXPECT_EQ(dto.chat_id, 42);
    EXPECT_EQ(dto.member_id, 99);
}

TEST(RemoveMemberRequestDtoTest, CorrectlyParsesPathAndAttributes) {
    auto req = drogon::HttpRequest::newHttpRequest();
    req->getAttributes()->insert("user_id", (int64_t)777);

    RemoveMemberRequestDto dto(req, 42, 99);

    EXPECT_EQ(dto.user_id, 777);
    EXPECT_EQ(dto.chat_id, 42);
    EXPECT_EQ(dto.member_id, 99);
}

TEST(GetChatByIdRequestDtoTest, CorrectlyParsesPathAndAttributes) {
    auto req = drogon::HttpRequest::newHttpRequest();
    req->getAttributes()->insert("user_id", (int64_t)777);

    GetChatByIdRequestDto dto(req, 42);

    EXPECT_EQ(dto.user_id, 777);
    EXPECT_EQ(dto.chat_id, 42);
}

struct UpdateMemberRoleRequestDtoTestCase {
    std::string test_name;
    int64_t attribute_user_id;
    int64_t chat_id;
    int64_t member_id;
    std::string str_json;

    int64_t expected_user_id;
    int64_t expected_chat_id;
    int64_t expected_member_id;
    std::string expected_role;
};

class UpdateMemberRoleRequestDtoTest
    : public testing::TestWithParam<UpdateMemberRoleRequestDtoTestCase> {};

TEST_P(UpdateMemberRoleRequestDtoTest, CorrectlyParsesValidRequest) {
    auto param = GetParam();
    auto req = drogon::HttpRequest::newHttpRequest();
    req->getAttributes()->insert("user_id", param.attribute_user_id);

    Json::Value json_body;
    std::istringstream s(param.str_json);
    ASSERT_TRUE(
        Json::parseFromStream(Json::CharReaderBuilder(), s, &json_body, nullptr)
    );

    UpdateMemberRoleRequestDto dto(
        req, std::make_shared<Json::Value>(json_body), param.chat_id,
        param.member_id
    );

    EXPECT_EQ(dto.user_id, param.expected_user_id);
    EXPECT_EQ(dto.chat_id, param.expected_chat_id);
    EXPECT_EQ(dto.member_id, param.expected_member_id);
    EXPECT_EQ(dto.new_role, param.expected_role);
}

INSTANTIATE_TEST_SUITE_P(
    DtoTests,
    UpdateMemberRoleRequestDtoTest,
    ::testing::Values(UpdateMemberRoleRequestDtoTestCase{
        "Success parsing", 10, 42, 99, R"({"role": "Admin"})", 10, 42, 99,
        "Admin"
    })
);

// struct ***RequestDtoTestCase {
//     std::string test_name;
//     int64_t attribute_user_id;
//     std::unordered_map<std::string, std::string> query_params;
//     std::string str_json;

// };

// class ***RequestDtoTest
//     : public testing::TestWithParam<***RequestDtoTestCase> {};

// TEST_P(***RequestDtoTest, CorrectlyParsesValidRequest) {
//     auto param = GetParam();

//     auto req = drogon::HttpRequest::newHttpRequest();
//     for (const auto &[key, val] : param.query_params) {
//         req->setParameter(key, val);
//     }
//     req->getAttributes()->insert("user_id", param.attribute_user_id);
//     Json::Value json_body;
//     std::istringstream s(param.str_json);
//     ASSERT_TRUE(Json::parseFromStream(Json::CharReaderBuilder(), s,
//     &json_body, nullptr))
//         << "Wrong json";
//     ***RequestDto dto(req);

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

struct AttachmentInfo {
    int64_t id;
    std::string file_name;
    int64_t file_size_bytes;
    int64_t message_id;
};

struct UserInfo {
    int64_t id;
    std::string display_name;
    std::optional<std::string> avatar_path;
};

struct MessageInfo {
    int64_t id;
    int64_t chat_id;
    std::string text;
    std::optional<UserInfo> sender_info;
};

struct ChatPreviewInfo {
    int64_t chat_id;
    std::string title;
    std::optional<std::string> avatar_path;
    std::optional<MessageInfo> last_message;
    int64_t unread_count;
    std::string type;
};

struct GetUserChatsResponseDtoTestCase {
    std::string test_name;
    std::vector<ChatPreviewInfo> chats_previews;
    std::vector<std::vector<AttachmentInfo>> last_message_attachments;
};

class GetUserChatsResponseDtoTest
    : public ::testing::TestWithParam<GetUserChatsResponseDtoTestCase> {};

TEST_P(GetUserChatsResponseDtoTest, CorrectlyBuildingJsonFromData) {
    auto param = GetParam();

    std::vector<std::vector<Attachment>> attachments(
        param.last_message_attachments.size()
    );
    std::size_t i = 0;
    for (const auto &att_vector : param.last_message_attachments) {
        for (const auto &att : att_vector) {
            Attachment fake_att;
            fake_att.setId(att.id);
            fake_att.setFileName(att.file_name);
            fake_att.setFileSizeBytes(att.file_size_bytes);
            fake_att.setMessageId(att.message_id);
            attachments[i].push_back(fake_att);
        }
        i++;
    }

    std::vector<ChatPreview> chats_previews;
    std::vector<std::optional<User>> last_messages_senders;
    chats_previews.reserve(param.chats_previews.size());
    last_messages_senders.reserve(param.chats_previews.size());
    for (const auto &preview : param.chats_previews) {
        ChatPreview fake_preview;
        fake_preview.chat_id = preview.chat_id;
        fake_preview.avatar_path = preview.avatar_path;
        fake_preview.title = preview.title;
        fake_preview.type = preview.type;
        fake_preview.unread_count = preview.unread_count;
        if (preview.last_message.has_value()) {
            Message fake_msg;
            fake_msg.setId(preview.last_message->id);
            fake_msg.setChatId(preview.last_message->chat_id);
            fake_msg.setText(preview.last_message->text);
            fake_preview.last_message = fake_msg;

            if (preview.last_message->sender_info.has_value()) {
                User fake_user;
                fake_user.setId(preview.last_message->sender_info->id);
                fake_user.setDisplayName(
                    preview.last_message->sender_info->display_name
                );
                last_messages_senders.push_back(fake_user);
            } else {
                last_messages_senders.push_back(std::nullopt);
            }
        } else {
            fake_preview.last_message = std::nullopt;
            last_messages_senders.push_back(std::nullopt);
        }
        chats_previews.push_back(fake_preview);
    }
    GetUserChatsResponseDto dto(
        std::move(chats_previews), std::move(attachments),
        std::move(last_messages_senders)
    );
    Json::Value json_dto = dto.toJson();
    ASSERT_TRUE(json_dto.isMember("chats"));
    ASSERT_TRUE(json_dto["chats"].isArray());
    for (Json::ArrayIndex i = 0; i < json_dto["chats"].size(); i++) {
        EXPECT_EQ(
            json_dto["chats"][i]["chat_id"], param.chats_previews[i].chat_id
        );
        EXPECT_EQ(json_dto["chats"][i]["title"], param.chats_previews[i].title);
        EXPECT_EQ(
            json_dto["chats"][i]["unread_count"],
            param.chats_previews[i].unread_count
        );
        EXPECT_EQ(json_dto["chats"][i]["type"], param.chats_previews[i].type);
        if (param.chats_previews[i].avatar_path.has_value()) {
            EXPECT_EQ(
                json_dto["chats"][i]["avatar_path"],
                param.chats_previews[i].avatar_path.value()
            );
        } else {
            EXPECT_EQ(json_dto["chats"][i]["avatar_path"], "");
        }
        if (!param.chats_previews[i].last_message.has_value()) {
            EXPECT_TRUE(json_dto["chats"][i]["last_message"].isNull());
        } else {
            EXPECT_EQ(
                json_dto["chats"][i]["last_message"]["id"],
                param.chats_previews[i].last_message->id
            );
            EXPECT_EQ(
                json_dto["chats"][i]["last_message"]["chat_id"],
                param.chats_previews[i].last_message->chat_id
            );
            EXPECT_EQ(
                json_dto["chats"][i]["last_message"]["chat_id"],
                param.chats_previews[i].chat_id
            );
            EXPECT_EQ(
                json_dto["chats"][i]["last_message"]["text"],
                param.chats_previews[i].last_message->text
            );
            ASSERT_TRUE(
                json_dto["chats"][i]["last_message"].isMember("attachments")
            );
            ASSERT_TRUE(
                json_dto["chats"][i]["last_message"]["attachments"].isArray()
            );
            EXPECT_EQ(
                json_dto["chats"][i]["last_message"]["attachments"].size(),
                param.last_message_attachments[i].size()
            );
            ASSERT_TRUE(
                json_dto["chats"][i]["last_message"].isMember("sender_info")
            );
            if (param.chats_previews[i].last_message->sender_info.has_value()) {
                auto expected_sender =
                    param.chats_previews[i].last_message->sender_info.value();
                auto json_sender =
                    json_dto["chats"][i]["last_message"]["sender_info"];
                EXPECT_EQ(json_sender["id"], expected_sender.id);
                EXPECT_EQ(
                    json_sender["display_name"], expected_sender.display_name
                );
            } else {
                EXPECT_TRUE(
                    json_dto["chats"][i]["last_message"]["sender_info"].isNull()
                );
            }
            for (Json::ArrayIndex j = 0;
                 j < param.last_message_attachments[i].size(); j++) {
                Json::Value att_info =
                    json_dto["chats"][i]["last_message"]["attachments"][j];
                EXPECT_EQ(
                    att_info["id"], param.last_message_attachments[i][j].id
                );
                EXPECT_EQ(
                    att_info["file_name"],
                    param.last_message_attachments[i][j].file_name
                );
                EXPECT_EQ(
                    att_info["file_size_bytes"],
                    param.last_message_attachments[i][j].file_size_bytes
                );
                EXPECT_EQ(
                    att_info["message_id"],
                    param.last_message_attachments[i][j].message_id
                );
            }
        }
    }
}

INSTANTIATE_TEST_SUITE_P(
    DtoTests,
    GetUserChatsResponseDtoTest,
    ::testing::Values(
        GetUserChatsResponseDtoTestCase{
            "Rich data: messages, attachments, optionals",
            {ChatPreviewInfo{
                 1, "C++ Developers", "media/cpp_ava.png",
                 MessageInfo{
                     100, 1, "Look what bug I found!",
                     UserInfo{777, "Test User", std::nullopt}
                 },
                 3, "group"
             },
             ChatPreviewInfo{
                 2, "Alesha", std::nullopt,
                 MessageInfo{
                     101, 2, "Hello",
                     UserInfo{888, "Alesha", "media/alesha_ava.png"}
                 },
                 0, "dialog"
             }},
            {{AttachmentInfo{50, "bug_report.txt", 1024, 100},
              AttachmentInfo{51, "screen.png", 2048, 100}},
             {}}
        },
        GetUserChatsResponseDtoTestCase{
            "New chat without messages",
            {ChatPreviewInfo{
                3, "Secret chat", std::nullopt, std::nullopt, 0, "direct"
            }},
            {{}}
        },
        GetUserChatsResponseDtoTestCase{"Empty list of chats", {}, {}}
    )
);

struct ChatInfo {
    int64_t id;
    std::string type;
};

struct CreateOrGetDirectResponseDtoTestCase {
    std::string test_name;

    ChatInfo chat;
};

class CreateOrGetDirectResponseDtoTest
    : public ::testing::TestWithParam<CreateOrGetDirectResponseDtoTestCase> {};

TEST_P(CreateOrGetDirectResponseDtoTest, CorrectlyBuildingJsonFromData) {
    auto param = GetParam();
    Chat chat;
    chat.setId(param.chat.id);
    chat.setType(param.chat.type);

    CreateOrGetDirectResponseDto dto(std::move(chat), true);
    Json::Value dto_json = dto.toJson();
    EXPECT_TRUE(dto_json.isMember("chat"));
    EXPECT_EQ(dto_json["chat"]["id"], param.chat.id);
    EXPECT_EQ(dto_json["chat"]["type"], param.chat.type);
}

INSTANTIATE_TEST_SUITE_P(
    DtoTests,
    CreateOrGetDirectResponseDtoTest,
    ::testing::Values(
        CreateOrGetDirectResponseDtoTestCase{"Success", {67, "Direct"}}
    )
);

struct GetChatMessagesResponseDtoTestCase {
    std::string test_name;
    std::vector<MessageInfo> messages;
    std::vector<std::vector<AttachmentInfo>> attachments;
    std::vector<std::vector<std::optional<std::string>>>
        attachments_download_urls;
};

class GetChatMessagesResponseDtoTest
    : public ::testing::TestWithParam<GetChatMessagesResponseDtoTestCase> {};

TEST_P(GetChatMessagesResponseDtoTest, CorrectlyBuildingJsonFromData) {
    auto param = GetParam();

    std::vector<Message> messages;
    std::vector<User> senders_info;
    messages.reserve(param.messages.size());
    senders_info.reserve(param.messages.size());
    for (const auto &msg_info : param.messages) {
        Message msg;
        msg.setId(msg_info.id);
        msg.setChatId(msg_info.chat_id);
        msg.setText(msg_info.text);
        messages.push_back(msg);

        if (msg_info.sender_info.has_value()) {
            User fake_user;
            fake_user.setId(msg_info.sender_info->id);
            fake_user.setDisplayName(msg_info.sender_info->display_name);
            senders_info.push_back(fake_user);
        } else {
            senders_info.push_back(User{});
        }
    }

    std::vector<std::vector<Attachment>> attachments(param.attachments.size());
    for (std::size_t i = 0; i < param.attachments.size(); i++) {
        for (const auto &att : param.attachments[i]) {
            Attachment fake_att;
            fake_att.setId(att.id);
            fake_att.setFileName(att.file_name);
            fake_att.setFileSizeBytes(att.file_size_bytes);
            fake_att.setMessageId(att.message_id);
            attachments[i].push_back(fake_att);
        }
    }

    GetChatMessagesResponseDto dto(
        std::move(messages), std::move(attachments),
        param.attachments_download_urls, std::move(senders_info)
    );
    Json::Value json_dto = dto.toJson();

    ASSERT_TRUE(json_dto.isMember("messages"));
    ASSERT_TRUE(json_dto["messages"].isArray());
    ASSERT_EQ(json_dto["messages"].size(), param.messages.size());
    for (Json::ArrayIndex i = 0; i < param.messages.size(); i++) {
        Json::Value msg = json_dto["messages"][i];
        EXPECT_EQ(msg["id"], param.messages[i].id);
        EXPECT_EQ(msg["chat_id"], param.messages[i].chat_id);
        EXPECT_EQ(msg["text"], param.messages[i].text);
        ASSERT_TRUE(msg.isMember("attachments"));
        ASSERT_TRUE(msg["attachments"].isArray());
        ASSERT_EQ(msg["attachments"].size(), param.attachments[i].size());
        ASSERT_EQ(
            msg["attachments"].size(), param.attachments_download_urls[i].size()
        );
        for (Json::ArrayIndex j = 0; j < msg["attachments"].size(); j++) {
            EXPECT_EQ(msg["attachments"][j]["id"], param.attachments[i][j].id);
            EXPECT_EQ(
                msg["attachments"][j]["file_name"],
                param.attachments[i][j].file_name
            );
            EXPECT_EQ(
                msg["attachments"][j]["file_size_bytes"],
                param.attachments[i][j].file_size_bytes
            );
            EXPECT_EQ(
                msg["attachments"][j]["message_id"],
                param.attachments[i][j].message_id
            );
            if (param.attachments_download_urls[i][j].has_value()) {
                EXPECT_EQ(
                    msg["attachments"][j]["download_url"],
                    param.attachments_download_urls[i][j].value()
                );
            } else {
                EXPECT_EQ(msg["attachments"][j]["download_url"], "");
            }
        }
        ASSERT_TRUE(msg.isMember("sender_info"));
        if (param.messages[i].sender_info.has_value()) {
            EXPECT_EQ(
                msg["sender_info"]["id"], param.messages[i].sender_info->id
            );
            EXPECT_EQ(
                msg["sender_info"]["display_name"],
                param.messages[i].sender_info->display_name
            );
        }
    }
}

INSTANTIATE_TEST_SUITE_P(
    DtoTests,
    GetChatMessagesResponseDtoTest,
    ::testing::Values(
        GetChatMessagesResponseDtoTestCase{
            "Rich data: messages with and without attachments",
            {MessageInfo{
                 100, 1, "Here are the files you requested",
                 UserInfo{777, "Alice", "media/alice_ava.png"}
             },
             MessageInfo{101, 1, "Thanks!", UserInfo{888, "Bob", std::nullopt}}
            },
            {{AttachmentInfo{50, "report.pdf", 15000, 100},
              AttachmentInfo{51, "meme.jpg", 3000, 100}},
             {}},
            {{"https://s3.myproject.com/files/report.pdf", std::nullopt}, {}}
        },
        GetChatMessagesResponseDtoTestCase{
            "Message without attachments",
            {MessageInfo{102, 2, "Just a plain text message", std::nullopt}},
            {{}},
            {{}}
        },
        GetChatMessagesResponseDtoTestCase{"Empty messages list", {}, {}, {}}
    )
);

struct SendMessageResponseDtoTestCase {
    std::string test_name;
    MessageInfo message;
    std::vector<AttachmentInfo> attachments;
    std::vector<std::optional<std::string>> attachments_download_urls;
    UserInfo sender_info;
};

class SendMessageResponseDtoTest
    : public ::testing::TestWithParam<SendMessageResponseDtoTestCase> {};

TEST_P(SendMessageResponseDtoTest, CorrectlyBuildingJsonFromData) {
    auto param = GetParam();

    Message msg;
    msg.setId(param.message.id);
    msg.setChatId(param.message.chat_id);
    msg.setText(param.message.text);

    std::vector<Attachment> attachments;
    attachments.reserve(param.attachments.size());
    for (const auto &att_info : param.attachments) {
        Attachment fake_att;
        fake_att.setId(att_info.id);
        fake_att.setFileName(att_info.file_name);
        fake_att.setFileSizeBytes(att_info.file_size_bytes);
        fake_att.setMessageId(att_info.message_id);
        attachments.push_back(fake_att);
    }

    User sender_info;
    sender_info.setId(param.sender_info.id);
    sender_info.setDisplayName(param.sender_info.display_name);
    sender_info.setAvatarPath(
        param.sender_info.avatar_path.has_value()
            ? param.sender_info.avatar_path.value()
            : ""
    );

    SendMessageResponseDto dto(
        std::move(msg), std::move(attachments), param.attachments_download_urls,
        std::move(sender_info)
    );
    Json::Value json_dto = dto.toJson();

    ASSERT_TRUE(json_dto.isMember("message"));
    Json::Value json_msg = json_dto["message"];

    EXPECT_EQ(json_msg["id"], param.message.id);
    EXPECT_EQ(json_msg["chat_id"], param.message.chat_id);
    EXPECT_EQ(json_msg["text"], param.message.text);

    ASSERT_TRUE(json_msg.isMember("attachments"));
    ASSERT_TRUE(json_msg["attachments"].isArray());
    ASSERT_EQ(json_msg["attachments"].size(), param.attachments.size());

    for (Json::ArrayIndex i = 0; i < json_msg["attachments"].size(); i++) {
        Json::Value att_json = json_msg["attachments"][i];

        EXPECT_EQ(att_json["id"], param.attachments[i].id);
        EXPECT_EQ(att_json["file_name"], param.attachments[i].file_name);
        EXPECT_EQ(
            att_json["file_size_bytes"], param.attachments[i].file_size_bytes
        );
        EXPECT_EQ(att_json["message_id"], param.attachments[i].message_id);

        if (param.attachments_download_urls[i].has_value()) {
            EXPECT_EQ(
                att_json["download_url"],
                param.attachments_download_urls[i].value()
            );
        } else {
            EXPECT_EQ(att_json["download_url"], "");
        }
    }

    EXPECT_EQ(json_msg["sender_info"]["id"], param.sender_info.id);
    EXPECT_EQ(
        json_msg["sender_info"]["display_name"], param.sender_info.display_name
    );
    EXPECT_EQ(
        json_msg["sender_info"]["avatar_path"],
        param.sender_info.avatar_path.has_value()
            ? param.sender_info.avatar_path.value()
            : ""
    );
}

INSTANTIATE_TEST_SUITE_P(
    DtoTests,
    SendMessageResponseDtoTest,
    ::testing::Values(
        SendMessageResponseDtoTestCase{
            "Message with mixed attachments",
            MessageInfo{1, 42, "Check out these files!"},
            {AttachmentInfo{100, "avatar.png", 5000, 1},
             AttachmentInfo{101, "document.pdf", 12000, 1}},
            {"https://s3.myproject.com/files/avatar.png", std::nullopt},
            {777, "Alice", "media/alice_ava.png"}
        },
        SendMessageResponseDtoTestCase{
            "Plain text message without attachments",
            MessageInfo{2, 42, "Just a simple text message"},
            {},
            {},
            {777, "Alice", std::nullopt}
        }
    )
);

struct ReadMessagesResponseDtoTestCase {
    std::string test_name;
    bool success;
    std::string expected_message;
};

class ReadMessagesResponseDtoTest
    : public ::testing::TestWithParam<ReadMessagesResponseDtoTestCase> {};

TEST_P(ReadMessagesResponseDtoTest, CorrectlyBuildingJsonFromData) {
    auto param = GetParam();

    ReadMessagesResponseDto dto(param.success);
    Json::Value json_dto = dto.toJson();

    EXPECT_EQ(json_dto["message"], param.expected_message);
}

INSTANTIATE_TEST_SUITE_P(
    DtoTests,
    ReadMessagesResponseDtoTest,
    ::testing::Values(
        ReadMessagesResponseDtoTestCase{
            "Success is true", true, "Succussfully read last messages"
        },
        ReadMessagesResponseDtoTestCase{
            "Success is false", false, "Failed to mark messages as read"
        }
    )
);

struct GetMessageByIdResponseDtoTestCase {
    std::string test_name;
    MessageInfo message;
    std::vector<AttachmentInfo> attachments;
    std::vector<std::optional<std::string>> attachments_download_urls;
};

class GetMessageByIdResponseDtoTest
    : public ::testing::TestWithParam<GetMessageByIdResponseDtoTestCase> {};

TEST_P(GetMessageByIdResponseDtoTest, CorrectlyBuildingJsonFromData) {
    auto param = GetParam();

    Message msg;
    msg.setId(param.message.id);
    msg.setChatId(param.message.chat_id);
    msg.setText(param.message.text);

    std::vector<Attachment> attachments;
    attachments.reserve(param.attachments.size());
    for (const auto &att_info : param.attachments) {
        Attachment fake_att;
        fake_att.setId(att_info.id);
        fake_att.setFileName(att_info.file_name);
        fake_att.setFileSizeBytes(att_info.file_size_bytes);
        fake_att.setMessageId(att_info.message_id);
        attachments.push_back(fake_att);
    }

    std::optional<User> sender_opt = std::nullopt;
    if (param.message.sender_info.has_value()) {
        User fake_user;
        fake_user.setId(param.message.sender_info->id);
        fake_user.setDisplayName(param.message.sender_info->display_name);
        sender_opt = fake_user;
    }

    GetMessageByIdResponseDto dto(
        std::move(msg), std::move(attachments), param.attachments_download_urls,
        std::move(sender_opt)
    );
    Json::Value json_dto = dto.toJson();

    ASSERT_TRUE(json_dto.isMember("message"));
    Json::Value json_msg = json_dto["message"];

    EXPECT_EQ(json_msg["id"], param.message.id);
    EXPECT_EQ(json_msg["chat_id"], param.message.chat_id);
    EXPECT_EQ(json_msg["text"], param.message.text);

    ASSERT_TRUE(json_msg.isMember("attachments"));
    ASSERT_TRUE(json_msg["attachments"].isArray());
    ASSERT_EQ(json_msg["attachments"].size(), param.attachments.size());

    for (Json::ArrayIndex i = 0; i < json_msg["attachments"].size(); i++) {
        Json::Value att_json = json_msg["attachments"][i];

        EXPECT_EQ(att_json["id"], param.attachments[i].id);
        EXPECT_EQ(att_json["file_name"], param.attachments[i].file_name);
        EXPECT_EQ(
            att_json["file_size_bytes"], param.attachments[i].file_size_bytes
        );
        EXPECT_EQ(att_json["message_id"], param.attachments[i].message_id);

        if (param.attachments_download_urls[i].has_value()) {
            EXPECT_EQ(
                att_json["download_url"],
                param.attachments_download_urls[i].value()
            );
        } else {
            EXPECT_EQ(att_json["download_url"], "");
        }
    }
    ASSERT_TRUE(json_msg.isMember("sender_info"));
    if (param.message.sender_info.has_value()) {
        EXPECT_EQ(json_msg["sender_info"]["id"], param.message.sender_info->id);
        EXPECT_EQ(
            json_msg["sender_info"]["display_name"],
            param.message.sender_info->display_name
        );
    } else {
        EXPECT_TRUE(json_msg["sender_info"].isNull());
    }
}

INSTANTIATE_TEST_SUITE_P(
    DtoTests,
    GetMessageByIdResponseDtoTest,
    ::testing::Values(
        GetMessageByIdResponseDtoTestCase{
            "Message with mixed attachments",
            MessageInfo{
                10, 5, "Found the message",
                UserInfo{999, "Charlie", "media/charlie_ava.png"}
            },
            {AttachmentInfo{200, "log.txt", 1024, 10},
             AttachmentInfo{201, "error.png", 4096, 10}},
            {"https://s3.myproject.com/files/log.txt", std::nullopt}
        },
        GetMessageByIdResponseDtoTestCase{
            "Plain text message without attachments",
            MessageInfo{11, 5, "No files here", std::nullopt},
            {},
            {}
        }
    )
);

struct GetAttachmentLinksResponseDtoTestCase {
    std::string test_name;
    std::vector<api::v1::UploadPresignedResult> attachments;
    std::vector<std::string> tokens;
};

class GetAttachmentLinksResponseDtoTest
    : public ::testing::TestWithParam<GetAttachmentLinksResponseDtoTestCase> {};

TEST_P(GetAttachmentLinksResponseDtoTest, CorrectlyBuildingJsonFromData) {
    auto param = GetParam();

    std::vector<api::v1::UploadPresignedResult> attachments_copy =
        param.attachments;
    std::vector<std::string> tokens_copy = param.tokens;

    GetAttachmentLinksResponseDto dto(
        std::move(attachments_copy), std::move(tokens_copy)
    );
    Json::Value json_dto = dto.toJson();

    ASSERT_TRUE(json_dto.isMember("attachments"));
    ASSERT_TRUE(json_dto["attachments"].isArray());
    ASSERT_EQ(json_dto["attachments"].size(), param.attachments.size());

    for (Json::ArrayIndex i = 0; i < json_dto["attachments"].size(); i++) {
        Json::Value file_json = json_dto["attachments"][i];

        EXPECT_EQ(file_json["upload_url"], param.attachments[i].upload_url);
        EXPECT_EQ(
            file_json["attachment_key"], param.attachments[i].attachment_key
        );
        EXPECT_EQ(file_json["content_type"], param.attachments[i].content_type);
        EXPECT_EQ(file_json["token"], param.tokens[i]);
    }
}

INSTANTIATE_TEST_SUITE_P(
    DtoTests,
    GetAttachmentLinksResponseDtoTest,
    ::testing::Values(
        GetAttachmentLinksResponseDtoTestCase{
            "Multiple attachments",
            {{"chat_1/img.png", "https://s3.url/upload1", "image/png",
              "img.png", 1024},
             {"chat_1/doc.pdf", "https://s3.url/upload2", "application/pdf",
              "doc.pdf", 4096}},
            {"abc123token", "xyz987token"}
        },
        GetAttachmentLinksResponseDtoTestCase{"Empty lists", {}, {}}
    )
);

TEST(RemoveMemberResponseDtoTest, CorrectlyBuildsMessageJson) {
    RemoveMemberResponseDto dto;
    Json::Value json_dto = dto.toJson();

    ASSERT_TRUE(json_dto.isMember("message"));
    EXPECT_EQ(json_dto["message"].asString(), "Successfully removed member");
}

TEST(UpdateMemberRoleResponseDtoTest, CorrectlyBuildsMessageJson) {
    UpdateMemberRoleResponseDto dto;
    Json::Value json_dto = dto.toJson();

    ASSERT_TRUE(json_dto.isMember("message"));
    EXPECT_EQ(json_dto["message"].asString(), "Successfully changed role");
}

TEST(UpdateChatInfoResponseDtoTest, CorrectlyBuildsMessageJson) {
    UpdateChatInfoResponseDto dto;
    Json::Value json_dto = dto.toJson();

    ASSERT_TRUE(json_dto.isMember("message"));
    EXPECT_EQ(json_dto["message"].asString(), "Successfully changed chat info");
}

struct ChatMemberDummyInfo {
    int64_t chat_id;
    int64_t user_id;
    std::string role;
};

struct UserDummyInfo {
    int64_t id;
    std::string display_name;
    std::string password_hash;
};

struct GetChatMembersResponseDtoTestCase {
    std::string test_name;
    std::vector<ChatMemberDummyInfo> members_data;
    std::vector<UserDummyInfo> users_data;
};

class GetChatMembersResponseDtoTest
    : public ::testing::TestWithParam<GetChatMembersResponseDtoTestCase> {};

TEST_P(GetChatMembersResponseDtoTest, CorrectlyBuildingJsonFromData) {
    auto param = GetParam();

    std::vector<drogon_model::messenger_db::ChatMembers> members;
    std::vector<User> users;

    // Генерируем фейковые объекты БД
    for (size_t i = 0; i < param.members_data.size(); ++i) {
        drogon_model::messenger_db::ChatMembers fake_member;
        fake_member.setChatId(param.members_data[i].chat_id);
        fake_member.setUserId(param.members_data[i].user_id);
        fake_member.setRole(param.members_data[i].role);
        members.push_back(fake_member);

        User fake_user;
        fake_user.setId(param.users_data[i].id);
        fake_user.setDisplayName(param.users_data[i].display_name);
        fake_user.setPasswordHash(param.users_data[i].password_hash);
        users.push_back(fake_user);
    }

    GetChatMembersResponseDto dto(std::move(members), std::move(users));
    Json::Value json_dto = dto.toJson();

    ASSERT_TRUE(json_dto.isMember("members")) << param.test_name;
    ASSERT_TRUE(json_dto["members"].isArray()) << param.test_name;
    ASSERT_EQ(json_dto["members"].size(), param.members_data.size())
        << param.test_name;

    for (Json::ArrayIndex i = 0; i < json_dto["members"].size(); i++) {
        Json::Value json_member = json_dto["members"][i];

        EXPECT_EQ(
            json_member["chat_id"].asInt64(), param.members_data[i].chat_id
        );
        EXPECT_EQ(
            json_member["user_id"].asInt64(), param.members_data[i].user_id
        );
        EXPECT_EQ(json_member["role"].asString(), param.members_data[i].role);

        ASSERT_TRUE(json_member.isMember("member_info"));
        Json::Value json_user = json_member["member_info"];

        EXPECT_EQ(json_user["id"].asInt64(), param.users_data[i].id);
        EXPECT_EQ(
            json_user["display_name"].asString(),
            param.users_data[i].display_name
        );

        EXPECT_FALSE(json_user.isMember("password_hash"))
            << "CRITICAL SECURITY FAILURE: password_hash leaked in test: "
            << param.test_name;
    }
}

INSTANTIATE_TEST_SUITE_P(
    DtoTests,
    GetChatMembersResponseDtoTest,
    ::testing::Values(
        GetChatMembersResponseDtoTestCase{
            "Multiple members serialization and security check",
            {{1, 10, "Owner"}, {1, 20, "Admin"}},
            {{10, "Alesha", "super_secret_hash_1"},
             {20, "Bob", "super_secret_hash_2"}}
        },
        GetChatMembersResponseDtoTestCase{"Empty members list", {}, {}}
    )
);

struct GetChatMemberResponseDtoTestCase {
    std::string test_name;
    ChatMemberDummyInfo member_data;
    UserDummyInfo user_data;
};

class GetChatMemberResponseDtoTest
    : public ::testing::TestWithParam<GetChatMemberResponseDtoTestCase> {};

TEST_P(GetChatMemberResponseDtoTest, CorrectlyBuildingJsonAndRemovesPassword) {
    auto param = GetParam();

    drogon_model::messenger_db::ChatMembers fake_member;
    fake_member.setChatId(param.member_data.chat_id);
    fake_member.setUserId(param.member_data.user_id);
    fake_member.setRole(param.member_data.role);

    User fake_user;
    fake_user.setId(param.user_data.id);
    fake_user.setDisplayName(param.user_data.display_name);
    fake_user.setPasswordHash(param.user_data.password_hash);

    GetChatMemberResponseDto dto(std::move(fake_member), std::move(fake_user));
    Json::Value json_dto = dto.toJson();

    ASSERT_TRUE(json_dto.isMember("chat_member"));
    Json::Value json_member = json_dto["chat_member"];

    EXPECT_EQ(json_member["chat_id"].asInt64(), param.member_data.chat_id);
    EXPECT_EQ(json_member["role"].asString(), param.member_data.role);

    ASSERT_TRUE(json_member.isMember("member_info"));
    Json::Value json_user = json_member["member_info"];

    EXPECT_EQ(json_user["id"].asInt64(), param.user_data.id);
    EXPECT_EQ(
        json_user["display_name"].asString(), param.user_data.display_name
    );

    EXPECT_FALSE(json_user.isMember("password_hash"))
        << "CRITICAL: Password hash leaked in single member DTO!";
}

INSTANTIATE_TEST_SUITE_P(
    DtoTests,
    GetChatMemberResponseDtoTest,
    ::testing::Values(GetChatMemberResponseDtoTestCase{
        "Correct serialization and password removal",
        {42, 777, "Admin"},
        {777, "Test Admin", "secret_admin_hash_123"}
    })
);

TEST(CreateGroupResponseDtoTest, CorrectlyBuildsJson) {
    drogon_model::messenger_db::Chats fake_chat;
    fake_chat.setId(42);
    fake_chat.setName("New Group Chat");
    fake_chat.setType("group");

    CreateGroupResponseDto dto(std::move(fake_chat));
    Json::Value json_dto = dto.toJson();

    ASSERT_TRUE(json_dto.isMember("chat"));
    EXPECT_EQ(json_dto["chat"]["id"].asInt64(), 42);
    EXPECT_EQ(json_dto["chat"]["name"].asString(), "New Group Chat");
    EXPECT_EQ(json_dto["chat"]["type"].asString(), "group");
}

TEST(GetChatByIdResponseDtoTest, CorrectlyBuildsJson) {
    drogon_model::messenger_db::Chats fake_chat;
    fake_chat.setId(100);
    fake_chat.setName("Existing Chat");
    fake_chat.setType("channel");

    GetChatByIdResponseDto dto(std::move(fake_chat));
    Json::Value json_dto = dto.toJson();

    ASSERT_TRUE(json_dto.isMember("chat"));
    EXPECT_EQ(json_dto["chat"]["id"].asInt64(), 100);
    EXPECT_EQ(json_dto["chat"]["name"].asString(), "Existing Chat");
    EXPECT_EQ(json_dto["chat"]["type"].asString(), "channel");
}

TEST(AddGroupChatMemberResponseDtoTest, CorrectlyBuildsJson) {
    drogon_model::messenger_db::ChatMembers fake_member;
    fake_member.setChatId(42);
    fake_member.setUserId(99);
    fake_member.setRole("Member");

    AddGroupChatMemberResponseDto dto(std::move(fake_member));
    Json::Value json_dto = dto.toJson();

    ASSERT_TRUE(json_dto.isMember("chat_member"));
    EXPECT_EQ(json_dto["chat_member"]["chat_id"].asInt64(), 42);
    EXPECT_EQ(json_dto["chat_member"]["user_id"].asInt64(), 99);
    EXPECT_EQ(json_dto["chat_member"]["role"].asString(), "Member");
}