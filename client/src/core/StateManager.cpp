#include "StateManager.hpp"

StateManager::StateManager(QObject *parent) : QObject(parent) {
    m_token = "";
    m_currentUserHandle = "";
    m_userId = -1;
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
        if (isLoggedIn() && m_rememberMe) {
            saveSession();
        }
    }
}

void StateManager::clearState() {
    setToken("");
    setCurrentUserHandle("");
    setUserId(-1);
    saveSession();
}

bool StateManager::isLoggedIn() const {
    return !m_token.isEmpty();
}

void StateManager::saveSession() {
    QSettings settings("MessangerAlyosha", "Session");
    settings.setValue("token", m_token);
    settings.setValue("userId", m_userId);
    settings.setValue("handle", m_currentUserHandle);
}

void StateManager::loadSession() {
    QSettings settings("MessangerAlyosha", "Session");
    m_token = settings.value("token", "").toString();
    m_userId = settings.value("userId", -1).toInt();
    m_currentUserHandle = settings.value("handle", "").toString();

    emit tokenChanged();
    emit userIdChanged();
    emit currentUserHandleChanged();
}

bool StateManager::getRememberMe() const {
    return m_rememberMe;
}

void StateManager::setRememberMe(bool rememberMe) {
    m_rememberMe = rememberMe;
}
