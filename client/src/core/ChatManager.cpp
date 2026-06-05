#include "ChatManager.hpp"
#include <QDebug>
#include <QImageReader>
#include <QJsonDocument>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrlQuery>

ChatManager::ChatManager(
    ConnectionManager *connection,
    StateManager *stateManager,
    MediaCacheManager *mediaCache,
    LocalChatStorage *chatStorage,
    QObject *parent
)
    : QObject(parent),
      m_connection(connection),
      m_stateManager(stateManager),
      m_mediaCache(mediaCache),
      m_chatStorage(chatStorage) {
    m_webSocket =
        new QWebSocket(QString(), QWebSocketProtocol::VersionLatest, this);

    connect(
        m_webSocket, &QWebSocket::connected, this,
        &ChatManager::onWebSocketConnected
    );
    connect(
        m_webSocket, &QWebSocket::disconnected, this,
        &ChatManager::onWebSocketDisconnected
    );
    connect(
        m_webSocket, &QWebSocket::textMessageReceived, this,
        &ChatManager::onWebSocketTextMessageReceived
    );
    connect(
        m_webSocket,
        QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error), this,
        &ChatManager::onWebSocketError
    );

    connect(
        m_webSocket, &QWebSocket::sslErrors, this,
        [this](const QList<QSslError> &errors) {
            QString host = m_webSocket->requestUrl().host();
            qDebug() << "[ChatManager] WebSocket SSL Errors for"
                     << m_webSocket->requestUrl().toString();
            for (const auto &error : errors) {
                qDebug() << "  -" << error.errorString();
            }

            if (host == "api.localhost" || host == "127.0.0.1") {
                qDebug() << "[ChatManager] Automatically ignoring SSL errors "
                            "for local host.";
                m_webSocket->ignoreSslErrors();
            }
        }
    );
}

void ChatManager::connectWebSocket() {
    StateManager *sm = m_stateManager;
    if (!sm || sm->getToken().isEmpty()) {
        return;
    }

    QNetworkRequest request(QUrl(m_connection->wsUrl()));
    request.setRawHeader("Authorization", "Bearer " + sm->getToken().toUtf8());
    m_webSocket->open(request);
}

void ChatManager::searchUsers(const QString &query) {
    QUrlQuery urlQuery;
    urlQuery.addQueryItem("query", query);
    urlQuery.addQueryItem("limit", "50");

    QNetworkReply *reply =
        m_connection->get("/users/search?" + urlQuery.toString());

    connect(reply, &QNetworkReply::finished, [this, reply]() {
        reply->deleteLater();
        if (reply->error() == QNetworkReply::NoError) {
            QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
            QJsonArray results = doc.object()["results"].toArray();
            emit usersFound(results);
        } else {
            emit chatError("Search failed: " + reply->errorString());
        }
    });
}

void ChatManager::fetchChats() {
    StateManager *sm = m_stateManager;
    if (!sm || sm->getUserId() <= 0) {
#ifdef QT_DEBAG
        qDebug() << "[ChatManager] fetchChats skipped. Invalid state manager "
                    "or userId:"
                 << (sm ? sm->getUserId() : -1);
#endif
        return;
    }

#ifdef QT_DEBAG
    qDebug() << "[ChatManager] fetchChats called for user ID:"
             << sm->getUserId();
#endif

    QJsonArray currentChats = m_chatStorage->getChatPreviews();
    emit chatsUpdated(currentChats);

    QNetworkReply *reply =
        m_connection->get("/chats/user/" + QString::number(sm->getUserId()));

    connect(reply, &QNetworkReply::finished, [this, reply]() {
        reply->deleteLater();
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray responseData = reply->readAll();
#ifdef QT_DEBAG
            qDebug() << "[ChatManager] fetchChats RAW JSON: " << responseData;
#endif
            QJsonArray previews = (QJsonDocument::fromJson(responseData))
                                      .object()["chats"]
                                      .toArray();
            qDebug() << "[ChatManager] Got previews with size "
                     << previews.size();
            m_chatStorage->updateChatPreviews(previews);
            for (const QJsonValue &preview : previews) {
                int64_t chat_id = preview["chat_id"].toInt();
                auto last_saved_message_optional =
                    m_chatStorage->getLastChatMessage(chat_id);
                if (!last_saved_message_optional.has_value()) {
                    continue;
                }
                auto last_saved_message = last_saved_message_optional.value();
                if (last_saved_message["id"].toInt() !=
                    preview["last_message"]["id"].toInt()) {
                    m_chatStorage->clearChat(chat_id);
                    qDebug() << "[ChatManager] last saved id is "
                             << last_saved_message["id"].toInt();
                    qDebug() << "yet server sent "
                             << preview["last_message"]["id"].toInt();
                }
            }

            QJsonArray combinedChats = m_chatStorage->getChatPreviews();
            // TODO LOCAL DB
            // получить вектор секретных чатов, конвертируем в QJsonObject и
            // combinedChats.append(secretChat) QML сам сортирует по времени
            // last_message

            emit chatsUpdated(combinedChats);
        } else {
            emit chatError("Fetch chats failed: " + reply->errorString());
        }
    });
}

void ChatManager::fetchChatHistory(const QString &chatId, int beforeId) {
    int64_t chat_id = chatId.toLongLong();
    qDebug() << "[ChatManager] fetchChatHistory() called with " << beforeId;
    std::optional<QJsonObject> oldest_message =
        m_chatStorage->getOldestChatMessage(chat_id);
    if (oldest_message.has_value() &&
        (beforeId == 0 || beforeId != oldest_message.value()["id"].toInt())) {
        emit chatsHistoryLoaded(m_chatStorage->getMessagesByChat(chat_id));
        return;
    }

    QString endpoint = "/chats/" + chatId + "/messages?limit=50";
    if (beforeId > 0) {
        endpoint += "&before_id=" + QString::number(beforeId);
    }
    QNetworkReply *reply = m_connection->get(endpoint);

    connect(
        reply, &QNetworkReply::finished,
        [this, chat_id, reply, beforeId, oldest_message]() {
            reply->deleteLater();
            if (reply->error() == QNetworkReply::NoError) {
                QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
#ifdef QT_DEBAG
                qDebug() << "[ChatManager] fetchChatHistory RAW JSON: "
                         << doc.toJson(QJsonDocument::Compact);
#endif
                QJsonArray raw = doc.object()["messages"].toArray();

                int currentUserId = m_stateManager->getUserId();
                QJsonArray messages;

                for (int i = 0; i < raw.size(); i++) {
                    QJsonObject msg = raw[i].toObject();
                    cacheMessageMedia(msg);

                    QJsonValue senderValue = msg["sender_id"];
                    QString senderIdStr =
                        senderValue.isString()
                            ? senderValue.toString()
                            : QString::number(senderValue.toInt());
                    QString currentUserIdStr = QString::number(currentUserId);

                    msg["is_me"] = (senderIdStr == currentUserIdStr);
                    if (!oldest_message.has_value() ||
                        oldest_message.value()["id"].toInt() >
                            msg["id"].toInt()) {
                        messages.append(msg);
                        m_chatStorage->addMessage(msg);
                    }
                }

                if (beforeId > 0) {
                    qDebug() << "[ChatManager] prepended history of size "
                             << messages.size();
                    emit chatsHistoryPrepended(messages);
                } else {
                    qDebug() << "[ChatManager] loaded history \n";
                    emit chatsHistoryLoaded(
                        m_chatStorage->getMessagesByChat(chat_id)
                    );
                }
            } else {
                emit chatError("Fetch history failed: " + reply->errorString());
            }
        }
    );
}

void ChatManager::sendMessage(const QString &chatId, const QString &text) {
    QJsonObject json;
    json["text"] = text;
    json["type"] = "text";
    int64_t chat_id = chatId.toLongLong();

    QNetworkReply *reply = m_connection->post(
        "/chats/" + chatId + "/messages", QJsonDocument(json).toJson()
    );

    connect(reply, &QNetworkReply::finished, [this, chat_id, reply, chatId]() {
        reply->deleteLater();
        if (reply->error() == QNetworkReply::NoError) {
            QJsonObject obj =
                QJsonDocument::fromJson(reply->readAll()).object();
            QJsonObject msg = obj["message"].toObject();
            msg["is_me"] = true;
            m_chatStorage->addMessage(msg);
            emit messageSentSuccess(msg);
        } else {
            emit chatError("Send message failed: " + reply->errorString());
        }
    });
}

void ChatManager::openDirectChat(
    int targetUserId,
    const QString &targetUserName
) {
    QJsonObject json;
    json["target_user_id"] = targetUserId;

    QNetworkReply *reply =
        m_connection->post("/chats/direct", QJsonDocument(json).toJson());

    connect(reply, &QNetworkReply::finished, [this, reply, targetUserName]() {
        reply->deleteLater();
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray responseData = reply->readAll();
#ifdef QT_DEBAG
            qDebug() << "[ChatManager] openDirectChat RAW JSON: "
                     << responseData;
#endif
            QJsonObject chat = QJsonDocument::fromJson(responseData)
                                   .object()["chat"]
                                   .toObject();
            QString chatId = QString::number(chat["id"].toInt());
            QString chatTitle = chat["title"].toString();
            if (chatTitle.isEmpty()) {
                chatTitle = chat["name"].toString();
                if (chatTitle.isEmpty() && !targetUserName.isEmpty()) {
                    chatTitle = targetUserName;
                }
            }
            emit directChatOpened(chatId, chatTitle);
            fetchChatHistory(chatId);
        } else {
            emit chatError("Open direct chat failed: " + reply->errorString());
        }
    });
}

void ChatManager::cacheMessageMedia(QJsonObject &message) {
    QJsonArray attachments = message["attachments"].toArray();
    for (int i = 0; i < attachments.size(); i++) {
        QJsonObject attachment = attachments.at(i).toObject();
        QString cachedFileLocation = m_mediaCache->getOrPut(
            attachment["s3_object_key"].toString(),
            attachment["download_url"].toString()
        );
        QString localFilePath = QUrl(cachedFileLocation).toLocalFile();
        QImageReader reader(localFilePath);
        attachment.insert("download_url", cachedFileLocation);
        attachment.insert("img_width", reader.size().width());
        attachment.insert("img_height", reader.size().height());
        attachments.replace(i, attachment);
    }
    message["attachments"] = attachments;
}

void ChatManager::onWebSocketConnected() {
    qDebug() << "[ChatManager] WebSocket connected!";
    emit webSocketConnected();
}

void ChatManager::onWebSocketDisconnected() {
    qDebug() << "[ChatManager] WebSocket disconnected";
    emit webSocketDisconnected();
}

void ChatManager::onWebSocketTextMessageReceived(const QString &message) {
    qDebug() << "[ChatManager] WS message:" << message;
    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());

    // TODO : change event_type checking for new enum

    if (doc["event_type"] == "NEW_MESSAGE") {
        QJsonObject msg = doc["data"]["message"].toObject();

        // secret
        if (msg["type"].toString() == "secret_payload" ||
            msg["chat_id"].isString()) {
            handleIncomingSecretPayload(msg);
            return;
        }

        QJsonValue senderValue = msg["sender_id"];
        QString senderIdStr =
            senderValue.isString()
                ? senderValue.toString()
                : QString::number(senderValue.toVariant().toLongLong());
        QString currentUserIdStr = QString::number(m_stateManager->getUserId());

        msg.insert("is_me", (senderIdStr == currentUserIdStr));
        m_chatStorage->addMessage(msg);
    }
    emit incomingWebSocketMessage(doc.object());
}

void ChatManager::clearCache() {
    qDebug() << "[ChatManager] Clearing chat cache (logout)";
    m_chatStorage->clear();
    if (m_webSocket->state() != QAbstractSocket::UnconnectedState) {
        m_webSocket->close();
    }
}

void ChatManager::onWebSocketError(QAbstractSocket::SocketError error) {
    qDebug() << "[ChatManager] WS error:" << error;
    emit chatError("WebSocket error: " + QString::number(error));
}

// group chats methods

void ChatManager::createGroupChat(
    const QString &name,
    const QString &description,
    const QVariantList &memberIds
) {
    if (name.trimmed().isEmpty()) {
        emit chatError("Название группы не может быть пустым");
        return;
    }

    QJsonObject json;
    json["chat_name"] = name.trimmed();
    json["description"] = description;

    QJsonArray membersArray;
    for (const QVariant &id : memberIds) {
        membersArray.append(id.toLongLong());
    }
    json["members"] = membersArray;

    QNetworkReply *reply =
        m_connection->post("/chats/group", QJsonDocument(json).toJson());

    connect(reply, &QNetworkReply::finished, [this, reply]() {
        reply->deleteLater();
        if (reply->error() == QNetworkReply::NoError) {
            QJsonObject obj =
                QJsonDocument::fromJson(reply->readAll()).object();
            QJsonObject chat = obj["chat"].toObject();
            emit groupChatCreated(chat);
            fetchChats();
        } else {
            emit chatError("Ошибка создания группы: " + reply->errorString());
        }
    });
}

void ChatManager::fetchChatMembers(const QString &chatId) {
    QNetworkReply *reply = m_connection->get("/chats/" + chatId + "/members");

    connect(reply, &QNetworkReply::finished, [this, reply]() {
        reply->deleteLater();
        if (reply->error() == QNetworkReply::NoError) {
            QJsonObject obj =
                QJsonDocument::fromJson(reply->readAll()).object();
            QJsonArray members = obj["members"].toArray();
            emit chatMembersLoaded(members);
        } else {
            emit chatError(
                "Ошибка загрузки участников: " + reply->errorString()
            );
        }
    });
}

void ChatManager::addChatMember(
    const QString &chatId,
    qint64 userId,
    const QString &role
) {
    QJsonObject json;
    json["user_id"] = userId;
    json["role"] = role;

    QNetworkReply *reply = m_connection->post(
        "/chats/" + chatId + "/members", QJsonDocument(json).toJson()
    );

    connect(reply, &QNetworkReply::finished, [this, reply, chatId]() {
        reply->deleteLater();
        if (reply->error() == QNetworkReply::NoError) {
            QJsonObject obj =
                QJsonDocument::fromJson(reply->readAll()).object();
            QJsonObject member = obj["chat_member"].toObject();
            emit chatMemberAdded(member);
            fetchChatMembers(chatId);
        } else {
            emit chatError(
                "Ошибка добавления участника: " + reply->errorString()
            );
        }
    });
}

void ChatManager::removeChatMember(
    const QString &chatId,
    qint64 userId,
    bool fetchAfter
) {
    QNetworkReply *reply = m_connection->networkManager()->sendCustomRequest(
        m_connection->createAuthRequest(
            "/chats/" + chatId + "/members/" + QString::number(userId)
        ),
        "DELETE"
    );

    connect(
        reply, &QNetworkReply::finished,
        [this, reply, chatId, userId, fetchAfter]() {
            reply->deleteLater();
            if (reply->error() == QNetworkReply::NoError) {
                emit actionSuccess("Участник удалён/Вы вышли из чата");

                if (fetchAfter) {
                    fetchChatMembers(chatId);
                }
                fetchChats();
            } else {
                emit chatError(
                    "Ошибка удаления участника: " + reply->errorString()
                );
            }
        }
    );
}

void ChatManager::updateChatInfo(
    const QString &chatId,
    const QString &newName,
    const QString &newDescription
) {
    if (newName.trimmed().isEmpty()) {
        emit chatError("Название группы не может быть пустым");
        return;
    }

    QJsonObject json;
    json["name"] = newName.trimmed();
    json["description"] = newDescription.trimmed();

    QNetworkReply *reply = m_connection->networkManager()->sendCustomRequest(
        m_connection->createAuthRequest("/chats/" + chatId), "PATCH",
        QJsonDocument(json).toJson()
    );

    connect(reply, &QNetworkReply::finished, [this, reply, chatId]() {
        reply->deleteLater();
        if (reply->error() == QNetworkReply::NoError) {
            emit actionSuccess("Настройки чата изменены");
            fetchChats();
        } else {
            emit chatError(
                "Ошибка изменения настроек чата: " + reply->errorString()
            );
        }
    });
}

void ChatManager::changeMemberRole(
    const QString &chatId,
    qint64 userId,
    const QString &newRole
) {
    QJsonObject json;
    json["role"] = newRole;

    QNetworkReply *reply = m_connection->networkManager()->sendCustomRequest(
        m_connection->createAuthRequest(
            "/chats/" + chatId + "/members/" + QString::number(userId)
        ),
        "PATCH", QJsonDocument(json).toJson()
    );

    connect(reply, &QNetworkReply::finished, [this, reply, chatId]() {
        reply->deleteLater();
        if (reply->error() == QNetworkReply::NoError) {
            emit actionSuccess("Роль участника изменена");
            fetchChatMembers(chatId);
        } else {
            qDebug() << "Change role error:" << reply->readAll();
            emit chatError("Ошибка изменения роли: " + reply->errorString());
        }
    });
}

void ChatManager::fetchChatInfo(const QString &chatId) {
    QNetworkReply *reply = m_connection->get("/chats/" + chatId);

    connect(reply, &QNetworkReply::finished, [this, reply]() {
        reply->deleteLater();
        if (reply->error() == QNetworkReply::NoError) {
            QJsonObject obj =
                QJsonDocument::fromJson(reply->readAll()).object();
            QJsonObject chat = obj["chat"].toObject();
            emit chatInfoLoaded(chat);
        } else {
            qDebug() << "Fetch chat info error:" << reply->errorString();
        }
    });
}

void ChatManager::createSecretChat(
    qint64 targetUserId,
    const QString &targetUserName
) {
    qDebug() << "[SecretChat] Создание секретного чата с юзером:"
             << targetUserId;
    // TODO LOCAL DB

    QString newSecretChatId = "sec_test_" + QString::number(targetUserId);
    emit directChatOpened(newSecretChatId, targetUserName);
    fetchChats();
}

void ChatManager::fetchSecretChatHistory(const QString &chatId, int beforeId) {
    qDebug() << "[SecretChat] Запрос истории для:" << chatId
             << "до:" << beforeId;
    // TODO LOCAL DB

    QJsonArray emptyHistory;

    if (beforeId > 0) {
        emit chatsHistoryPrepended(emptyHistory);
    } else {
        emit chatsHistoryLoaded(emptyHistory);
    }
}

void ChatManager::sendSecretMessage(
    const QString &chatId,
    const QString &text
) {
    qDebug() << "[SecretChat] Отправка сообщения " << chatId << ":" << text;
    // TODO LOCAL DB
}

void ChatManager::handleIncomingSecretPayload(const QJsonObject &payload) {
    // TODO LOCAL DB
}

void ChatManager::uploadSecretFile(
    const QString &chatId,
    const QString &filePath,
    bool asFile,
    const QString &caption,
    const QString &msgType
) {
    qDebug() << "[SecretChat] Отправка файла в" << chatId << ":" << filePath;

    // TODO LOCAL DB
}