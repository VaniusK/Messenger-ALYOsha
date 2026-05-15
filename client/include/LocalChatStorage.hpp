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
    explicit LocalChatStorage(QObject *parent = nullptr);

    void addMessage(QJsonObject message);
    QJsonArray getMessagesByChat(int64_t chat_id);
    std::optional<QJsonObject> getOldestChatMessage(int64_t chat_id);
    void clear();
};
