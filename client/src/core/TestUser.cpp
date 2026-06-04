#include <qsignalspy.h>
#include <TestUser.hpp>
#include "AuthManager.hpp"
#include "ChatManager.hpp"
#include "StateManager.hpp"

TestUser::TestUser(
    QString handle,
    QString displayName,
    QString password,
    QObject *parent
)
    : m_handle(handle),
      m_displayName(displayName),
      m_password(password),
      QObject(parent) {
    m_stateManager = new StateManager(parent);
    m_connectionManager = new ConnectionManager(
        [this]() { return m_stateManager->getToken(); },
        "http://server:8080/v1", "ws://server:8080/ws/chat", parent
    );
    m_authManager =
        new AuthManager(m_connectionManager, m_stateManager, parent);
    m_mediaCacheManager = new MediaCacheManager(m_connectionManager, parent);
    m_localChatStorage = new LocalChatStorage(true, handle, parent);
    m_chatManager = new ChatManager(
        m_connectionManager, m_stateManager, m_mediaCacheManager,
        m_localChatStorage, parent
    );

    registerSelf();
    login();
}

QString TestUser::getHandle() {
    return m_handle;
}

QString TestUser::getDisplayName() {
    return m_displayName;
}

int64_t TestUser::getId() {
    return m_stateManager->getUserId();
}

int64_t TestUser::getOpenedChatId() {
    return m_openedChatId;
}

void TestUser::registerSelf() {
    m_authManager->registerUser(m_handle, m_displayName, m_password);
    waitForSignal(m_authManager, &AuthManager::registerSuccess, 1000);
}

void TestUser::login() {
    m_authManager->loginUser(m_handle, m_password);
    waitForSignal(m_authManager, &AuthManager::loginSuccess, 1000);
    waitForSignal(m_authManager, &AuthManager::userIdFetched, 1000);
}

void TestUser::fetchChatsSync(int timeout_ms) {
    m_chatManager->fetchChats();
    waitForSignal(m_chatManager, &ChatManager::chatsUpdated, timeout_ms / 2);
    waitForSignal(m_chatManager, &ChatManager::chatsUpdated, timeout_ms / 2);
}

void TestUser::openDirectChatSync(
    int64_t targetUserId,
    const QString &targetUserName,
    int timeout_ms
) {
    m_chatManager->openDirectChat(targetUserId, targetUserName);
    auto spy = waitForSignal(
        m_chatManager, &ChatManager::directChatOpened, timeout_ms
    );
    m_openedChatId = spy->takeLast().at(0).toULongLong();
}

void TestUser::sendMessageSync(
    const QString &chatId,
    const QString &text,
    int timeout_ms
) {
    m_chatManager->sendMessage(chatId, text);
    waitForSignal(m_chatManager, &ChatManager::messageSentSuccess, timeout_ms);
}