#pragma once

#include <gmock/gmock.h>
#include <cstdint>
#include <optional>
#include <string>
#include "gmock/gmock.h"
#include "models/Chats.h"
#include "models/Users.h"
#include "repositories/ChatRepository.hpp"

using User = drogon_model::messenger_db::Users;
using Chat = drogon_model::messenger_db::Chats;
using ChatPreview = messenger::dto::ChatPreview;
using ChatMember = drogon_model::messenger_db::ChatMembers;

class MockChatRepository
    : public messenger::repositories::ChatRepositoryInterface {
public:
    using ChatRepositoryInterface::ChatRepositoryInterface;
    MOCK_METHOD(
        drogon::Task<std::optional<Message>>,
        getMessageById,
        (int64_t),
        (override)
    );
    MOCK_METHOD(
        drogon::Task<std::vector<Message>>,
        getAllMessages,
        (),
        (override)
    );
    MOCK_METHOD(
        drogon::Task<Message>,
        sendMessage,
        (int64_t,
         int64_t,
         std::string,
         std::optional<int64_t>,
         std::optional<int64_t>),
        (override)
    );
    MOCK_METHOD(
        drogon::Task<std::vector<Message>>,
        getMessagesByChat,
        (int64_t, std::optional<int64_t>, int64_t),
        (override)
    );
    MOCK_METHOD(
        drogon::Task<bool>,
        editMessage,
        (int64_t, std::string),
        (override)
    );
    MOCK_METHOD(drogon::Task<bool>, removeMessage, (int64_t), (override));
    MOCK_METHOD(
        drogon::Task<Chat>,
        getOrCreateDirect,
        (int64_t, int64_t),
        (override)
    );
    MOCK_METHOD(
        drogon::Task<std::optional<Chat>>,
        getDirect,
        (int64_t, int64_t),
        (override)
    );
    MOCK_METHOD(
        drogon::Task<std::optional<Chat>>,
        getById,
        (int64_t),
        (override)
    );
    MOCK_METHOD(
        drogon::Task<std::vector<ChatPreview>>,
        getByUser,
        (int64_t),
        (override)
    );
    MOCK_METHOD(
        drogon::Task<bool>,
        markAsRead,
        (int64_t, int64_t, int64_t),
        (override)
    );
    MOCK_METHOD(
        drogon::Task<Chat>,
        createGroup,
        (std::string, int64_t, std::vector<int64_t>),
        (override)
    );
    MOCK_METHOD(
        drogon::Task<std::vector<ChatMember>>,
        getMembers,
        (int64_t),
        (override)
    );
    MOCK_METHOD(
        drogon::Task<ChatMember>,
        addMember,
        (int64_t, int64_t, std::string),
        (override)
    );
    MOCK_METHOD(
        drogon::Task<bool>,
        removeMember,
        (int64_t, int64_t),
        (override)
    );
    MOCK_METHOD(
        drogon::Task<bool>,
        updateMemberRole,
        (int64_t, int64_t, std::string),
        (override)
    );
    MOCK_METHOD(
        drogon::Task<bool>,
        updateInfo,
        (int64_t chat_id,
         std::optional<std::string>,
         std::optional<std::string>,
         std::optional<std::string>),
        (override)
    );
    MOCK_METHOD(drogon::Task<Chat>, createSaved, (int64_t), (override));
};