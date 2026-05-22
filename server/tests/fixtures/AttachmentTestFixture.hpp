#pragma once
#include <drogon/utils/coroutine.h>
#include "DbTestFixture.hpp"
#include "repositories/AttachmentRepository.hpp"
#include "repositories/ChatRepository.hpp"
#include "repositories/MessageRepository.hpp"
#include "repositories/UserRepository.hpp"
#include "utils/Enum.hpp"

using MessageRepository = messenger::repositories::MessageRepository;
using ChatRepository = messenger::repositories::ChatRepository;
using UserRepository = messenger::repositories::UserRepository;
using AttachmentRepository = messenger::repositories::AttachmentRepository;
using Chat = messenger::repositories::Chat;
using User = messenger::repositories::User;
using Attachment = messenger::repositories::Attachment;

class AttachmentTestFixture : public DbTestFixture {
private:
    ChatRepository chat_repo_ = ChatRepository(
        std::make_unique<MessageRepository>(
            std::make_unique<AttachmentRepository>()
        ),
        std::make_unique<UserRepository>()
    );
    MessageRepository message_repo_ =
        MessageRepository(std::make_unique<AttachmentRepository>());

protected:
    AttachmentRepository repo_ = AttachmentRepository();
    UserRepository user_repo_ = UserRepository();
    Message dummy_message_1;
    Message dummy_message_2;
    Message dummy_message_3;

    void SetUp() override {
        auto dbClient = app().getDbClient();
        dbClient->execSqlSync("SET client_min_messages TO WARNING;");
        dbClient->execSqlSync("TRUNCATE TABLE users CASCADE;");
        drogon::sync_wait(user_repo_.create("dummy_user", "dummy_user", "hash")
        );
        User dummy_user =
            drogon::sync_wait(user_repo_.getByHandle("dummy_user")).value();
        Chat dummy_chat =
            drogon::sync_wait(chat_repo_.createSaved(dummy_user.getValueOfId())
            );
        dummy_message_1 =
            drogon::sync_wait(message_repo_.send(
                                  dummy_chat.getValueOfId(),
                                  dummy_user.getValueOfId(), "hey",
                                  std::nullopt, std::nullopt,
                                  messenger::models::MessageType::Text
                              ))
                .first;
        dummy_message_2 =
            drogon::sync_wait(message_repo_.send(
                                  dummy_chat.getValueOfId(),
                                  dummy_user.getValueOfId(), "hey",
                                  std::nullopt, std::nullopt,
                                  messenger::models::MessageType::Text
                              ))
                .first;
        dummy_message_3 =
            drogon::sync_wait(message_repo_.send(
                                  dummy_chat.getValueOfId(),
                                  dummy_user.getValueOfId(), "hey",
                                  std::nullopt, std::nullopt,
                                  messenger::models::MessageType::Text
                              ))
                .first;
    }
};