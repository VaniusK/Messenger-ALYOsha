#include <drogon/orm/Result.h>
#include "fixtures/ChatTestFixture.hpp"

using ChatRepository = messenger::repositories::ChatRepository;
using Chat = drogon_model::messenger_db::Chats;

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