#pragma once

#include <AuthManager.hpp>
#include <ChatManager.hpp>
#include <ConnectionManager.hpp>
#include <LocalChatStorage.hpp>
#include <MediaCacheManager.hpp>
#include <StateManager.hpp>

class TestUser : public QObject {
    Q_OBJECT
public:
    TestUser(
        QString handle,
        QString displayName,
        QString password,
        QObject *parent
    );

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

    void registerSelf();
    void login();
};