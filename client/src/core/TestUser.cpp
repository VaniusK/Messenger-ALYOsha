#include <TestUser.hpp>
#include "StateManager.hpp"

TestUser::TestUser(QString handle, QString displayName, QString password)
    : m_handle(handle), m_displayName(displayName), m_password(password) {
    m_stateManager = new StateManager(nullptr);
    m_connectionManager = new ConnectionManager(
        [*this]() { return m_stateManager->getToken(); },
        "https://api.alyosha-test.ru/v1", "wss://api.alyosha-test.ru/ws/chat",
        nullptr
    );
    m_authManager =
        new AuthManager(m_connectionManager, m_stateManager, nullptr);
    m_mediaCacheManager = new MediaCacheManager(m_connectionManager, nullptr);
    m_localChatStorage = new LocalChatStorage(true, "default", nullptr);
    m_chatManager = new ChatManager(
        m_connectionManager, m_stateManager, m_mediaCacheManager,
        m_localChatStorage, nullptr
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