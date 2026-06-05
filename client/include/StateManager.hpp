#pragma once

#include <QObject>
#include <QSettings>
#include <QString>

class StateManager : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString token READ getToken WRITE setToken NOTIFY tokenChanged)
    Q_PROPERTY(QString currentUserHandle READ getCurrentUserHandle WRITE
                   setCurrentUserHandle NOTIFY currentUserHandleChanged)
    Q_PROPERTY(int userId READ getUserId WRITE setUserId NOTIFY userIdChanged)
    Q_PROPERTY(QString theme READ getTheme WRITE setTheme NOTIFY themeChanged)
    Q_PROPERTY(QString accentColor READ getAccentColor WRITE setAccentColor
                   NOTIFY accentColorChanged)

public:
    explicit StateManager(QObject *parent = nullptr);

    QString getToken() const;
    void setToken(const QString &token);

    QString getCurrentUserHandle() const;
    void setCurrentUserHandle(const QString &handle);

    int getUserId() const;
    void setUserId(int id);

    Q_INVOKABLE bool isLoggedIn() const;
    Q_INVOKABLE void clearState();
    Q_INVOKABLE void saveSession();
    Q_INVOKABLE void loadSession();

    Q_PROPERTY(bool rememberMe READ getRememberMe WRITE setRememberMe)

    bool getRememberMe() const;
    void setRememberMe(bool rememberMe);

    QString getTheme() const;
    void setTheme(const QString &theme);

    QString getAccentColor() const;
    void setAccentColor(const QString &color);

    // Methods for local user's files. It would be better if we will use this
    // paths also for common chats cache. One db for all users is quite bugful
    // decision.

    QString getUserDirectory() const {
        return m_userDirPath;
    }

    QString getTempDirectory() const {
        return m_tempDirPath;
    }

    QString getSecretAttachmentsDirectory() const {
        return m_secretAttachmentsDirPath;
    }

    QString getSecretDatabasePath() const {
        return m_secretDbPath;
    }

    void initUserEnvironment();

signals:
    void tokenChanged();
    void currentUserHandleChanged();
    void userIdChanged();
    void themeChanged();
    void accentColorChanged();

    void stateCleared();

private:
    QString m_token;
    QString m_currentUserHandle;
    int m_userId = -1;
    bool m_rememberMe = true;
    QString m_theme;
    QString m_accentColor;

    QString m_userDirPath;
    QString m_tempDirPath;
    QString m_secretAttachmentsDirPath;
    QString m_secretDbPath;
};
