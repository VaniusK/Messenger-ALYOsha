#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonObject>
#include <QSignalSpy>
#include <TestUser.hpp>
#include <chrono>

using ::testing::_;
using ::testing::Return;

class TestUserFixture : public ::testing::Test {
protected:
    std::unique_ptr<QCoreApplication> m_app;

    void SetUp() override {
        if (!QCoreApplication::instance()) {
            static int argc = 1;
            static char name[] = "test";
            static char *argv[] = {name, nullptr};
            m_app = std::make_unique<QCoreApplication>(argc, argv);
        }
    }
};

TEST_F(TestUserFixture, BasicMessaging) {
    auto user_1 = TestUser("user1", "User", "12345678", m_app.get());
    auto user_2 = TestUser("user2", "User", "12345678", m_app.get());

    qDebug() << "user_1 sending messages";

    user_1.openDirectChatSync(user_2.getId(), user_2.getDisplayName());
    user_1.sendMessageSync(QString::number(user_1.getOpenedChatId()), "Hi");
    user_1.sendMessageSync(QString::number(user_1.getOpenedChatId()), "Hello");
    user_1.sendMessageSync(
        QString::number(user_1.getOpenedChatId()), "Talk to me"
    );

    qDebug() << "user_2 fetching chat history - subsequent calls are fast "
                "because of cache";
    auto fetch_chat_start_time = std::chrono::high_resolution_clock::now();
    user_2.openDirectChatSync(user_1.getId(), user_1.getDisplayName());
    auto fetch_chat_end_time = std::chrono::high_resolution_clock::now();
    int fetch_chat_duration =
        std::chrono::duration_cast<std::chrono::microseconds>(
            fetch_chat_end_time - fetch_chat_start_time
        )
            .count();

    qDebug() << "Uncached: " << fetch_chat_duration << "ms";

    auto cached_fetch_chat_start_time =
        std::chrono::high_resolution_clock::now();
    user_2.openDirectChatSync(
        user_1.getId(), user_1.getDisplayName(), fetch_chat_duration / 2
    );
    auto cached_fetch_chat_end_time = std::chrono::high_resolution_clock::now();
    int cached_fetch_chat_duration =
        std::chrono::duration_cast<std::chrono::microseconds>(
            cached_fetch_chat_end_time - cached_fetch_chat_start_time
        )
            .count();

    qDebug() << "Cached: " << cached_fetch_chat_duration << "ms";

    user_2.openDirectChatSync(
        user_1.getId(), user_1.getDisplayName(), fetch_chat_duration / 2
    );
    user_2.openDirectChatSync(
        user_1.getId(), user_1.getDisplayName(), fetch_chat_duration / 2
    );
}
