#pragma once

#include <qglobal.h>
#include <qstringview.h>
#include <qtmetamacros.h>
#include <QObject>

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
        StateManager *stateManger,
        QObject *parent = nullptr
    );

    ~SecretChatManager() override = default;

    Q_INVOKABLE bool initSession();
    Q_INVOKABLE void logout();

    Q_INVOKABLE QString getSecretChatsPreviews() const;
    Q_INVOKABLE void createSecretChatRequest(qint64 target_user_id);
    Q_INVOKABLE void acceptSecretChatRequest(
        qint64 target_user_id,
        const QByteArray &other_public_key
    );
    Q_INVOKABLE void
    initSecretChat(qint64 target_user_id, const QByteArray &other_public_key);

private:
    client::db::SecretDatabaseManager *m_dbManager;
    StateManager *m_stateManager;

    QByteArray m_publicKey;
    QByteArray m_privateKey;
};

}  // namespace client::core