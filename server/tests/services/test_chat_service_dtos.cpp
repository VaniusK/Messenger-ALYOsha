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

struct MessageInfo {
    int64_t id;
    int64_t chat_id;
    std::string text;
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
    chats_previews.reserve(param.chats_previews.size());
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
        } else {
            fake_preview.last_message = std::nullopt;
        }
        chats_previews.push_back(fake_preview);
    }
    GetUserChatsResponseDto dto(
        std::move(chats_previews), std::move(attachments)
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
                 MessageInfo{100, 1, "Look what bug I found!"}, 3, "group"
             },
             ChatPreviewInfo{
                 2, "Alesha", std::nullopt, MessageInfo{101, 2, "Hello"}, 0,
                 "dialog"
             }},
            {{AttachmentInfo{50, "bug_report.txt", 1024, 100},
              AttachmentInfo{51, "screen.png", 2048, 100}},
             {}}
        },
        GetUserChatsResponseDtoTestCase{
            "New chat without messages",
            {ChatPreviewInfo{
                3, "Secret chat", std::nullopt, std::nullopt, 0, "dialog"
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
    messages.reserve(param.messages.size());
    for (const auto &msg_info : param.messages) {
        Message msg;
        msg.setId(msg_info.id);
        msg.setChatId(msg_info.chat_id);
        msg.setText(msg_info.text);
        messages.push_back(msg);
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
        param.attachments_download_urls
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
    }
}

INSTANTIATE_TEST_SUITE_P(
    DtoTests,
    GetChatMessagesResponseDtoTest,
    ::testing::Values(
        GetChatMessagesResponseDtoTestCase{
            "Rich data: messages with and without attachments",
            {MessageInfo{100, 1, "Here are the files you requested"},
             MessageInfo{101, 1, "Thanks!"}},
            {{AttachmentInfo{50, "report.pdf", 15000, 100},
              AttachmentInfo{51, "meme.jpg", 3000, 100}},
             {}},
            {{"https://s3.myproject.com/files/report.pdf", std::nullopt}, {}}
        },
        GetChatMessagesResponseDtoTestCase{
            "Message without attachments",
            {MessageInfo{102, 2, "Just a plain text message"}},
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

    SendMessageResponseDto dto(
        std::move(msg), std::move(attachments), param.attachments_download_urls
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
            {"https://s3.myproject.com/files/avatar.png", std::nullopt}
        },
        SendMessageResponseDtoTestCase{
            "Plain text message without attachments",
            MessageInfo{2, 42, "Just a simple text message"},
            {},
            {}
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

    GetMessageByIdResponseDto dto(
        std::move(msg), std::move(attachments), param.attachments_download_urls
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
}

INSTANTIATE_TEST_SUITE_P(
    DtoTests,
    GetMessageByIdResponseDtoTest,
    ::testing::Values(
        GetMessageByIdResponseDtoTestCase{
            "Message with mixed attachments",
            MessageInfo{10, 5, "Found the message"},
            {AttachmentInfo{200, "log.txt", 1024, 10},
             AttachmentInfo{201, "error.png", 4096, 10}},
            {"https://s3.myproject.com/files/log.txt", std::nullopt}
        },
        GetMessageByIdResponseDtoTestCase{
            "Plain text message without attachments",
            MessageInfo{11, 5, "No files here"},
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