#pragma once

#include <QObject>
#include <QString>

class StateManager : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString token READ getToken WRITE setToken NOTIFY tokenChanged)
    Q_PROPERTY(QString currentUserHandle READ getCurrentUserHandle WRITE
                   setCurrentUserHandle NOTIFY currentUserHandleChanged)
    Q_PROPERTY(int userId READ getUserId WRITE setUserId NOTIFY userIdChanged)

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

signals:
    void tokenChanged();
    void currentUserHandleChanged();
    void userIdChanged();

private:
    QString m_token;
    QString m_currentUserHandle;
    int m_userId = -1;
};
