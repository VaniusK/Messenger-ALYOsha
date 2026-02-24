#include "ChatManager.hpp"
#include <qabstractsocket.h>
#include <qdebug.h>
#include <qglobal.h>
#include <qjsonarray.h>
#include <qjsondocument.h>
#include <qjsonobject.h>
#include <qnetworkreply.h>
#include <qnetworkrequest.h>
#include <qobject.h>
#include <qtmetamacros.h>
#include <qurl.h>
#include <QGuiApplication>
#include <QJSEngine>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkRequest>
#include <QUrl>
#include "StateManager.hpp"
extern QJSEngine *qmlEngine(const QObject *obj);

ChatManager::ChatManager(QObject *parent) : QObject(parent) {
    m_networkManager = new QNetworkAccessManager(this);
    m_webSocket = new QWebSocket();

    connect(
        m_webSocket, &QWebSocket::connected, this,
        &ChatManager::onWebSocketConnected
    );
    connect(
        m_webSocket, &QWebSocket::disconnected, this,
        &ChatManager::onWebSocketDisconnected
    );
    connect(
        m_webSocket, &QWebSocket::textMessageReceived, this,
        &ChatManager::onWebSocketTextMessageReceived
    );
    connect(
        m_webSocket,
        QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error), this,
        &ChatManager::onWebSocketError
    );
}

StateManager *ChatManager::getStateManager() {
    for (QObject *obj : qApp->children()) {
        if (StateManager *sm = qobject_cast<StateManager *>(obj)) {
            return sm;
        }
    }
    return nullptr;
}

QNetworkRequest ChatManager::createAuthenticatedRequest(const QString &endpoint
) {
    QNetworkRequest request(QUrl(m_baseUrl + endpoint));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    StateManager *sm = getStateManager();
    if (sm) {
        QString token = sm->getToken();
        request.setRawHeader("Authorization", "Bearer " + token.toUtf8());
    }
    return request;
}

void ChatManager::connectWebSocket() {
    StateManager *sm = getStateManager();
    if (!sm || sm->getToken().isEmpty()) {
        return;
    }

    QUrl url = QUrl(m_wsUrl);
    QNetworkRequest request(url);
    request.setRawHeader("Authorization", "Bearer " + sm->getToken().toUtf8());
}

void ChatManager::searchUsers(const QString &qurry) {
    QNetworkRequest request = createAuthenticatedRequest(
        "/users/search?querry=" + qurry + "&limit=20"
    );
    QNetworkReply *reply = m_networkManager->get(request);

    connect(reply, &QNetworkReply::finished, [this, reply]() {
        reply->deleteLater();

        if (reply->error() == QNetworkReply::NoError) {
            QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
            QJsonArray results = doc.object()["results"].toArray();
            emit usersFound(results);
        } else {
            emit chatError("Search failed: " + reply->errorString());
        }
    });
}

void ChatManager::fetchChats() {
    StateManager *sm = getStateManager();
    if (!sm) {
        return;
    }

    int userId = sm->getUserId();

    QNetworkRequest request =
        createAuthenticatedRequest("chats/user/" + QString::number(userId));
    QNetworkReply *reply = m_networkManager->get(request);

    connect(reply, &QNetworkReply::finished, [this, reply]() {
        reply->deleteLater();

        if (reply->error() == QNetworkReply::NoError) {
            // QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
            // emit chatsUpdated(doc.object()["chats"].toArray());
            emit chatsUpdated(QJsonArray());
        } else {
            emit chatError("Fetch chats failed: " + reply->errorString());
        }
    });
}

void ChatManager::fetchChatHistory(const QString &chatId) {
    QNetworkRequest request =
        createAuthenticatedRequest("chats/" + chatId + "/messages");
    QNetworkReply *reply = m_networkManager->get(request);

    connect(reply, &QNetworkReply::finished, [this, reply]() {
        reply->deleteLater();

        if (reply->error() == QNetworkReply::NoError) {
            emit chatsHistoryLoaded(QJsonArray());
        } else {
            emit chatError("Fetch history failed: " + reply->errorString());
        }
    });
}

void ChatManager::sendMessage(const QString &chatId, const QString &text) {
    QNetworkRequest request =
        createAuthenticatedRequest("chats/" + chatId + "/messages");
    QJsonObject json;
    json["text"] = text;
    QJsonDocument doc(json);

    QNetworkReply *reply = m_networkManager->post(request, doc.toJson());

    connect(reply, &QNetworkReply::finished, [this, reply, chatId]() {
        reply->deleteLater();

        if (reply->error() == QNetworkReply::NoError) {
            emit messageSentSuccess();
            fetchChatHistory(chatId);
        } else {
            emit chatError("Send message failed: " + reply->errorString());
        }
    });
}

void ChatManager::onWebSocketConnected() {
    qDebug() << "[ChatManager] WebSocket connected!";
    emit webSocketConnected();
}

void ChatManager::onWebSocketDisconnected() {
    qDebug() << "[ChatManager] WebSocket disconnected";
    emit webSocketDisconnected();
}

void ChatManager::onWebSocketTextMessageReceived(const QString &message) {
    qDebug() << "[ChatManager] WS message:" << message;
    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
    QJsonObject obj = doc.object();
    emit incomingWebSocketMessage(obj);
}

void ChatManager::onWebSocketError(QAbstractSocket::SocketError error) {
    qDebug() << "[ChatManager] WS error:" << error;
    emit chatError("WebSocket error");
}
