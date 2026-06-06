#include "StateManager.hpp"
#include <qdir.h>
#include <qobject.h>
#include <qstandardpaths.h>
#include <QDir>

StateManager::StateManager(QObject *parent) : QObject(parent) {
    m_token = "";
    m_currentUserHandle = "";
    m_userId = -1;
    bool m_rememberMe = true;
    m_theme = "classic";
    m_accentColor = "#5eb5f7";
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
    m_userDirPath.clear();
    m_tempDirPath.clear();
    m_secretAttachmentsDirPath.clear();
    m_secretDbPath.clear();
    saveSession();

    emit stateCleared();
}

bool StateManager::isLoggedIn() const {
    return !m_token.isEmpty();
}

void StateManager::saveSession() {
    QSettings settings("MessangerAlyosha", "Session");
    settings.setValue("token", m_token);
    settings.setValue("userId", m_userId);
    settings.setValue("handle", m_currentUserHandle);
    settings.setValue("theme", m_theme);
    settings.setValue("accentColor", m_accentColor);
}

void StateManager::loadSession() {
    QSettings settings("MessangerAlyosha", "Session");
    m_token = settings.value("token", "").toString();
    m_userId = settings.value("userId", -1).toInt();
    m_currentUserHandle = settings.value("handle", "").toString();
    m_theme = settings.value("theme", "classic").toString();
    m_accentColor = settings.value("accentColor", "#5eb5f7").toString();

    emit tokenChanged();
    emit userIdChanged();
    emit currentUserHandleChanged();
    emit themeChanged();
    emit accentColorChanged();
}

bool StateManager::getRememberMe() const {
    return m_rememberMe;
}

void StateManager::setRememberMe(bool rememberMe) {
    m_rememberMe = rememberMe;
}

QString StateManager::getTheme() const {
    return m_theme;
}

void StateManager::setTheme(const QString &theme) {
    if (m_theme != theme) {
        m_theme = theme;
        emit themeChanged();
        saveSession();
    }
}

QString StateManager::getAccentColor() const {
    return m_accentColor;
}

void StateManager::setAccentColor(const QString &color) {
    if (m_accentColor != color) {
        m_accentColor = color;
        emit accentColorChanged();
        saveSession();
    }
}

void StateManager::initUserEnvironment() {
    QString app_data_path =
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir baseDir(app_data_path);

    m_userDirPath = baseDir.filePath(QString("user_%1").arg(m_userId));
    QDir userDir(m_userDirPath);

    m_tempDirPath = userDir.filePath("temp");
    m_secretAttachmentsDirPath = userDir.filePath("secret_attachments");
    m_secretDbPath = userDir.filePath("database/secret.db");

    userDir.mkpath("temp");
    userDir.mkpath("secret_attachments");
    userDir.mkpath("database");
    qDebug() << "[StateManager] Successfully initialized user environment.";
}