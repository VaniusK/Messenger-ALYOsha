#include <TestUser.hpp>
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
        "https://api.alyosha-test.ru/v1", "wss://api.alyosha-test.ru/ws/chat",
        parent
    );
    m_authManager =
        new AuthManager(m_connectionManager, m_stateManager, parent);
    m_mediaCacheManager = new MediaCacheManager(m_connectionManager, parent);
    m_localChatStorage = new LocalChatStorage(true, "default", parent);
    m_chatManager = new ChatManager(
        m_connectionManager, m_stateManager, m_mediaCacheManager,
        m_localChatStorage, parent
    );

    registerSelf();
    login();
}

void TestUser::registerSelf() {
    m_authManager->registerUser(m_handle, m_displayName, m_password);
}

void TestUser::login() {
    m_authManager->loginUser(m_handle, m_password);
}