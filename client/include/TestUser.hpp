#pragma once

#include <qsignalspy.h>
#include <AuthManager.hpp>
#include <ChatManager.hpp>
#include <ConnectionManager.hpp>
#include <LocalChatStorage.hpp>
#include <MediaCacheManager.hpp>
#include <StateManager.hpp>
#include <stdexcept>

class TestUser : public QObject {
    Q_OBJECT
public:
    TestUser(
        QString handle,
        QString displayName,
        QString password,
        QObject *parent
    );

    QString getHandle();
    QString getDisplayName();
    int64_t getId();
    int64_t getOpenedChatId();
    ChatManager *getChatManager();

    void fetchChatsSync(int timeout_ms = 1000);
    void fetchChatHistorySync(int64_t chatId, int timeout_ms = 1000);
    void fetchChatHistoryAsync(int64_t chatId);
    void openDirectChatSync(
        int64_t targetUserId,
        const QString &targetUserName,
        int timeout_ms = 1000
    );

    void sendMessageSync(
        const int64_t chatId,
        const QString &text,
        int timeout_ms = 1000
    );

    void sendMessageAsync(const int64_t chatId, const QString &text);

private:
    StateManager *m_stateManager;
    ConnectionManager *m_connectionManager;
    MediaCacheManager *m_mediaCacheManager;
    LocalChatStorage *m_localChatStorage;
    AuthManager *m_authManager;
    ChatManager *m_chatManager;
    QString m_handle;
    QString m_displayName;
    QString m_password;
    int64_t m_openedChatId;

    template <typename T, typename Func>
    std::unique_ptr<QSignalSpy>
    waitForSignal(T *obj, Func signal, int timeout_ms) {
        auto spy = std::make_unique<QSignalSpy>(obj, signal);
        if (!spy->wait(timeout_ms)) {
            throw std::runtime_error("Test request timeouted");
        }
        return spy;
    }

    void registerSelf();
    void login();
};