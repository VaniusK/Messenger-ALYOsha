#include <ConnectionManager.hpp>
#include <QNetworkRequest>
#include <QString>
#include <QUrl>

ConnectionManager::ConnectionManager(
    StateManager *stateManager,
    QObject *parent
)
    : QObject(parent), m_stateManager(stateManager) {
    m_networkManager = new QNetworkAccessManager(this);
}

QString ConnectionManager::baseUrl() const {
    return m_baseUrl;
}

QString ConnectionManager::wsUrl() const {
    return m_wsUrl;
}

StateManager *ConnectionManager::stateManager() const {
    return m_stateManager;
}

QNetworkRequest ConnectionManager::createAuthRequest(const QString &endpoint
) const {
    QNetworkRequest request(QUrl(m_baseUrl + endpoint));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    if (m_stateManager && !m_stateManager->getToken().isEmpty()) {
        request.setRawHeader(
            "Authorization", "Bearer " + m_stateManager->getToken().toUtf8()
        );
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