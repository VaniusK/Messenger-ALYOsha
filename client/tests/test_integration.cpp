#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonObject>
#include <QSignalSpy>
#include <TestUser.hpp>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <memory>

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
    srand(static_cast<unsigned int>(time(nullptr)));
    auto user_1 = TestUser(
        "user" + QString::number(rand()), "User", "12345678", m_app.get()
    );
    auto user_2 = TestUser(
        "user" + QString::number(rand()), "User", "12345678", m_app.get()
    );

    qDebug() << "user handles:" << user_1.getHandle() << user_2.getHandle();

    qDebug() << "user_1 sending messages";

    user_1.openDirectChatSync(user_2.getId(), user_2.getDisplayName());
    user_1.sendMessageSync(user_1.getOpenedChatId(), "Hi");
    user_1.sendMessageSync(user_1.getOpenedChatId(), "Hello");
    user_1.sendMessageSync(user_1.getOpenedChatId(), "Talk to me");

    qDebug() << "user_2 fetching chat history - subsequent calls should be "
                "fast because of cache";
    auto fetch_chat_start_time = std::chrono::high_resolution_clock::now();
    user_2.fetchChatHistorySync(user_1.getOpenedChatId());
    auto fetch_chat_end_time = std::chrono::high_resolution_clock::now();
    int fetch_chat_duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            fetch_chat_end_time - fetch_chat_start_time
        )
            .count();

    qDebug() << "Uncached: " << fetch_chat_duration << "ms";

    auto cached_fetch_chat_start_time =
        std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 1000; i++) {
        user_2.fetchChatHistorySync(
            user_1.getOpenedChatId(), fetch_chat_duration / 2
        );
    }
    auto cached_fetch_chat_end_time = std::chrono::high_resolution_clock::now();
    int cached_fetch_chat_duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            cached_fetch_chat_end_time - cached_fetch_chat_start_time
        )
            .count();

    qDebug() << "Cached: " << cached_fetch_chat_duration / (double)1000 << "ms";
}

TEST_F(TestUserFixture, LoadTesting) {
    bool should_run_load_test =
        qEnvironmentVariable(
            "SHOULD_RUN_LOAD_TEST", "ws://server:8080/ws/chat"
        ) == "true";
    if (!should_run_load_test) {
        return;
    }
    const int totalUsers = 10;
    std::vector<std::unique_ptr<TestUser>> users(totalUsers);
    for (int i = 0; i < totalUsers; i++) {
        std::unique_ptr<TestUser> user(new TestUser(
            "user" + QString::number(rand()) + QString::number(i), "user",
            "12345678", m_app.get()
        ));
        std::swap(users[i], user);
    }
    int totalMessages = users.size();
    std::atomic<int> sentMessages{0};
    std::atomic<int> sentMessagesSuccesses{0};
    std::atomic<int> readChat{0};
    std::atomic<int> readChatSuccesses{0};
    const int totalActions = 1000;
    std::vector<std::vector<int64_t>> directChatIds(
        totalUsers, std::vector<int64_t>(totalUsers)
    );
    for (int i = 0; i < totalUsers; i++) {
        for (int j = i + 1; j < totalUsers; j++) {
            auto &user1 = users[i];
            auto &user2 = users[j];
            user1->openDirectChatSync(user2->getId(), user2->getDisplayName());
            directChatIds[i][j] = user1->getOpenedChatId();
            directChatIds[j][i] = user1->getOpenedChatId();
        }
    }
    for (int i = 0; i < totalUsers; i++) {
        auto &user = users[i];
        QObject::connect(
            user->getChatManager(), &ChatManager::messageSentSuccess,
            [&, this]() { sentMessagesSuccesses++; }
        );

        QObject::connect(
            user->getChatManager(), &ChatManager::chatsHistoryLoaded,
            [&, this]() { readChatSuccesses++; }
        );
    }

    for (int t = 0; t < totalActions; t++) {
        int64_t i = rand() % totalUsers;
        int64_t j = rand() % totalUsers;
        if (i == j) {
            continue;
        }
        auto &user1 = users[i];
        auto &user2 = users[j];
        if (rand() % 10 <= 0) {
            sentMessages++;
            user1->sendMessageAsync(
                directChatIds[i][j], "msg " + QString::number(i)
            );
        } else {
            readChat++;
            user1->fetchChatHistoryAsync(directChatIds[i][j]);
        }
    }
    QElapsedTimer elapsed;
    elapsed.start();
    while (sentMessagesSuccesses < sentMessages &&
           readChatSuccesses < readChat && elapsed.elapsed() < 30000) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
    }
    qDebug() << "Processed" << sentMessages << "sent messages and" << readChat
             << "read chats in" << elapsed.elapsed() << "ms";
}