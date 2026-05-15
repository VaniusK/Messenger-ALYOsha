#include "LocalChatStorage.hpp"
#include <qdir.h>
#include <qsqldatabase.h>
#include <qstandardpaths.h>
#include <stdexcept>

LocalChatStorage::LocalChatStorage(QObject *parent) : QObject(parent) {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    auto data_location =
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);

    QDir data_dir(QDir::cleanPath(
        QDir(data_location).path() + QDir::separator() + "chats.db"
    ));

    db.setDatabaseName(":memory:");
    // TODO: хранение между сессиями
    // db.setDatabaseName(data_dir.path());

    if (!db.open()) {
        qDebug() << "Error: Could not open DB:" << db.lastError().text();
        // throw std::runtime_error("Couldn't open database for chats");
    } else {
        qDebug() << "DB opened";
    }
    static const char *SCHEMA = R"(
    CREATE TABLE IF NOT EXISTS messages (
        id          INTEGER PRIMARY KEY,
        chat_id     INTEGER NOT NULL,
        text     TEXT NOT NULL,
        json_data   TEXT NOT NULL
    )
)";
    QSqlQuery query;
    query.exec(SCHEMA);
}

void LocalChatStorage::addMessage(QJsonObject message_object) {
    QJsonDocument message(message_object);
    QSqlQuery query;
    query.prepare(
        "INSERT INTO messages (id, chat_id, text, json_data) VALUES (:id, "
        ":chat_id, "
        ":text,"
        ":json_data)"
    );
    query.bindValue(":id", message["id"].toInt());
    query.bindValue(":chat_id", message["chat_id"].toInt());
    query.bindValue(":text", message["text"].toString());
    query.bindValue(
        ":json_data", QString::fromUtf8(message.toJson(QJsonDocument::Compact))
    );
    if (!query.exec()) {
        qDebug() << "Error: Could not add message to DB:"
                 << query.lastError().text();
        // throw std::runtime_error("Couldn't add message to DB");
    } else {
        qDebug() << "Message added to DB";
    }
}

QJsonArray LocalChatStorage::getMessagesByChat(int64_t chat_id) {
    QSqlQuery query;
    query.prepare(
        "SELECT json_data from messages WHERE messages.chat_id = :chat_id "
        "ORDER BY messages.chat_id ASC"
    );
    query.bindValue(":chat_id", QVariant::fromValue(chat_id));
    if (!query.exec()) {
        qDebug() << "Error: Could't read messages from DB:"
                 << query.lastError().text();
    } else {
        qDebug() << "Reading messages from DB";
        QJsonArray messages;
        while (query.next()) {
            QJsonDocument message =
                QJsonDocument::fromJson(query.value(0).toString().toUtf8());
            messages.push_back(message.object());
        }
        return messages;
    }
}