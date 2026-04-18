#include <drogon/orm/Result.h>
#include <gtest/gtest.h>
#include <stdexcept>
#include "../fixtures/MessageTestFixture.hpp"
#include "utils/Enum.hpp"

using MessageRepository = messenger::repositories::MessageRepository;
using Message = drogon_model::messenger_db::Messages;

using namespace drogon;
using namespace drogon::orm;

TEST_F(MessageTestFixture, TestSend) {
    /* When valid data is provided
    send() should create a new message,
    and it should be retrievable via getAll()*/
    Message message = sync_wait(repo_.send(
        dummy_chat_1.getValueOfId(), dummy_user1_.getValueOfId(), "my message",
        std::nullopt, std::nullopt, messenger::models::MessageType::Text
    ));
    std::vector<Message> messages = sync_wait(repo_.getAll());
    EXPECT_EQ(messages.size(), 1);
    EXPECT_EQ(messages[0].getValueOfId(), message.getValueOfId());
    EXPECT_EQ(
        messages[0].getValueOfType(), messenger::models::MessageType::Text
    );
}

TEST_F(MessageTestFixture, TestSendInvalidChatId) {
    /* When given chat id does not exist
    send() should throw runtime error*/
    EXPECT_THROW(
        sync_wait(repo_.send(
            dummy_chat_1.getValueOfId() - 1, dummy_user1_.getValueOfId(),
            "my message", std::nullopt, std::nullopt,
            messenger::models::MessageType::Text
        )),
        std::runtime_error
    );
}

TEST_F(MessageTestFixture, TestSendInvalidSenderId) {
    /* When given sender id does not exist
    send() should throw runtime error*/
    EXPECT_THROW(
        sync_wait(repo_.send(
            dummy_chat_1.getValueOfId(), dummy_user1_.getValueOfId() - 1,
            "my message", std::nullopt, std::nullopt,
            messenger::models::MessageType::Text
        )),
        std::runtime_error
    );
}

TEST_F(MessageTestFixture, TestSendOptionals) {
    /* When valid data with optional arguments is provided
    send() should create a new message,
    and it should be retrievable via getAll()*/
    Message original_message = sync_wait(repo_.send(
        dummy_chat_1.getValueOfId(), dummy_user1_.getValueOfId(), "my message",
        std::nullopt, std::nullopt, messenger::models::MessageType::Text
    ));
    Message message = sync_wait(repo_.send(
        dummy_chat_1.getValueOfId(), dummy_user1_.getValueOfId(), "my message",
        original_message.getValueOfId(), dummy_user2_.getValueOfId(),
        messenger::models::MessageType::Text
    ));
    std::vector<Message> messages = sync_wait(repo_.getAll());
    EXPECT_EQ(messages.size(), 2);
    EXPECT_EQ(
        message.getValueOfReplyToMessageId(), original_message.getValueOfId()
    );
    EXPECT_EQ(
        message.getValueOfForwardedFromUserId(), dummy_user2_.getValueOfId()
    );
    EXPECT_EQ(
        message.getValueOfForwardedFromUserName(),
        dummy_user2_.getValueOfDisplayName()
    );
}

TEST_F(MessageTestFixture, TestSendMultiple) {
    /* When multiple messages are sent,
    send() should create a new message for each,
    and they should be retrievable via getAll()*/
    Message message1 = sync_wait(repo_.send(
        dummy_chat_1.getValueOfId(), dummy_user1_.getValueOfId(), "my message",
        std::nullopt, std::nullopt, messenger::models::MessageType::Text
    ));
    Message message2 = sync_wait(repo_.send(
        dummy_chat_1.getValueOfId(), dummy_user1_.getValueOfId(), "my message",
        std::nullopt, std::nullopt, messenger::models::MessageType::Text
    ));
    Message message3 = sync_wait(repo_.send(
        dummy_chat_1.getValueOfId(), dummy_user1_.getValueOfId(), "my message",
        std::nullopt, std::nullopt, messenger::models::MessageType::Text
    ));
    std::vector<Message> messages = sync_wait(repo_.getAll());
    EXPECT_EQ(messages.size(), 3);
}

TEST_F(MessageTestFixture, TestGetById) {
    /* When message with given id exists
    getById() should return in*/
    Message message = sync_wait(repo_.send(
        dummy_chat_1.getValueOfId(), dummy_user1_.getValueOfId(), "my message",
        std::nullopt, std::nullopt, messenger::models::MessageType::Text
    ));
    auto result = sync_wait(repo_.getById(message.getValueOfId()));
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result.value().getValueOfId(), message.getValueOfId());
}

TEST_F(MessageTestFixture, TestGetByIdFail) {
    /* When message with given id does not exist
    getById() should return nullopt*/
    Message message = sync_wait(repo_.send(
        dummy_chat_1.getValueOfId(), dummy_user1_.getValueOfId(), "my message",
        std::nullopt, std::nullopt, messenger::models::MessageType::Text
    ));
    auto result = sync_wait(repo_.getById(message.getValueOfId() + 1));
    EXPECT_FALSE(result.has_value());
}

TEST_F(MessageTestFixture, TestGetByChat) {
    /* When messages in given chat exist
    getByChat() should return them
    and nothing else*/
    Message message1 = sync_wait(repo_.send(
        dummy_chat_1.getValueOfId(), dummy_user1_.getValueOfId(), "my message",
        std::nullopt, std::nullopt, messenger::models::MessageType::Text
    ));
    Message message2 = sync_wait(repo_.send(
        dummy_chat_1.getValueOfId(), dummy_user1_.getValueOfId(), "my message",
        std::nullopt, std::nullopt, messenger::models::MessageType::Text
    ));
    Message message3 = sync_wait(repo_.send(
        dummy_chat_2.getValueOfId(), dummy_user1_.getValueOfId(), "my message",
        std::nullopt, std::nullopt, messenger::models::MessageType::Text
    ));
    std::vector<Message> messages = sync_wait(
        repo_.getByChat(dummy_chat_1.getValueOfId(), std::nullopt, 100)
    );
    EXPECT_EQ(
        std::count_if(
            messages.begin(), messages.end(),
            [&message1](const Message &u) {
                return message1.getValueOfId() == u.getValueOfId();
            }
        ),
        1
    );
    EXPECT_EQ(
        std::count_if(
            messages.begin(), messages.end(),
            [&message2](const Message &u) {
                return message2.getValueOfId() == u.getValueOfId();
            }
        ),
        1
    );
    EXPECT_EQ(
        std::count_if(
            messages.begin(), messages.end(),
            [&message3](const Message &u) {
                return message3.getValueOfId() == u.getValueOfId();
            }
        ),
        0
    );
}

TEST_F(MessageTestFixture, TestGetByChatOptionals) {
    /* When messages in given chat exist
    getByChat() should return them
    respecting before_id and limit params*/
    Message message1 = sync_wait(repo_.send(
        dummy_chat_1.getValueOfId(), dummy_user1_.getValueOfId(), "my message",
        std::nullopt, std::nullopt, messenger::models::MessageType::Text
    ));
    Message message2 = sync_wait(repo_.send(
        dummy_chat_1.getValueOfId(), dummy_user1_.getValueOfId(), "my message",
        std::nullopt, std::nullopt, messenger::models::MessageType::Text
    ));
    Message message3 = sync_wait(repo_.send(
        dummy_chat_2.getValueOfId(), dummy_user1_.getValueOfId(), "my message",
        std::nullopt, std::nullopt, messenger::models::MessageType::Text
    ));
    std::vector<Message> messages = sync_wait(
        repo_.getByChat(dummy_chat_1.getValueOfId(), message2.getValueOfId(), 1)
    );
    EXPECT_EQ(messages.size(), 1);
    EXPECT_EQ(
        std::count_if(
            messages.begin(), messages.end(),
            [&message1](const Message &u) {
                return message1.getValueOfId() == u.getValueOfId();
            }
        ),
        1
    );
}

TEST_F(MessageTestFixture, TestEdit) {
    /* When message exists,
    edit() should return true
    and the text should be updated*/
    Message message1 = sync_wait(repo_.send(
        dummy_chat_1.getValueOfId(), dummy_user1_.getValueOfId(), "text",
        std::nullopt, std::nullopt, messenger::models::MessageType::Text
    ));
    bool result = sync_wait(repo_.edit(message1.getValueOfId(), "new text"));
    EXPECT_TRUE(result);
    Message message_updated =
        sync_wait(repo_.getById(message1.getValueOfId())).value();
    EXPECT_EQ(message_updated.getValueOfText(), "new text");
}

TEST_F(MessageTestFixture, TestEditFail) {
    /* When message does not exist,
    edit() should return false
    and the text should be updated*/
    Message message1 = sync_wait(repo_.send(
        dummy_chat_1.getValueOfId(), dummy_user1_.getValueOfId(), "text",
        std::nullopt, std::nullopt, messenger::models::MessageType::Text
    ));
    bool result =
        sync_wait(repo_.edit(message1.getValueOfId() - 1, "new text"));
    EXPECT_FALSE(result);
}

TEST_F(MessageTestFixture, TestRemove) {
    /* When message exists,
    remove() should return true
    and the message should be deleted*/
    Message message1 = sync_wait(repo_.send(
        dummy_chat_1.getValueOfId(), dummy_user1_.getValueOfId(), "text",
        std::nullopt, std::nullopt, messenger::models::MessageType::Text
    ));
    bool result = sync_wait(repo_.remove(message1.getValueOfId()));
    EXPECT_TRUE(result);
    auto message_result = sync_wait(repo_.getById(message1.getValueOfId()));
    EXPECT_FALSE(message_result.has_value());
}

TEST_F(MessageTestFixture, TestRemoveFail) {
    /* When message does not exist,
    remove() should return false*/
    Message message1 = sync_wait(repo_.send(
        dummy_chat_1.getValueOfId(), dummy_user1_.getValueOfId(), "text",
        std::nullopt, std::nullopt, messenger::models::MessageType::Text
    ));
    bool result = sync_wait(repo_.remove(message1.getValueOfId() - 1));
    EXPECT_FALSE(result);
}