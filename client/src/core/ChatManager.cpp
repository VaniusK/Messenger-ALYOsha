#include "ChatManager.hpp"
#include <QDebug>
#include <QJsonDocument>
#include <QNetworkReply>
#include <QNetworkRequest>

ChatManager::ChatManager(
    ConnectionManager *connection,
    StateManager *stateManager,
    QObject *parent
)
    : QObject(parent), m_connection(connection), m_stateManager(stateManager) {
    m_webSocket =
        new QWebSocket(QString(), QWebSocketProtocol::VersionLatest, this);

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

    connect(
        m_webSocket, &QWebSocket::sslErrors, this,
        [this](const QList<QSslError> &errors) {
            QString host = m_webSocket->requestUrl().host();
            qDebug() << "[ChatManager] WebSocket SSL Errors for"
                     << m_webSocket->requestUrl().toString();
            for (const auto &error : errors) {
                qDebug() << "  -" << error.errorString();
            }

            if (host == "api.localhost" || host == "127.0.0.1") {
                qDebug() << "[ChatManager] Automatically ignoring SSL errors "
                            "for local host.";
                m_webSocket->ignoreSslErrors();
            }
        }
    );
}

void ChatManager::connectWebSocket() {
    StateManager *sm = m_stateManager;
    if (!sm || sm->getToken().isEmpty()) {
        return;
    }

    QNetworkRequest request(QUrl(m_connection->wsUrl()));
    request.setRawHeader("Authorization", "Bearer " + sm->getToken().toUtf8());
    m_webSocket->open(request);
}

void ChatManager::searchUsers(const QString &query) {
    QJsonObject json;
    json["query"] = query;
    json["limit"] = 50;

    QNetworkReply *reply = m_connection->getWithBody(
        "/users/search", QJsonDocument(json).toJson()
    );

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
    StateManager *sm = m_stateManager;
    if (!sm || sm->getUserId() <= 0) {
#ifdef QT_DEBAG
        qDebug() << "[ChatManager] fetchChats skipped. Invalid state manager "
                    "or userId:"
                 << (sm ? sm->getUserId() : -1);
#endif
        return;
    }

#ifdef QT_DEBAG
    qDebug() << "[ChatManager] fetchChats called for user ID:"
             << sm->getUserId();
#endif

    QNetworkReply *reply =
        m_connection->get("/chats/user/" + QString::number(sm->getUserId()));

    connect(reply, &QNetworkReply::finished, [this, reply]() {
        reply->deleteLater();
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray responseData = reply->readAll();
#ifdef QT_DEBAG
            qDebug() << "[ChatManager] fetchChats RAW JSON: " << responseData;
#endif
            QJsonDocument doc = QJsonDocument::fromJson(responseData);
            emit chatsUpdated(doc.object()["chats"].toArray());
        } else {
            emit chatError("Fetch chats failed: " + reply->errorString());
        }
    });
}

void ChatManager::fetchChatHistory(const QString &chatId, int beforeId) {
    QJsonObject reqJson;
    reqJson["limit"] = 50;

    if (beforeId > 0) {
        reqJson["before_id"] = beforeId;
    }

    QNetworkReply *reply = m_connection->getWithBody(
        "/chats/" + chatId + "/messages", QJsonDocument(reqJson).toJson()
    );

    connect(reply, &QNetworkReply::finished, [this, reply, beforeId]() {
        reply->deleteLater();
        if (reply->error() == QNetworkReply::NoError) {
            QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
#ifdef QT_DEBAG
            qDebug() << "[ChatManager] fetchChatHistory RAW JSON: "
                     << doc.toJson(QJsonDocument::Compact);
#endif
            QJsonArray raw = doc.object()["messages"].toArray();

            int currentUserId = m_stateManager->getUserId();
            QJsonArray messages;
            for (int i = raw.size() - 1; i >= 0; i--) {
                QJsonObject msg = raw[i].toObject();
                QJsonValue senderValue = msg["sender_id"];
                QString senderIdStr =
                    senderValue.isString()
                        ? senderValue.toString()
                        : QString::number(senderValue.toInt());
                QString currentUserIdStr = QString::number(currentUserId);

                msg["is_me"] = (senderIdStr == currentUserIdStr);
                messages.append(msg);
            }

            if (beforeId > 0) {
                emit chatsHistoryPrepended(messages);
            } else {
                emit chatsHistoryLoaded(messages);
            }
        } else {
            emit chatError("Fetch history failed: " + reply->errorString());
        }
    });
}

void ChatManager::sendMessage(const QString &chatId, const QString &text) {
    QJsonObject json;
    json["text"] = text;

    QNetworkReply *reply = m_connection->post(
        "/chats/" + chatId + "/messages", QJsonDocument(json).toJson()
    );

    connect(reply, &QNetworkReply::finished, [this, reply, chatId]() {
        reply->deleteLater();
        if (reply->error() == QNetworkReply::NoError) {
            QJsonObject obj =
                QJsonDocument::fromJson(reply->readAll()).object();
            QJsonObject msg = obj["message"].toObject();
            msg["is_me"] = true;
            emit messageSentSuccess(msg);
        } else {
            emit chatError("Send message failed: " + reply->errorString());
        }
    });
}

void ChatManager::openDirectChat(
    int targetUserId,
    const QString &targetUserName
) {
    QJsonObject json;
    json["target_user_id"] = targetUserId;

    QNetworkReply *reply =
        m_connection->post("/chats/direct", QJsonDocument(json).toJson());

    connect(reply, &QNetworkReply::finished, [this, reply, targetUserName]() {
        reply->deleteLater();
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray responseData = reply->readAll();
#ifdef QT_DEBAG
            qDebug() << "[ChatManager] openDirectChat RAW JSON: "
                     << responseData;
#endif
            QJsonObject chat = QJsonDocument::fromJson(responseData)
                                   .object()["chat"]
                                   .toObject();
            QString chatId = QString::number(chat["id"].toInt());
            QString chatTitle = chat["title"].toString();
            if (chatTitle.isEmpty()) {
                chatTitle = chat["name"].toString();
                if (chatTitle.isEmpty() && !targetUserName.isEmpty()) {
                    chatTitle = targetUserName;
                }
            }
            emit directChatOpened(chatId, chatTitle);
            fetchChatHistory(chatId);
        } else {
            emit chatError("Open direct chat failed: " + reply->errorString());
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
    emit incomingWebSocketMessage(doc.object());
}

void ChatManager::onWebSocketError(QAbstractSocket::SocketError error) {
    qDebug() << "[ChatManager] WS error:" << error;
    emit chatError("WebSocket error: " + QString::number(error));
}
