#include <drogon/orm/Result.h>
#include "../fixtures/ChatTestFixture.hpp"
#include "dto/ChatPreview.hpp"
#include "utils/Enum.hpp"

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
        std::nullopt, std::nullopt, messenger::models::MessageType::Text
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
    EXPECT_EQ(chat1_preview.type, chat1.getValueOfType());
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
        std::nullopt, std::nullopt, messenger::models::MessageType::Text
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
        std::nullopt, std::nullopt, messenger::models::MessageType::Text
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
        std::nullopt, std::nullopt, messenger::models::MessageType::Text
    ));
    bool result = sync_wait(repo_.markAsRead(
        chat.getValueOfId(), dummy_user1_.getValueOfId() - 1,
        message.getValueOfId()
    ));
    EXPECT_FALSE(result);
}

TEST_F(ChatTestFixture, TestCreateGroup) {
    /* When valid data is provided,
    CreateGroup should create group chat
    and return it*/
    Chat chat = sync_wait(repo_.createGroup(
        "Чат жабоманов", dummy_user1_.getValueOfId(),
        {dummy_user1_.getValueOfId(), dummy_user2_.getValueOfId(),
         dummy_user3_.getValueOfId()}
    ));
    auto chat_result = sync_wait(repo_.getById(chat.getValueOfId()));
    EXPECT_TRUE(chat_result.has_value());
    EXPECT_EQ(chat_result.value().getValueOfId(), chat.getValueOfId());
    EXPECT_EQ(chat.getValueOfName(), "Чат жабоманов");
}

TEST_F(ChatTestFixture, TestGetMembers) {
    /* When valid data is provided,
    getMembers should return members of the chat*/
    Chat chat = sync_wait(repo_.createGroup(
        "Чат жабоманов", dummy_user1_.getValueOfId(),
        {dummy_user1_.getValueOfId(), dummy_user2_.getValueOfId(),
         dummy_user3_.getValueOfId()}
    ));
    EXPECT_EQ(chat.getValueOfType(), messenger::models::ChatType::Group);
    auto chat_members = sync_wait(repo_.getMembers(chat.getValueOfId()));
    EXPECT_EQ(chat_members.size(), 3);
    auto chat_member1 = *std::find_if(
        chat_members.begin(), chat_members.end(),
        [this](const ChatMember &m) {
            return m.getValueOfUserId() == dummy_user1_.getValueOfId();
        }
    );
    auto chat_member2 = *std::find_if(
        chat_members.begin(), chat_members.end(),
        [this](const ChatMember &m) {
            return m.getValueOfUserId() == dummy_user2_.getValueOfId();
        }
    );
    auto chat_member3 = *std::find_if(
        chat_members.begin(), chat_members.end(),
        [this](const ChatMember &m) {
            return m.getValueOfUserId() == dummy_user3_.getValueOfId();
        }
    );
    EXPECT_EQ(
        chat_member1.getValueOfRole(), messenger::models::ChatRole::Owner
    );
    EXPECT_EQ(
        chat_member2.getValueOfRole(), messenger::models::ChatRole::Member
    );
    EXPECT_EQ(
        chat_member3.getValueOfRole(), messenger::models::ChatRole::Member
    );
}

TEST_F(ChatTestFixture, TestAddMember) {
    /* When valid data is provided,
    addMember should add a member to the group
    and return it*/
    Chat chat = sync_wait(repo_.createGroup(
        "Чат жабоманов", dummy_user1_.getValueOfId(),
        {dummy_user1_.getValueOfId(), dummy_user2_.getValueOfId()}
    ));
    sync_wait(repo_.addMember(
        chat.getValueOfId(), dummy_user3_.getValueOfId(),
        messenger::models::ChatRole::Moderator
    ));
    auto members = sync_wait(repo_.getMembers(chat.getValueOfId()));
    EXPECT_EQ(members.size(), 3);
    auto chat_member3 = *std::find_if(
        members.begin(), members.end(),
        [this](const ChatMember &m) {
            return m.getValueOfUserId() == dummy_user3_.getValueOfId();
        }
    );
    EXPECT_EQ(
        chat_member3.getValueOfRole(), messenger::models::ChatRole::Moderator
    );
}

TEST_F(ChatTestFixture, TestAddMemberFail) {
    /* When trying to add member to a direct chat,
    addMember should throw an exception*/
    Chat chat = sync_wait(repo_.getOrCreateDirect(
        dummy_user1_.getValueOfId(), dummy_user2_.getValueOfId()
    ));
    EXPECT_THROW(
        sync_wait(repo_.addMember(
            chat.getValueOfId(), dummy_user3_.getValueOfId(),
            messenger::models::ChatRole::Moderator
        )),
        std::logic_error
    );
}

TEST_F(ChatTestFixture, TestRemoveMember) {
    /* When valid data is provided,
    removeMember should remove a member from the group
    and return true*/
    Chat chat = sync_wait(repo_.getOrCreateDirect(
        dummy_user1_.getValueOfId(), dummy_user2_.getValueOfId()
    ));
    EXPECT_TRUE(sync_wait(
        repo_.removeMember(chat.getValueOfId(), dummy_user2_.getValueOfId())
    ));
}

TEST_F(ChatTestFixture, TestUpdateMemberRole) {
    /* When valid data is provided,
    updateMemberRole should update member's role
    and return true*/
    Chat chat = sync_wait(repo_.createGroup(
        "Чат жабоманов", dummy_user1_.getValueOfId(),
        {dummy_user1_.getValueOfId(), dummy_user2_.getValueOfId()}
    ));
    auto result = sync_wait(repo_.updateMemberRole(
        chat.getValueOfId(), dummy_user2_.getValueOfId(),
        messenger::models::ChatRole::Moderator
    ));
    EXPECT_TRUE(result);
    auto members = sync_wait(repo_.getMembers(chat.getValueOfId()));
    auto chat_member2 = *std::find_if(
        members.begin(), members.end(),
        [this](const ChatMember &m) {
            return m.getValueOfUserId() == dummy_user2_.getValueOfId();
        }
    );
    EXPECT_EQ(
        chat_member2.getValueOfRole(), messenger::models::ChatRole::Moderator
    );
}

TEST_F(ChatTestFixture, TestUpdateMemberRoleFail) {
    /* When member does not exist,
    updateMemberRole should return false*/
    Chat chat = sync_wait(repo_.createGroup(
        "Чат жабоманов", dummy_user1_.getValueOfId(),
        {dummy_user1_.getValueOfId(), dummy_user2_.getValueOfId()}
    ));
    auto result = sync_wait(repo_.updateMemberRole(
        chat.getValueOfId(), dummy_user3_.getValueOfId(),
        messenger::models::ChatRole::Moderator
    ));
    EXPECT_FALSE(result);
}

TEST_F(ChatTestFixture, TestUpdateInfo) {
    /* When valid data is provided,
    updateInfo should update chat info
    and return true*/
    Chat chat = sync_wait(repo_.createGroup(
        "Чат жабоманов", dummy_user1_.getValueOfId(),
        {dummy_user1_.getValueOfId(), dummy_user2_.getValueOfId()}
    ));
    auto result = sync_wait(repo_.updateInfo(
        chat.getValueOfId(), std::nullopt, "new_avatar", "new_description"
    ));
    EXPECT_TRUE(result);
    Chat new_chat = sync_wait(repo_.getById(chat.getValueOfId())).value();
    EXPECT_EQ(new_chat.getValueOfAvatarPath(), "new_avatar");
    EXPECT_EQ(new_chat.getValueOfDescription(), "new_description");
}

TEST_F(ChatTestFixture, TestUpdateInfoFail) {
    /* When chat does not exist,
    updateInfo should return false*/
    Chat chat = sync_wait(repo_.createGroup(
        "Чат жабоманов", dummy_user1_.getValueOfId(),
        {dummy_user1_.getValueOfId(), dummy_user2_.getValueOfId()}
    ));
    auto result = sync_wait(repo_.updateInfo(
        chat.getValueOfId() - 1, std::nullopt, "new_avatar", "new_description"
    ));
    EXPECT_FALSE(result);
}

TEST_F(ChatTestFixture, TestCreateSaved) {
    /* When valid data is provided,
    createSaved should create Saved Messages chat
    and return it*/
    Chat chat = sync_wait(repo_.createSaved(dummy_user1_.getValueOfId()));
    EXPECT_EQ(chat.getValueOfType(), messenger::models::ChatType::Saved);
    auto members = sync_wait(repo_.getMembers(chat.getValueOfId()));
    EXPECT_EQ(members.size(), 1);
    auto member = members[0];
    EXPECT_EQ(member.getValueOfChatType(), messenger::models::ChatType::Saved);
}