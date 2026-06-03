#include "include/SecretDatabaseManager.hpp"
#include <qsqldatabase.h>
#include <qsqlquery.h>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QString>

namespace client::db {

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

}  // namespace client::db
