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
}
