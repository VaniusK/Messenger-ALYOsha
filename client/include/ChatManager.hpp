#pragma once
#include <QAbstractSocket>
#include <QJsonArray>
#include <QJsonObject>
#include <QObject>
#include <QString>
#include <QWebSocket>
#include "ConnectionManager.hpp"
#include "LocalChatStorage.hpp"
#include "MediaCacheManager.hpp"
#include "StateManager.hpp"

class ChatManager : public QObject {
    Q_OBJECT

public:
    explicit ChatManager(
        ConnectionManager *connection,
        StateManager *stateManager,
        MediaCacheManager *media_cache,
        LocalChatStorage *chatStorage,
        QObject *parent = nullptr
    );

    Q_INVOKABLE void searchUsers(const QString &querry);
    Q_INVOKABLE void fetchChats();
    Q_INVOKABLE void fetchChatHistory(const QString &chatId, int beforeId = 0);
    Q_INVOKABLE void sendMessage(const QString &chatId, const QString &text);
    Q_INVOKABLE void connectWebSocket();
    Q_INVOKABLE void
    openDirectChat(int targetUserId, const QString &targetUserName = "");
    Q_INVOKABLE void cacheMessageMedia(QJsonObject &message);
    Q_INVOKABLE void clearCache();
    Q_INVOKABLE void createGroupChat(
        const QString &name,
        const QString &description,
        const QVariantList &memberIds
    );
    Q_INVOKABLE void fetchChatMembers(const QString &chatId);
    Q_INVOKABLE void addChatMember(
        const QString &chatId,
        qint64 userId,
        const QString &role = "member"
    );
    Q_INVOKABLE void removeChatMember(
        const QString &chatId,
        qint64 userId,
        bool fetchAfter = true
    );
    Q_INVOKABLE void updateChatInfo(
        const QString &chatId,
        const QString &newName,
        const QString &newDescription
    );
    Q_INVOKABLE void changeMemberRole(
        const QString &chatId,
        qint64 userId,
        const QString &newRole
    );
    Q_INVOKABLE void fetchChatInfo(const QString &chatId);
    Q_INVOKABLE void
    createSecretChat(qint64 targetUserId, const QString &targetUserName);
    Q_INVOKABLE void
    fetchSecretChatHistory(const QString &chatId, int beforeId = 0);
    Q_INVOKABLE void
    sendSecretMessage(const QString &chatId, const QString &text);
    Q_INVOKABLE void uploadSecretFile(
        const QString &chatId,
        const QString &filePath,
        bool asFile,
        const QString &caption,
        const QString &msgtype
    );
    // Q_INVOKABLE void deleteSecretChat(const QString &chatId);

signals:
    void usersFound(const QJsonArray &users);
    void chatsUpdated(const QJsonArray &chats);
    void chatsHistoryLoaded(const QJsonArray &messages);
    void chatsHistoryPrepended(const QJsonArray &messages);
    void messageSentSuccess(const QJsonObject &msg);
    void directChatOpened(const QString &chatId, const QString &chatTitle);
    void chatError(const QString &errorMsg);
    void webSocketConnected();
    void webSocketDisconnected();
    void incomingWebSocketMessage(const QJsonObject &data);
    void groupChatCreated(const QJsonObject &chat);
    void chatMembersLoaded(const QJsonArray &members);
    void chatMemberAdded(const QJsonObject &member);
    void actionSuccess(const QString &message);
    void chatInfoLoaded(const QJsonObject &chat);

private slots:
    void onWebSocketConnected();
    void onWebSocketDisconnected();
    void onWebSocketTextMessageReceived(const QString &message);
    void onWebSocketError(QAbstractSocket::SocketError error);

private:
    ConnectionManager *m_connection;
    StateManager *m_stateManager;
    MediaCacheManager *m_mediaCache;
    LocalChatStorage *m_chatStorage;
    QWebSocket *m_webSocket;
    void handleIncomingSecretPayload(const QJsonObject &payload);
};
