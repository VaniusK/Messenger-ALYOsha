#pragma once
#include "DbTestFixture.hpp"
#include "repositories/ChatRepository.hpp"
#include "repositories/MessageRepository.hpp"
#include "repositories/UserRepository.hpp"

using ChatRepository = messenger::repositories::ChatRepository;
using UserRepository = messenger::repositories::UserRepository;
using MessageRepository = messenger::repositories::MessageRepository;
using AttachmentRepository = messenger::repositories::AttachmentRepository;
using User = messenger::repositories::User;
using Message = messenger::repositories::Message;

class ChatTestFixture : public DbTestFixture {
private:
    UserRepository user_repo_ = UserRepository();

protected:
    ChatRepository repo_ = ChatRepository(
        std::make_unique<MessageRepository>(
            std::make_unique<AttachmentRepository>()
        ),
        std::make_unique<UserRepository>()
    );
    User dummy_user1_;
    User dummy_user2_;
    User dummy_user3_;

public:
    void SetUp() override {
        auto dbClient = app().getDbClient();
        dbClient->execSqlSync("SET client_min_messages TO WARNING;");
        dbClient->execSqlSync("TRUNCATE TABLE users CASCADE;");
        drogon::sync_wait(
            user_repo_.create("dummy_user1", "dummy_user1", "hash")
        );
        dummy_user1_ =
            drogon::sync_wait(user_repo_.getByHandle("dummy_user1")).value();
        sync_wait(user_repo_.updateProfile(
            dummy_user1_.getValueOfId(), std::nullopt, "img1.png", std::nullopt
        ));
        dummy_user1_ =
            drogon::sync_wait(user_repo_.getByHandle("dummy_user1")).value();

        drogon::sync_wait(
            user_repo_.create("dummy_user2", "dummy_user2", "hash")
        );
        dummy_user2_ =
            drogon::sync_wait(user_repo_.getByHandle("dummy_user2")).value();
        sync_wait(user_repo_.updateProfile(
            dummy_user2_.getValueOfId(), std::nullopt, "img2.png", std::nullopt
        ));
        dummy_user2_ =
            drogon::sync_wait(user_repo_.getByHandle("dummy_user2")).value();

        drogon::sync_wait(
            user_repo_.create("dummy_user3", "dummy_user3", "hash")
        );
        dummy_user3_ =
            drogon::sync_wait(user_repo_.getByHandle("dummy_user3")).value();
    }
};