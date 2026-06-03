#pragma once

#include <qglobal.h>
#include <QObject>
#include <QSqlDatabase>
#include <QString>

namespace client::db {
inline constexpr const char *SECURE_DB_CONNECTION = "SecureE2EConnection";

inline constexpr const char *SCHEMA_IDENTITY = R"(
    CREATE TABLE IF NOT EXISTS user_identity (
        id INTEGER PRIMARY KEY CHECK (id = 1),
        public_key BLOB NOT NULL,
        private_key BLOB NOT NULL
    );
    )";

inline constexpr const char *SCHEMA_CHATS = R"(
    CREATE TABLE IF NOT EXISTS secret_chats (
        id TEXT PRIMARY KEY,
        type TEXT NOT NULL,
        peer_id BIGINT NOT NULL,
        name TEXT,
        last_message TEXT,
        unread_count INTEGER DEFAULT 0,
        shared_secret BLOB NOT NULL,
        status TEXT NOT NULL DEFAULT
    );
    )";

inline constexpr const char *SCHEMA_MESSAGES = R"(
    CREATE TABLE IF NOT EXISTS secret_messages (
        id TEXT PRIMARY KEY,
        chat_id TEXT NOT NULL,
        sender_id BIGINT NOT NULL,
        message_type TEXT NOT NULL,
        text TEXT,
        sent_at TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
        FOREIGN KEY(chat_id) REFERENCES secret_chats(id) ON DELETE CASCADE
    );
)";

inline constexpr const char *SCHEMA_ATTACHMENTS = R"(
    CREATE TABLE IF NOT EXISTS secret_attachments (
        id TEXT PRIMARY KEY,
        message_id TEXT NOT NULL,
        file_name TEXT NOT NULL,
        file_size_bytes BIGINT NOT NULL,
        file_type TEXT NOT NULL,
        s3_object_key TEXT NOT NULL,
        local_path TEXT,
        file_key BLOB,
        FOREIGN KEY(message_id) REFERENCES secret_messages(message_id) ON DELETE CASCADE
    );
)";

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

    bool createRequestedChat(qint64 chat_id, );
    bool createChat(
        qint64 chat_id,
        const QString &type,
        qint64 peer_id,
        const QString &title,
        const QByteArray &shared_secret
    );
    bool updateLastMessage(qint64 chat_id, const QString &last_message_json);
    QByteArray getSharedSecret(qint64 chat_id);
    bool saveMessage(
        const QString &message_id,
        qint64 chat_id,
        bool is_outgoing,
        const QString &content_type,
        const QString &text,
        qint64 timestamp,
        const QString &status
    );
    bool saveAttachment(
        const QString &attachment_id,
        const QString &message_id,
        const QString &file_name,
        qint64 file_size,
        const QString &file_type,
        const QString &local_path,
        const QByteArray &file_key
    );

    Q_INVOKABLE QString getChatsJson();
    Q_INVOKABLE QString
    getMessagesJson(qint64 chat_id, int limit = 50, int offset = 0);
    Q_INVOKABLE QString getMessageJson(const QString &message_id);

private:
    bool createTables();
    QSqlDatabase getDatabase() const;
};
}  // namespace client::db
