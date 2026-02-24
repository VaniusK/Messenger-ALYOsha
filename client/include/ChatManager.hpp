#pragma once

#include <qabstractsocket.h>
#include <qglobal.h>
#include <qjsonarray.h>
#include <qjsonobject.h>
#include <qnetworkrequest.h>
#include <qtmetamacros.h>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>
#include <QString>
#include <QWebSocket>
#include "StateManager.hpp"

class ChatManager : public QObject {
    Q_OBJECT

public:
    explicit ChatManager(QObject *parent = nullptr);

    Q_INVOKABLE void searchUsers(const QString &querry);
    Q_INVOKABLE void fetchChats();
    Q_INVOKABLE void fetchChatHistory(const QString &chatId);
    Q_INVOKABLE void sendMessage(const QString &chatId, const QString &text);
    Q_INVOKABLE void connectWebSocket();

signals:
    void usersFound(const QJsonArray &users);
    void chatsUpdated(const QJsonArray &chats);
    void chatsHistoryLoaded(const QJsonArray &messages);
    void messageSentSuccess();
    void chatError(const QString &errorMsg);
    void webSocketConnected();
    void webSocketDisconnected();
    void incomingWebSocketMessage(const QJsonObject &data);

private slots:
    void onWebSocketConnected();
    void onWebSocketDisconnected();
    void onWebSocketTextMessageReceived(const QString &message);
    void onWebSocketError(QAbstractSocket::SocketError error);

private:
    QNetworkAccessManager *m_networkManager;
    QWebSocket *m_webSocket;
    const QString m_baseUrl = "http://127.0.0.1:5555/api/v1";
    const QString m_wsUrl = "ws://127.0.0.1:5555/api/v1/chat";

    QNetworkRequest createAuthenticatedRequest(const QString &endpoint);
    StateManager *getStateManager();
};
