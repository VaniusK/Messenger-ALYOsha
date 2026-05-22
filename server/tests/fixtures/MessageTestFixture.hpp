#pragma once
#include "DbTestFixture.hpp"
#include "repositories/AttachmentRepository.hpp"
#include "repositories/ChatRepository.hpp"
#include "repositories/MessageRepository.hpp"
#include "repositories/UserRepository.hpp"

using MessageRepository = messenger::repositories::MessageRepository;
using ChatRepository = messenger::repositories::ChatRepository;
using UserRepository = messenger::repositories::UserRepository;
using AttachmentRepository = messenger::repositories::AttachmentRepository;
using User = messenger::repositories::User;
using Chat = messenger::repositories::Chat;

class MessageTestFixture : public DbTestFixture {
private:
    ChatRepository chat_repo_ = ChatRepository(
        std::make_unique<MessageRepository>(
            std::make_unique<AttachmentRepository>()
        ),
        std::make_unique<UserRepository>()
    );

protected:
    MessageRepository repo_ =
        MessageRepository(std::make_unique<AttachmentRepository>());
    UserRepository user_repo_ = UserRepository();
    AttachmentRepository attachment_repo_ = AttachmentRepository();
    User dummy_user1_;
    User dummy_user2_;
    User dummy_user3_;
    Chat dummy_chat_1;
    Chat dummy_chat_2;

    void SetUp() override {
        auto dbClient = app().getDbClient();
        dbClient->execSqlSync("SET client_min_messages TO WARNING;");
        dbClient->execSqlSync("TRUNCATE TABLE users CASCADE;");
        drogon::sync_wait(
            user_repo_.create("dummy_user1", "dummy_user1", "hash")
        );
        dummy_user1_ =
            drogon::sync_wait(user_repo_.getByHandle("dummy_user1")).value();
        drogon::sync_wait(
            user_repo_.create("dummy_user2", "dummy_user2", "hash")
        );
        drogon::sync_wait(
            user_repo_.create("dummy_user3", "dummy_user3", "hash")
        );
        dummy_user2_ =
            drogon::sync_wait(user_repo_.getByHandle("dummy_user2")).value();
        dummy_user3_ =
            drogon::sync_wait(user_repo_.getByHandle("dummy_user3")).value();

        dummy_chat_1 = sync_wait(chat_repo_.getOrCreateDirect(
            dummy_user1_.getValueOfId(), dummy_user2_.getValueOfId()
        ));
        dummy_chat_2 = sync_wait(chat_repo_.getOrCreateDirect(
            dummy_user1_.getValueOfId(), dummy_user3_.getValueOfId()
        ));
    }
};