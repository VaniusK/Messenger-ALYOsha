#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonObject>
#include <QSignalSpy>
#include <TestUser.hpp>

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

    qDebug() << user_2.getId() << " user 2 id";
    qDebug() << user_2.getDisplayName() << " user 2 display name";

    user_1.openDirectChatSync(user_2.getId(), user_2.getDisplayName());
    user_1.sendMessageSync(QString::number(user_1.getOpenedChatId()), "Hi");
    user_1.sendMessageSync(QString::number(user_1.getOpenedChatId()), "Hello");
    user_1.sendMessageSync(
        QString::number(user_1.getOpenedChatId()), "Talk to me"
    );

    user_2.openDirectChatSync(user_1.getId(), user_1.getDisplayName());
    user_2.openDirectChatSync(user_1.getId(), user_1.getDisplayName(), 10);
    user_2.openDirectChatSync(user_1.getId(), user_1.getDisplayName(), 10);
    user_2.openDirectChatSync(user_1.getId(), user_1.getDisplayName(), 10);
}
