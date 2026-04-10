#pragma once

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QObject>
#include <QString>
#include <functional>

class ConnectionManager : public QObject {
    Q_OBJECT

public:
    explicit ConnectionManager(
        std::function<QString()> tokenProvider,
        QObject *parent = nullptr
    );

    QString baseUrl() const;
    void setBaseUrl(const QString &url);
    QString wsUrl() const;

    QNetworkRequest createAuthRequest(const QString &endpoint) const;
    virtual QNetworkReply *get(const QString &endpoint);
    virtual QNetworkReply *
    post(const QString &endpoint, const QByteArray &body);
    virtual QNetworkReply *
    getWithBody(const QString &endpoint, const QByteArray &body);

private:
    QNetworkAccessManager *m_networkManager;
    std::function<QString()> m_tokenProvider;
    QString m_baseUrl = "https://api.alyosha.su/v1";
    QString m_wsUrl = "wss://api.alyosha.su/ws/chat";
};
