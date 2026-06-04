#pragma once

#include <qglobal.h>
#include <qtmetamacros.h>
#include <QObject>
#include <QSqlDatabase>
#include <QString>
#include <optional>

namespace client::db {
inline constexpr const char *SECURE_DB_CONNECTION = "SecureE2EConnection";

struct UserIdentity {
    QByteArray publicKey;
    QByteArray privateKey;
};

class SecretDatabaseManager : public QObject {
    Q_OBJECT

public:
    explicit SecretDatabaseManager(QObject *parent = nullptr) {
    }

    ~SecretDatabaseManager();

    bool init(const QString &db_path);
    void logout();

    bool
    saveIdentity(const QByteArray &public_key, const QByteArray &private_key);
    std::optional<UserIdentity> getIdentity();

    bool createChat(
        const QString &chat_id,
        const QString &type,
        qint64 peer_id,
        const QString &name,
        const QByteArray &shared_secret,
        const QString &status
    );
    QByteArray getSharedSecret(const QString &chat_id);
    bool saveMessage(
        const QString &message_id,
        const QString &chat_id,
        qint64 sender_id,
        const QString &message_type,
        const QString &text,
        qint64 sent_at
    );
    bool saveAttachment(
        const QString &id,
        const QString &message_id,
        const QString &file_name,
        qint64 file_size,
        const QString &file_type,
        const QString &s3_object_key,
        const std::optional<QString> &local_path,
        const QByteArray &file_key
    );

    Q_INVOKABLE QString getChatsJson(qint64 current_user_id);
    Q_INVOKABLE QString getMessagesJson(
        const QString &chat_id,
        int limit = 50,
        qint64 before_timestamp = 0
    );
    Q_INVOKABLE QString getMessageJson(const QString &message_id);
    Q_INVOKABLE bool
    markChatAsRead(const QString &chat_id, qint64 current_user_id);

private:
    bool createTables();
    QSqlDatabase getDatabase() const;
};
}  // namespace client::db
