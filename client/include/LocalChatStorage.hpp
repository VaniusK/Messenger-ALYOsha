#pragma once
#include <qjsondocument.h>
#include <qtmetamacros.h>
#include <QAbstractSocket>
#include <QJsonArray>
#include <QJsonObject>
#include <QObject>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QString>

class LocalChatStorage : public QObject {
    Q_OBJECT

public:
    explicit LocalChatStorage(
        bool should_use_in_memory_database,
        QString connectionName,
        QObject *parent = nullptr
    );

    void addMessage(QJsonObject message);
    QJsonArray getMessagesByChat(int64_t chat_id);
    std::optional<QJsonObject> getOldestChatMessage(int64_t chat_id);
    std::optional<QJsonObject> getLastChatMessage(int64_t chat_id);
    void clearChat(int64_t chat_id);
    void clear();
    void updateChatPreviews(const QJsonArray &chats);
    QJsonArray getChatPreviews();

private:
    const QString m_connectionName;
};
