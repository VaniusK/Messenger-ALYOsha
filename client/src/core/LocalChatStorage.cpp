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
        qDebug() << "Ошибка: не удалось подключить БД:"
                 << db.lastError().text();
        throw std::runtime_error("Couldn't open database for chats");
    } else {
        qDebug() << "БД успешно открыта!";
    }
    static const char *SCHEMA = R"(
    CREATE TABLE IF NOT EXISTS messages (
        id          INTEGER PRIMARY KEY,
        chat_id     INTEGER NOT NULL,
        json_data   TEXT NOT NULL
    )
)";
    QSqlQuery query;
    query.exec(SCHEMA);
}

void LocalChatStorage::addMessage(QJsonDocument message) {
}