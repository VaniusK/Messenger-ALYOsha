#pragma once

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>
#include <QString>
#include "ConnectionManager.hpp"
#include "StateManager.hpp"

class AuthManager : public QObject {
    Q_OBJECT

public:
    explicit AuthManager(
        ConnectionManager *connection,
        StateManager *StateManager,
        QObject *parent = nullptr
    );
    Q_INVOKABLE void registerUser(
        const QString &handle,
        const QString &displayName,
        const QString &password
    );
    Q_INVOKABLE void loginUser(const QString &handle, const QString &password);

signals:
    void registerSuccess();
    void registerFailed(const QString &errorMsg);
    void loginSuccess(const QString &token);
    void loginFailed(const QString &errorMsg);
    void userIdFetched(int userId);

private:
    ConnectionManager *m_connection;
    StateManager *m_stateManager;
    void fetchUserId(const QString &handle);
};
