#include "StateManager.hpp"

StateManager::StateManager(QObject *parent) : QObject(parent) {
    m_token = "";
    m_currentUserHandle = "";
    m_userId = 0;
}

QString StateManager::getToken() const {
    return m_token;
}

void StateManager::setToken(const QString &token) {
    if (m_token != token) {
        m_token = token;
        emit tokenChanged();
    }
}

QString StateManager::getCurrentUserHandle() const {
    return m_currentUserHandle;
}

void StateManager::setCurrentUserHandle(const QString &handle) {
    if (m_currentUserHandle != handle) {
        m_currentUserHandle = handle;
        emit currentUserHandleChanged();
    }
}

int StateManager::getUserId() const {
    return m_userId;
}

void StateManager::setUserId(int id) {
    if (m_userId != id) {
        m_userId = id;
        emit userIdChanged();
    }
}

void StateManager::clearState() {
    setToken("");
    setCurrentUserHandle("");
    setUserId(-1);
}

bool StateManager::isLoggedIn() const {
    return !m_token.isEmpty();
}
