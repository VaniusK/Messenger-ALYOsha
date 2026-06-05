#pragma once

#include <qtmetamacros.h>
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
    Q_INVOKABLE QString fetchSecretChatHistory(
        const QString &chat_id,
        qint64 before_timestamp = 0
    ) const;
    Q_INVOKABLE QString getOneMessage(const QString &message_id) const;
    Q_INVOKABLE QString sendSecretMessage(
        const QString &chat_id,
        const QString &text,
        const QString &type,
        const QJsonArray &attachments = QJsonArray()
    );                                                          // to impl
    Q_INVOKABLE void markChatAsRead(const QString &chat_id);    // to impl
    Q_INVOKABLE void deleteSecretChat(const QString &chat_id);  // to impl

    Q_INVOKABLE void clearSecretCache();
public slots:
    void processIncomingSecretPayload(const QJsonObject &envelope);
signals:
    // TODO: signal that cache cleared with size of cleared cache
    // TODO: signal for errors

private:
    client::db::SecretDatabaseManager *m_dbManager;
    StateManager *m_stateManager;
    ConnectionManager *m_connectionManager;

    void
    initSecretChat(const QString &chat_id, const QByteArray &other_public_key);
    void acceptSecretChatRequest(
        qint64 target_user_id,
        const QByteArray &other_public_key,
        const QString &chat_id
    );

    QByteArray m_publicKey;
    QByteArray m_privateKey;
};

}  // namespace client::core