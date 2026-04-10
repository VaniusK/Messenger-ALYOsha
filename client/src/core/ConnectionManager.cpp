#include <ConnectionManager.hpp>
#include <QDebug>
#include <QNetworkRequest>
#include <QString>
#include <QUrl>

ConnectionManager::ConnectionManager(
    std::function<QString()> tokenProvider,
    QObject *parent
)
    : QObject(parent), m_tokenProvider(tokenProvider) {
    m_networkManager = new QNetworkAccessManager(this);

    connect(
        m_networkManager, &QNetworkAccessManager::finished,
        [](QNetworkReply *reply) {
            if (reply->error() != QNetworkReply::NoError) {
                qDebug() << "Network Error (" << reply->error()
                         << "):" << reply->errorString();
                qDebug() << "Target URL:" << reply->url().toString();
            }
        }
    );

    connect(
        m_networkManager, &QNetworkAccessManager::sslErrors, this,
        [](QNetworkReply *reply, const QList<QSslError> &errors) {
            QString host = reply->url().host();
            qDebug() << "[ConnectionManager] SSL Errors for"
                     << reply->url().toString();
            for (const auto &error : errors) {
                qDebug() << "  -" << error.errorString();
            }

            if (host == "api.localhost" || host == "127.0.0.1") {
                qDebug() << "[ConnectionManager] Automatically ignoring SSL "
                            "errors for local host.";
                reply->ignoreSslErrors();
            }
        }
    );
}

QString ConnectionManager::baseUrl() const {
    return m_baseUrl;
}

QString ConnectionManager::wsUrl() const {
    return m_wsUrl;
}

QNetworkRequest ConnectionManager::createAuthRequest(const QString &endpoint
) const {
    QNetworkRequest request(QUrl(m_baseUrl + endpoint));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QString token = m_tokenProvider ? m_tokenProvider() : "";
    if (!token.isEmpty()) {
        request.setRawHeader("Authorization", "Bearer " + token.toUtf8());
    }
    return request;
}

QNetworkReply *ConnectionManager::get(const QString &endpoint) {
    return m_networkManager->get(createAuthRequest(endpoint));
}

QNetworkReply *
ConnectionManager::post(const QString &endpoint, const QByteArray &body) {
    return m_networkManager->post(createAuthRequest(endpoint), body);
}

QNetworkReply *ConnectionManager::getWithBody(
    const QString &endpoint,
    const QByteArray &body
) {
    return m_networkManager->sendCustomRequest(
        createAuthRequest(endpoint), "GET", body
    );
}
