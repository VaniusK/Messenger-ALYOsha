#pragma once

#include <QJsonArray>
#include <QObject>
#include "ConnectionManager.hpp"

namespace client::db {
class SecretDatabaseManager;
}

class StateManager;

namespace client::core {

class SecretChatManager : public QObject {
    Q_OBJECT

public:
    explicit SecretChatManager(
        client::db::SecretDatabaseManager *dbManager,
        StateManager *stateManager,
        ConnectionManager *connectionManager,
        QObject *parent = nullptr
    );

    ~SecretChatManager() override = default;

    Q_INVOKABLE bool initSession();
    Q_INVOKABLE void logout();

    Q_INVOKABLE QString getSecretChatsPreviews() const;
    Q_INVOKABLE void createSecretChatRequest(qint64 target_user_id);
    Q_INVOKABLE QString
    fetchSecretChatHistory(const QString &chat_id, qint64 before_timestamp = 0);
    Q_INVOKABLE QString getOneMessage(const QString &message_id) const;
    Q_INVOKABLE QString sendSecretMessage(
        const QString &chat_id,
        const QString &text,
        const QString &messageType,
        const QVector<QString> &filepaths = QVector<QString>()
    );
    Q_INVOKABLE void
    downloadSecretAttachment(const QString &message_id, const QString &file_id);
    Q_INVOKABLE void markChatAsRead(const QString &chat_id);
    Q_INVOKABLE void deleteSecretChat(const QString &chat_id);

    Q_INVOKABLE void clearSecretCache();
public slots:
    void processIncomingSecretPayload(const QJsonObject &envelope);
signals:
    void secretChatsUpdated();
    void secretChatCreated(const QString &chatId, const QString &title);
    void secretMessageReceived(const QJsonObject &message);
    void secretChatError(const QString &errorMsg);
    void attachmentDownloaded(
        const QString &messageId,
        const QString &fileId,
        const QString &fileUrl
    );
    void
    attachmentDownloadFailed(const QString &messageId, const QString &fileId);

private:
    client::db::SecretDatabaseManager *m_dbManager;
    StateManager *m_stateManager;
    ConnectionManager *m_connectionManager;
    QSet<QString> m_activeDownloads;

    void
    initSecretChat(const QString &chat_id, const QByteArray &other_public_key);
    void encodeAndUploadSecretFiles(
        const QString &message_id,
        const QVector<QString> &filepaths,
        std::function<void(QJsonArray)> on_complete
    );
    void acceptSecretChatRequest(
        qint64 target_user_id,
        const QByteArray &other_public_key,
        const QString &chat_id
    );
    static QString getMimeType(const QString &filePath);

    void handleIncomingSecretMessage(const QJsonObject &envelope);
    void handleChatRead(const QJsonObject &envelope);

    QByteArray m_publicKey;
    QByteArray m_privateKey;
};

}  // namespace client::core