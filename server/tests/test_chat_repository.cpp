#include <drogon/orm/Result.h>
#include "dto/ChatPreview.hpp"
#include "fixtures/ChatTestFixture.hpp"

using ChatRepository = messenger::repositories::ChatRepository;
using Chat = drogon_model::messenger_db::Chats;
using ChatMember = drogon_model::messenger_db::ChatMembers;
using ChatPreview = messenger::dto::ChatPreview;

using namespace drogon;
using namespace drogon::orm;

TEST_F(ChatTestFixture, TestCreateDirect) {
    /* When valid data is provided,
    and direct chat doesn't exist
    getOrCreateDirect() should create and return it*/
    Chat chat = sync_wait(repo_.getOrCreateDirect(
        dummy_user1_.getValueOfId(), dummy_user2_.getValueOfId()
    ));
}

TEST_F(ChatTestFixture, TestGetDirect) {
    /* When valid data is provided,
    and direct chat already exists
    getOrCreateDirect() should return it*/
    Chat chat1 = sync_wait(repo_.getOrCreateDirect(
        dummy_user1_.getValueOfId(), dummy_user2_.getValueOfId()
    ));
    Chat chat2 = sync_wait(repo_.getOrCreateDirect(
        dummy_user1_.getValueOfId(), dummy_user2_.getValueOfId()
    ));
    EXPECT_EQ(chat1.getValueOfId(), chat2.getValueOfId());
}

TEST_F(ChatTestFixture, TestGetDirectMethod) {
    /* When valid data is provided,
    and direct chat exists
    getDirect() should return it*/
    Chat chat1 = sync_wait(repo_.getOrCreateDirect(
        dummy_user1_.getValueOfId(), dummy_user2_.getValueOfId()
    ));
    auto result2 = sync_wait(repo_.getDirect(
        dummy_user1_.getValueOfId(), dummy_user2_.getValueOfId()
    ));
    EXPECT_TRUE(result2.has_value());
    EXPECT_EQ(chat1.getValueOfId(), result2.value().getValueOfId());
}

TEST_F(ChatTestFixture, TestGetDirectFail) {
    /* When valid data is provided,
    and direct chat does not exist
    getDirect() should return nullopt*/
    auto result = sync_wait(repo_.getDirect(
        dummy_user1_.getValueOfId(), dummy_user2_.getValueOfId()
    ));
    EXPECT_FALSE(result.has_value());
}

TEST_F(ChatTestFixture, TestGetById) {
    /* When valid data is provided,
    and chat exists,
    getById() should return it*/
    Chat chat = sync_wait(repo_.getOrCreateDirect(
        dummy_user1_.getValueOfId(), dummy_user2_.getValueOfId()
    ));
    auto chat_result = sync_wait(repo_.getById(chat.getValueOfId()));
    EXPECT_TRUE(chat_result.has_value());
    EXPECT_EQ(chat_result.value().getValueOfId(), chat.getValueOfId());
}

TEST_F(ChatTestFixture, TestGetByIdFail) {
    /* When valid data is provided,
    and chat does not exist,
    getById() should return nullopt*/
    Chat chat = sync_wait(repo_.getOrCreateDirect(
        dummy_user1_.getValueOfId(), dummy_user2_.getValueOfId()
    ));
    auto chat_result = sync_wait(repo_.getById(chat.getValueOfId() - 1));
    EXPECT_FALSE(chat_result.has_value());
}

TEST_F(ChatTestFixture, TestGetByUserDirect) {
    /* When valid data with direct chat is provided,
    getByUser() should return vector of chats
    which the user is a member of*/
    Chat chat1 = sync_wait(repo_.getOrCreateDirect(
        dummy_user1_.getValueOfId(), dummy_user2_.getValueOfId()
    ));
    Chat chat2 = sync_wait(repo_.getOrCreateDirect(
        dummy_user1_.getValueOfId(), dummy_user3_.getValueOfId()
    ));
    Chat chat3 = sync_wait(repo_.getOrCreateDirect(
        dummy_user2_.getValueOfId(), dummy_user3_.getValueOfId()
    ));
    Message message = sync_wait(repo_.sendMessage(
        chat1.getValueOfId(), dummy_user2_.getValueOfId(), "my message",
        std::nullopt, std::nullopt
    ));
    std::vector<ChatPreview> chatPreviews =
        sync_wait(repo_.getByUser(dummy_user1_.getValueOfId()));
    EXPECT_EQ(chatPreviews.size(), 2);
    EXPECT_EQ(
        std::count_if(
            chatPreviews.begin(), chatPreviews.end(),
            [&chat1](const ChatPreview &u) {
                return chat1.getValueOfId() == u.chat_id;
            }
        ),
        1
    );

    EXPECT_EQ(
        std::count_if(
            chatPreviews.begin(), chatPreviews.end(),
            [&chat2](const ChatPreview &u) {
                return chat2.getValueOfId() == u.chat_id;
            }
        ),
        1
    );

    ChatPreview chat1_preview = *std::find_if(
        chatPreviews.begin(), chatPreviews.end(),
        [&chat1](const ChatPreview &u) {
            return chat1.getValueOfId() == u.chat_id;
        }
    );
    EXPECT_EQ(chat1_preview.unread_count, 1);
    EXPECT_EQ(
        chat1_preview.avatar_path.value(), dummy_user2_.getValueOfAvatarPath()
    );
    EXPECT_EQ(chat1_preview.chat_id, chat1.getValueOfId());
    EXPECT_EQ(
        chat1_preview.last_message.value().getValueOfId(),
        message.getValueOfId()
    );
    EXPECT_EQ(chat1_preview.title, dummy_user2_.getValueOfDisplayName());
}

TEST_F(ChatTestFixture, TestGetByUserEmptyDirect) {
    /* When valid data with empty direct chat is provided,
    getByUser() should return vector of chats
    which the user is a member of*/
    Chat chat1 = sync_wait(repo_.getOrCreateDirect(
        dummy_user1_.getValueOfId(), dummy_user3_.getValueOfId()
    ));
    std::vector<ChatPreview> chatPreviews =
        sync_wait(repo_.getByUser(dummy_user1_.getValueOfId()));
    EXPECT_EQ(chatPreviews.size(), 1);
    EXPECT_EQ(
        std::count_if(
            chatPreviews.begin(), chatPreviews.end(),
            [&chat1](const ChatPreview &u) {
                return chat1.getValueOfId() == u.chat_id;
            }
        ),
        1
    );

    ChatPreview chat1_preview = *std::find_if(
        chatPreviews.begin(), chatPreviews.end(),
        [&chat1](const ChatPreview &u) {
            return chat1.getValueOfId() == u.chat_id;
        }
    );
    EXPECT_EQ(chat1_preview.unread_count, 0);
    EXPECT_FALSE(chat1_preview.avatar_path.has_value());
    EXPECT_EQ(chat1_preview.chat_id, chat1.getValueOfId());
    EXPECT_FALSE(chat1_preview.last_message.has_value());
    EXPECT_EQ(chat1_preview.title, dummy_user3_.getValueOfDisplayName());
}

TEST_F(ChatTestFixture, TestMarkAsRead) {
    /* When valid data is provided,
    markAsRead() should return true
    and update chat_member*/
    Chat chat = sync_wait(repo_.getOrCreateDirect(
        dummy_user1_.getValueOfId(), dummy_user2_.getValueOfId()
    ));
    Message message = sync_wait(repo_.sendMessage(
        chat.getValueOfId(), dummy_user2_.getValueOfId(), "my message",
        std::nullopt, std::nullopt
    ));
    bool result = sync_wait(repo_.markAsRead(
        chat.getValueOfId(), dummy_user1_.getValueOfId(), message.getValueOfId()
    ));
    EXPECT_TRUE(result);
    std::vector<ChatPreview> chatPreviews =
        sync_wait(repo_.getByUser(dummy_user1_.getValueOfId()));
    EXPECT_EQ(chatPreviews.size(), 1);
    EXPECT_EQ(chatPreviews[0].unread_count, 0);
}

TEST_F(ChatTestFixture, TestMarkAsReadFailNoChat) {
    /* When chat does not exist,
    markAsRead() should return false*/
    Chat chat = sync_wait(repo_.getOrCreateDirect(
        dummy_user1_.getValueOfId(), dummy_user2_.getValueOfId()
    ));
    Message message = sync_wait(repo_.sendMessage(
        chat.getValueOfId(), dummy_user2_.getValueOfId(), "my message",
        std::nullopt, std::nullopt
    ));
    bool result = sync_wait(repo_.markAsRead(
        chat.getValueOfId() - 1, dummy_user1_.getValueOfId(),
        message.getValueOfId()
    ));
    EXPECT_FALSE(result);
}

TEST_F(ChatTestFixture, TestMarkAsReadFailNoUser) {
    /* When user does not exist,
    markAsRead() should return false*/
    Chat chat = sync_wait(repo_.getOrCreateDirect(
        dummy_user1_.getValueOfId(), dummy_user2_.getValueOfId()
    ));
    Message message = sync_wait(repo_.sendMessage(
        chat.getValueOfId(), dummy_user2_.getValueOfId(), "my message",
        std::nullopt, std::nullopt
    ));
    bool result = sync_wait(repo_.markAsRead(
        chat.getValueOfId(), dummy_user1_.getValueOfId() - 1,
        message.getValueOfId()
    ));
    EXPECT_FALSE(result);
}