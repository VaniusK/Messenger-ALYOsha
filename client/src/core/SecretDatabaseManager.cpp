#include "include/SecretDatabaseManager.hpp"
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QString>
#include <QVariant>
#include <optional>

namespace client::db {

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
        peer_id INTEGER NOT NULL,
        name TEXT,
        shared_secret BLOB NOT NULL,
        status TEXT NOT NULL
    );
)";

inline constexpr const char *SCHEMA_MESSAGES = R"(
    CREATE TABLE IF NOT EXISTS secret_messages (
        id TEXT PRIMARY KEY,
        chat_id TEXT NOT NULL,
        sender_id INTEGER NOT NULL,
        message_type TEXT NOT NULL,
        text TEXT,
        sent_at INTEGER NOT NULL,
        is_read INTEGER DEFAULT 0,
        FOREIGN KEY(chat_id) REFERENCES secret_chats(id) ON DELETE CASCADE
    );

    CREATE INDEX IF NOT EXISTS idx_messages_chat_time 
    ON secret_messages (chat_id, sent_at DESC);

    CREATE INDEX IF NOT EXISTS idx_messages_unread 
    ON secret_messages (chat_id, is_read, sender_id);
)";

inline constexpr const char *SCHEMA_ATTACHMENTS = R"(
    CREATE TABLE IF NOT EXISTS secret_attachments (
        id TEXT PRIMARY KEY,
        message_id TEXT NOT NULL,
        file_name TEXT NOT NULL,
        file_size_bytes INTEGER NOT NULL,
        file_type TEXT NOT NULL,
        s3_object_key TEXT NOT NULL,
        local_path TEXT,
        file_key BLOB NOT NULL,
        FOREIGN KEY(message_id) REFERENCES secret_messages(id) ON DELETE CASCADE
    );
)";

SecretDatabaseManager::~SecretDatabaseManager() {
    logout();
}

void SecretDatabaseManager::logout() {
    if (QSqlDatabase::contains(SECURE_DB_CONNECTION)) {
        {
            QSqlDatabase db = getDatabase();
            if (db.isOpen()) {
                db.close();
            }
        }
        QSqlDatabase::removeDatabase(SECURE_DB_CONNECTION);
        qDebug() << "[SecretDB] Database closed and connection removed.";
    }
}

QSqlDatabase SecretDatabaseManager::getDatabase() const {
    return QSqlDatabase::database(SECURE_DB_CONNECTION);
}

bool SecretDatabaseManager::init(const QString &db_path) {
    QFileInfo file_info(db_path);
    QDir().mkpath(file_info.absolutePath());

    QSqlDatabase db;
    if (QSqlDatabase::contains(SECURE_DB_CONNECTION)) {
        db = getDatabase();
    } else {
        db = QSqlDatabase::addDatabase("QSQLITE", SECURE_DB_CONNECTION);
    }

    db.setDatabaseName(db_path);

    if (!db.open()) {
        qCritical() << "[SecretDB] Failed to open secure database:"
                    << db.lastError().text();
        return false;
    }

    QSqlQuery pragma_query(db);
    pragma_query.exec("PRAGMA foreign_keys = ON;");

    return createTables();
}

bool SecretDatabaseManager::createTables() {
    QSqlDatabase db = getDatabase();
    QSqlQuery query(db);

    auto execute_sql =
        [&query](const char *sql, const char *table_name) -> bool {
        if (!query.exec(sql)) {
            qCritical() << "[SecretDB] Failed to create table" << table_name
                        << ":" << query.lastError().text();
            return false;
        }
        return true;
    };

    if (!execute_sql(SCHEMA_IDENTITY, "user_identity")) {
        return false;
    }
    if (!execute_sql(SCHEMA_CHATS, "secret_chats")) {
        return false;
    }
    if (!execute_sql(SCHEMA_MESSAGES, "secret_messages")) {
        return false;
    }
    if (!execute_sql(SCHEMA_ATTACHMENTS, "secret_attachments")) {
        return false;
    }

    qDebug() << "[SecretDB] All secure tables initialized successfully.";
    return true;
}

bool SecretDatabaseManager::saveIdentity(
    const QByteArray &public_key,
    const QByteArray &private_key
) {
    QSqlDatabase db = getDatabase();
    QSqlQuery query(db);

    query.prepare(
        "INSERT OR REPLACE INTO user_identity (id, public_key, private_key) "
        "VALUES (1, :pub, :priv)"
    );
    query.bindValue(":pub", public_key);
    query.bindValue(":priv", private_key);

    if (!query.exec()) {
        qCritical() << "[SecretDB] Failed to save identity:"
                    << query.lastError().text();
        return false;
    }
    return true;
}

std::optional<UserIdentity> SecretDatabaseManager::getIdentity() {
    QSqlDatabase db = getDatabase();
    QSqlQuery query(db);
    query.prepare(
        "SELECT public_key, private_key FROM user_identity WHERE id = 1"
    );

    if (query.exec() && query.next()) {
        UserIdentity identity;
        identity.publicKey = query.value("public_key").toByteArray();
        identity.privateKey = query.value("private_key").toByteArray();
        return identity;
    }
    return std::nullopt;
}

bool SecretDatabaseManager::createChat(
    const QString &chat_id,
    const QString &type,
    qint64 peer_id,
    const QString &name,
    const QByteArray &shared_secret,
    const QString &status
) {
    QSqlDatabase db = getDatabase();
    QSqlQuery query(db);
    query.prepare(
        R"(INSERT INTO secret_chats (id, type, peer_id, name, shared_secret, status) 
                 VALUES (:id, :type, :peer_id, :name, :shared_secret, :status)
    )"
    );
    query.bindValue(":id", chat_id);
    query.bindValue(":type", type);
    query.bindValue(":peer_id", peer_id);
    query.bindValue(":name", name);
    query.bindValue(":shared_secret", shared_secret);
    query.bindValue(":status", status);

    if (!query.exec()) {
        qCritical() << "[SecretDB] Failed to create chat:"
                    << query.lastError().text();
        return false;
    }

    // TODO : add emit for QML

    return true;
}

QByteArray SecretDatabaseManager::getSharedSecret(const QString &chat_id) {
    auto db = getDatabase();
    QSqlQuery query(db);

    query.prepare("SELECT shared_secret FROM secret_chats WHERE id = :chat_id");
    query.bindValue(":chat_id", chat_id);

    if (query.exec() && query.next()) {
        return query.value("shared_secret").toByteArray();
    }
    qWarning() << "[SecretDB] Shared secret not found for chat:" << chat_id;
    return QByteArray();
}

bool SecretDatabaseManager::saveMessage(
    const QString &message_id,
    const QString &chat_id,
    qint64 sender_id,
    const QString &message_type,
    const QString &text,
    qint64 sent_at
) {
    auto db = getDatabase();
    QSqlQuery query(db);

    query.prepare(
        R"(INSERT INTO secret_messages (id, chat_id, sender_id, message_type, text, sent_at) VALUES (:id, :chat_id, :sender_id, :message_type, :text, :sent_at))"
    );
    query.bindValue(":id", message_id);
    query.bindValue(":chat_id", chat_id);
    query.bindValue(":sender_id", sender_id);
    query.bindValue(":message_type", message_type);
    query.bindValue(":text", text);
    query.bindValue(":sent_at", sent_at);

    if (!query.exec()) {
        qCritical() << "[SecretDB] Failed to save message:"
                    << query.lastError().text();
        return false;
    }

    // TODO emit for qml

    return true;
}

bool SecretDatabaseManager::saveAttachment(
    const QString &id,
    const QString &message_id,
    const QString &file_name,
    qint64 file_size,
    const QString &file_type,
    const QString &s3_object_key,
    const std::optional<QString> &local_path,
    const QByteArray &file_key
) {
    QSqlDatabase db = getDatabase();
    QSqlQuery query(db);

    query.prepare(R"(
        INSERT INTO secret_attachments (
            id, message_id, file_name, file_size_bytes, 
            file_type, s3_object_key, local_path, file_key
        ) VALUES (
            :id, :msg_id, :name, :size, 
            :type, :s3_key, :local_path, :key
        )
    )");

    query.bindValue(":id", id);
    query.bindValue(":msg_id", message_id);
    query.bindValue(":name", file_name);
    query.bindValue(":size", file_size);
    query.bindValue(":type", file_type);
    query.bindValue(":s3_key", s3_object_key);
    if (local_path.has_value()) {
        query.bindValue(":local_path", local_path.value());
    } else {
        query.bindValue(
            ":local_path", QVariant(QMetaType::fromType<QString>())
        );
    }

    query.bindValue(":key", file_key);

    if (!query.exec()) {
        qCritical() << "[SecretDB] Failed to save attachment:"
                    << query.lastError().text();
        return false;
    }

    return true;
}

QString SecretDatabaseManager::getChatsJson(qint64 current_user_id) {
    QSqlDatabase db = getDatabase();
    QSqlQuery query(db);

    query.prepare(R"(
        SELECT
            c.id,
            c.type,
            c.peer_id,
            c.name,
            c.status,
            (
                SELECT COUNT(*)
                FROM secret_messages
                WHERE chat_id = c.id
                  AND is_read = 0
                  AND sender_id != :my_id
            ) AS unread_count,
            m.id AS lm_id, 
            m.sender_id AS lm_sender_id, 
            m.message_type AS lm_type, 
            m.text AS lm_text, 
            m.sent_at AS lm_sent_at,
            m.is_read AS lm_is_read
        FROM secret_chats c
        LEFT JOIN secret_messages m ON m.id = (
            SELECT id FROM secret_messages 
            WHERE chat_id = c.id 
            ORDER BY sent_at DESC LIMIT 1
        )
        GROUP BY c.id
        ORDER BY m.sent_at DESC NULLS LAST
        )");

    query.bindValue(":my_id", current_user_id);

    if (!query.exec()) {
        qCritical() << "[SecretDB] Failed to get chats:"
                    << query.lastError().text();
        return "[]";
    }

    QJsonArray chats_array;
    while (query.next()) {
        QJsonObject chat_obj;
        chat_obj["id"] = query.value("id").toString();
        chat_obj["type"] = query.value("type").toString();
        chat_obj["peer_id"] = query.value("peer_id").toLongLong();
        chat_obj["name"] = query.value("name").toString();
        chat_obj["unread_count"] = query.value("unread_count").toInt();
        chat_obj["status"] = query.value("status").toString();

        if (query.value("lm_id").isNull()) {
            chat_obj["last_message"] = QJsonValue::Null;
        } else {
            QJsonObject last_msg_obj;
            last_msg_obj["id"] = query.value("lm_id").toString();
            last_msg_obj["sender_id"] =
                query.value("lm_sender_id").toLongLong();
            last_msg_obj["message_type"] = query.value("lm_type").toString();
            last_msg_obj["text"] = query.value("lm_text").toString();
            last_msg_obj["is_read"] = query.value("lm_is_read").toInt();

            qint64 ts_msecs = query.value("lm_sent_at").toLongLong();
            QDateTime dt = QDateTime::fromMSecsSinceEpoch(ts_msecs, Qt::UTC);
            last_msg_obj["sent_at"] = dt.toString(Qt::ISODateWithMs);

            chat_obj["last_message"] = last_msg_obj;
        }

        chats_array.append(chat_obj);
    }
    return QString::fromUtf8(
        QJsonDocument(chats_array).toJson(QJsonDocument::Compact)
    );
}

QString SecretDatabaseManager::getMessagesJson(
    const QString &chat_id,
    int limit,
    qint64 before_timestamp
) {
    QSqlDatabase db = getDatabase();
    QSqlQuery query(db);

    QString sql = R"(
        SELECT id, sender_id, message_type, text, sent_at, is_read
        FROM secret_messages 
        WHERE chat_id = :chat_id 
    )";

    if (before_timestamp > 0) {
        sql += " AND sent_at < :before_ts ";
    }

    sql += " ORDER BY sent_at DESC LIMIT :limit";

    query.prepare(sql);
    query.bindValue(":chat_id", chat_id);
    if (before_timestamp > 0) {
        query.bindValue(":before_ts", before_timestamp);
    }
    query.bindValue(":limit", limit);

    if (!query.exec()) {
        qCritical() << "[SecretDB] Failed to get messages:"
                    << query.lastError().text();
        return "[]";
    }

    QJsonArray messages_array;
    while (query.next()) {
        QJsonObject msg_obj;
        msg_obj["id"] = query.value("id").toString();
        msg_obj["chat_id"] = chat_id;
        msg_obj["sender_id"] = query.value("sender_id").toLongLong();
        msg_obj["message_type"] = query.value("message_type").toString();
        msg_obj["text"] = query.value("text").toString();
        msg_obj["is_read"] = query.value("is_read").toInt();

        qint64 ts_msecs = query.value("sent_at").toLongLong();
        QDateTime dt = QDateTime::fromMSecsSinceEpoch(ts_msecs, Qt::UTC);
        msg_obj["sent_at"] = dt.toString(Qt::ISODateWithMs);

        messages_array.append(msg_obj);
    }

    return QString::fromUtf8(
        QJsonDocument(messages_array).toJson(QJsonDocument::Compact)
    );
}

QString SecretDatabaseManager::getMessageJson(const QString &message_id) {
    QSqlDatabase db = getDatabase();
    QSqlQuery query(db);

    query.prepare(R"(
        SELECT id, chat_id, sender_id, message_type, text, sent_at, is_read
        FROM secret_messages 
        WHERE id = :id 
    )");

    query.bindValue(":id", message_id);

    if (!query.exec()) {
        qCritical() << "[SecretDB] Failed to get message:"
                    << query.lastError().text();
        return "";
    }

    if (!query.next()) {
        return "{}";
    }

    QJsonObject msg_obj;
    msg_obj["id"] = query.value("id").toString();
    msg_obj["chat_id"] = query.value("chat_id").toString();
    msg_obj["sender_id"] = query.value("sender_id").toLongLong();
    msg_obj["message_type"] = query.value("message_type").toString();
    msg_obj["text"] = query.value("text").toString();
    msg_obj["is_read"] = query.value("is_read").toInt();

    qint64 ts_msecs = query.value("sent_at").toLongLong();
    QDateTime dt = QDateTime::fromMSecsSinceEpoch(ts_msecs, Qt::UTC);
    msg_obj["sent_at"] = dt.toString(Qt::ISODateWithMs);

    return QString::fromUtf8(
        QJsonDocument(msg_obj).toJson(QJsonDocument::Compact)
    );
}

bool SecretDatabaseManager::markChatAsRead(
    const QString &chat_id,
    qint64 current_user_id
) {
    QSqlDatabase db = getDatabase();
    QSqlQuery query(db);

    query.prepare(R"(
        UPDATE secret_messages 
        SET is_read = 1 
        WHERE chat_id = :chat_id AND sender_id != :my_id AND is_read = 0)");

    query.bindValue(":chat_id", chat_id);
    query.bindValue(":my_id", current_user_id);

    if (query.exec() && query.numRowsAffected() > 0) {
        // TODO : emit for qml
        return true;
    }
    return false;
}

bool SecretDatabaseManager::deleteChat(const QString &chat_id) {
    auto db = getDatabase();
    QSqlQuery query(db);
    query.prepare("DELETE FROM secret_chats WHERE id = :chat_id");
    query.bindValue(":chat_id", chat_id);

    if (!query.exec()) {
        qCritical() << "[SecretDB] Failed to delete chat:"
                    << query.lastError().text();
        return false;
    }

    if (query.numRowsAffected() == 0) {
        qWarning() << "[SecretDB] Chat with ID" << chat_id
                   << "was not found for deletion.";
        return false;
    }

    // TODO: emit
    return true;
}

bool SecretDatabaseManager::updateChatStatus(
    const QString &chat_id,
    const QString &status,
    const QByteArray &shared_secret
) {
    auto db = getDatabase();
    QSqlQuery query(db);
    query.prepare(
        "UPDATE secret_chats SET status = :status, shared_secret = "
        ":shared_secret WHERE chat_id = :chat_id"
    );
    query.bindValue(":status", status);
    query.bindValue(":shared_secret", shared_secret);
    query.bindValue(":chat_id", chat_id);

    if (!query.exec()) {
        qCritical() << "[SecretDB] Failed to update chat status:"
                    << query.lastError().text();
        return false;
    }

    if (query.numRowsAffected() == 0) {
        qWarning() << "[SecretDB] Chat with ID" << chat_id
                   << "was not found. Status update skipped.";
        return false;
    }

    qDebug() << "[SecretDB] Chat" << chat_id << "status successfully updated to"
             << status;
    return true;
}

}  // namespace client::db
