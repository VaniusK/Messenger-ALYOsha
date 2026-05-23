#include "LocalChatStorage.hpp"
#include <qdir.h>
#include <qjsondocument.h>
#include <qsqldatabase.h>
#include <qstandardpaths.h>
#include <stdexcept>

LocalChatStorage::LocalChatStorage(QObject *parent)
    : QObject(parent), is_outdated(true) {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    auto data_location =
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);

    QDir data_dir(data_location);
    data_dir.mkpath(".");

    // db.setDatabaseName(":memory:");
    db.setDatabaseName(data_dir.filePath("chats.db"));

    if (!db.open()) {
        qDebug() << "Error: Could not open DB:" << db.lastError().text();
        // throw std::runtime_error("Couldn't open database for chats");
    } else {
        qDebug() << "DB opened";
    }
    static const char *MESSAGES_SCHEMA = R"(
    CREATE TABLE IF NOT EXISTS messages (
        id          INTEGER PRIMARY KEY,
        chat_id     INTEGER NOT NULL,
        text     TEXT NOT NULL,
        json_data   TEXT NOT NULL
    )
)";
    QSqlQuery query;
    query.exec(MESSAGES_SCHEMA);

    static const char *CHAT_PREVIEWS_SCHEMA = R"(
    CREATE TABLE IF NOT EXISTS chat_previews (
        id          INTEGER PRIMARY KEY,
        json_data   TEXT NOT NULL
    )
)";
    QSqlQuery chat_previews_query;
    chat_previews_query.exec(CHAT_PREVIEWS_SCHEMA);

    QSqlQuery indexQuery;
    indexQuery.prepare("CREATE INDEX idx_field ON messages(chat_id);");
    indexQuery.exec();
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
    } else {
        qDebug() << "Message added to DB";
    }
}

QJsonArray LocalChatStorage::getMessagesByChat(int64_t chat_id) {
    QSqlQuery query;
    query.prepare(
        "SELECT json_data FROM messages WHERE messages.chat_id = :chat_id "
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

std::optional<QJsonObject> LocalChatStorage::getOldestChatMessage(
    int64_t chat_id
) {
    QSqlQuery query;
    query.prepare(
        "SELECT json_data from messages WHERE messages.chat_id = :chat_id "
        "ORDER BY messages.chat_id ASC LIMIT 1"
    );
    query.bindValue(":chat_id", QVariant::fromValue(chat_id));
    if (!query.exec()) {
        qDebug() << "Error: Could't read oldest message from DB:"
                 << query.lastError().text();
    } else {
        qDebug() << "Reading oldest message from DB";
        while (query.next()) {
            QJsonDocument message =
                QJsonDocument::fromJson(query.value(0).toString().toUtf8());
            return message.object();
        }
        return std::nullopt;
    }
}

void LocalChatStorage::clear() {
    QSqlQuery query;
    query.prepare("DELETE FROM messages");
    if (!query.exec()) {
        qDebug() << "Error: Could't clear DB:" << query.lastError().text();
    } else {
        qDebug() << "Clearing DB";
    }
}

void LocalChatStorage::updateChatPreviews(const QJsonArray &chats) {
    QSqlQuery clear_query;
    clear_query.prepare("DELETE FROM chat_previews");
    if (!clear_query.exec()) {
        qDebug() << "Error: Could't clear Chat Previews DB:"
                 << clear_query.lastError().text();
    }
    QSqlQuery query;
    for (const QJsonValue &chat_value : chats) {
        QJsonDocument chat;
        chat.setObject(chat_value.toObject());
        QSqlQuery query;
        query.prepare(
            "INSERT INTO chat_previews (id, json_data) VALUES (:id, "
            ":json_data)"
        );
        query.bindValue(":id", chat["chat_id"].toInt());
        query.bindValue(
            ":json_data", QString::fromUtf8(chat.toJson(QJsonDocument::Compact))
        );
        if (!query.exec()) {
            qDebug() << "Error: Could not add message to DB:"
                     << query.lastError().text();
        } else {
            qDebug() << "Chat preview added to DB";
        }
    }
}

QJsonArray LocalChatStorage::getChatPreviews() {
    QSqlQuery query;
    query.prepare("SELECT json_data from chat_previews");
    if (!query.exec()) {
        qDebug() << "Error: Could't read chat previews from DB:"
                 << query.lastError().text();
    } else {
        qDebug() << "Reading chat previews from DB";
        QJsonArray chat_previews;
        while (query.next()) {
            QJsonDocument chat_preview =
                QJsonDocument::fromJson(query.value(0).toString().toUtf8());
            chat_previews.push_back(chat_preview.object());
        }
        return chat_previews;
    }
}
