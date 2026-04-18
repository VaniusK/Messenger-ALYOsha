#pragma once
#include <QAbstractSocket>
#include <QJsonArray>
#include <QJsonObject>
#include <QObject>
#include <QString>
#include <QWebSocket>
#include "ConnectionManager.hpp"
#include "StateManager.hpp"

class ChatManager : public QObject {
    Q_OBJECT

public:
    explicit ChatManager(
        ConnectionManager *connection,
        StateManager *stateManager,
        QObject *parent = nullptr
    );

    Q_INVOKABLE void searchUsers(const QString &querry);
    Q_INVOKABLE void fetchChats();
    Q_INVOKABLE void fetchChatHistory(const QString &chatId, int beforeId = 0);
    Q_INVOKABLE void sendMessage(const QString &chatId, const QString &text);
    Q_INVOKABLE void connectWebSocket();
    Q_INVOKABLE void
    openDirectChat(int targetUserId, const QString &targetUserName = "");
    Q_INVOKABLE void sendMessageWithAttachment(
        const QString &chatId,
        const QString &caption,
        const QString &fileName,
        const QString &fileType,
        qint64 fileSizeBytes,
        const QString &s3ObjectKey
    );

signals:
    void usersFound(const QJsonArray &users);
    void chatsUpdated(const QJsonArray &chats);
    void chatsHistoryLoaded(const QJsonArray &messages);
    void chatsHistoryPrepended(const QJsonArray &messages);
    void messageSentSuccess(const QJsonObject &message);
    void directChatOpened(const QString &chatId, const QString &chatTitle);
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
    ConnectionManager *m_connection;
    StateManager *m_stateManager;
    QWebSocket *m_webSocket;
};
