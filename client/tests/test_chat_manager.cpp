#include <gtest/gtest.h>
#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonObject>
#include <QSignalSpy>
#include <memory>
#include "ChatManager.hpp"
#include "StateManager.hpp"
#include "gmock/gmock.h"
#include "mocks/FakeNetworkReply.hpp"
#include "mocks/MockConnectionManager.hpp"

using ::testing::_;
using ::testing::Return;

class ChatManagerTest : public ::testing::Test {
protected:
    std::unique_ptr<QCoreApplication> app;
    std::shared_ptr<MockConnectionManager> mockConn;
    std::unique_ptr<StateManager> stateManager;
    std::unique_ptr<MediaCacheManager> mediaCacheManager;
    std::unique_ptr<LocalChatStorage> localChatStorage;
    std::unique_ptr<ChatManager> chatManager;

    void SetUp() override {
        if (!QCoreApplication::instance()) {
            static int argc = 1;
            static char name[] = "test";
            static char *argv[] = {name, nullptr};
            app = std::make_unique<QCoreApplication>(argc, argv);
        }

        mockConn = std::make_shared<MockConnectionManager>();
        stateManager = std::make_unique<StateManager>();
        mediaCacheManager = std::make_unique<MediaCacheManager>(mockConn.get());
        localChatStorage = std::make_unique<LocalChatStorage>(true, "default");
        chatManager = std::make_unique<ChatManager>(
            mockConn.get(), stateManager.get(), mediaCacheManager.get(),
            localChatStorage.get()
        );
    }
};

TEST_F(ChatManagerTest, FetchChatsSuccess) {
    stateManager->setUserId(42);
    auto *fakeReply = new FakeNetworkReply(
        200, "{\"chats\":[{\"id\":1, \"title\":\"Test chat\"}]}"
    );
    EXPECT_CALL(*mockConn, get(QString("/chats/user/42")))
        .WillOnce(Return(fakeReply));

    QSignalSpy spy(chatManager.get(), &ChatManager::chatsUpdated);

    chatManager->fetchChats();
    fakeReply->emitFinished();

    EXPECT_EQ(spy.count(), 1);
    QJsonArray chatsArray = spy.takeFirst().at(0).toJsonArray();
    EXPECT_EQ(chatsArray.size(), 1);
    EXPECT_EQ(
        chatsArray[0].toObject()["title"].toString().toStdString(), "Test chat"
    );
}

TEST_F(ChatManagerTest, SendMessageSuccess) {
    auto *fakePostReply = new FakeNetworkReply(200, "{}");
    EXPECT_CALL(*mockConn, post(QString("/chats/1/messages"), _))
        .WillOnce(Return(fakePostReply));

    auto *fakeHistoryReply = new FakeNetworkReply(200, "{\"messages\":[]}");
    EXPECT_CALL(*mockConn, get(QString("/chats/1/messages?limit=20")))
        .WillOnce(Return(fakeHistoryReply));

    QSignalSpy spySent(chatManager.get(), &ChatManager::messageSentSuccess);
    QSignalSpy spyHistory(chatManager.get(), &ChatManager::chatsHistoryLoaded);

    chatManager->sendMessage("1", "Йоу!");
    fakePostReply->emitFinished();
    fakeHistoryReply->emitFinished();

    EXPECT_EQ(spySent.count(), 1);
    EXPECT_EQ(spyHistory.count(), 1);
}
