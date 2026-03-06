#pragma once

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QObject>
#include <QString>
#include "StateManager.hpp"

class ConnectionManager : public QObject {
    Q_OBJECT

public:
    explicit ConnectionManager(
        StateManager *stateManager,
        QObject *parent = nullptr
    );

    QString baseUrl() const;
    void setBaseUrl(const QString &url);
    QString wsUrl() const;
    StateManager *stateManager() const;

    QNetworkRequest createAuthRequest(const QString &endpoint) const;
    QNetworkReply *get(const QString &endpoint);
    QNetworkReply *post(const QString &endpoint, const QByteArray &body);
    QNetworkReply *getWithBody(const QString &endpoint, const QByteArray &body);

private:
    QNetworkAccessManager *m_networkManager;
    StateManager *m_stateManager;
    QString m_baseUrl = "http://127.0.0.1:5555/api/v1";
    QString m_wsUrl = "ws://127.0.0.1:5555/ws/chat";
};
