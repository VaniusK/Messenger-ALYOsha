#include "include/SecretChatManager.hpp"
#include <qglobal.h>
#include <qjsondocument.h>
#include <qjsonobject.h>
#include <qjsonvalue.h>
#include <qnetworkreply.h>
#include <qstringview.h>
#include <QDir>
#include <QUuid>
#include "CryptoManager.hpp"
#include "SecretDatabaseManager.hpp"
#include "StateManager.hpp"
#include "WebsocketsMessagesTypes.h"

namespace client::core {

SecretChatManager::SecretChatManager(
    client::db::SecretDatabaseManager *dbManager,
    StateManager *stateManager,
    ConnectionManager *connectionManager,
    QObject *parent
)
    : QObject(parent),
      m_dbManager(dbManager),
      m_stateManager(stateManager),
      m_connectionManager(connectionManager) {
}

bool SecretChatManager::initSession() {
    if (!m_dbManager || !m_stateManager) {
        return false;
    }

    QString dbPath = m_stateManager->getSecretDatabasePath();

    if (!m_dbManager->init(dbPath)) {
        qCritical() << "[SecretChatManager] Failed to init DB at" << dbPath;
        emit secretChatError("Ошибка инициализации базы данных");
        return false;
    }
    auto user_keys = m_dbManager->getIdentity();
    if (!user_keys.has_value()) {
        qDebug() << "[SecretChatManager] New device detected. Generating new "
                    "E2E keys...";

        auto [newPub, newPriv] = crypto::CryptoManager::generateKeyPair();

        if (!m_dbManager->saveIdentity(newPub, newPriv)) {
            qCritical() << "[SecretChatManager] Failed to save newly generated "
                           "keys to DB!";
            emit secretChatError("Не удалось сохранить ключи шифрования");
            return false;
        }

        m_publicKey = std::move(newPub);
        m_privateKey = std::move(newPriv);
    } else {
        m_publicKey = std::move(user_keys->publicKey);
        m_privateKey = std::move(user_keys->privateKey);
    }

    qDebug() << "[SecretChatManager] Session successfully initialized.";
    emit secretChatsUpdated();
    return true;
}

void SecretChatManager::logout() {
    m_publicKey.clear();
    m_privateKey.clear();

    if (m_dbManager) {
        m_dbManager->logout();
    }
    qDebug() << "[SecretChatManager] Logged out safely. Keys wiped from RAM.";
}

void SecretChatManager::clearSecretCache() {
    if (!m_stateManager) {
        qCritical() << "[SecretChatManager] StateManager isn't initialized!";
        return;
    }

    QString attachmentsPath = m_stateManager->getSecretAttachmentsDirectory();
    QDir attachmentsDir(attachmentsPath);

    if (!attachmentsDir.exists()) {
        return;
    }

    qint64 freedBytes = 0;
    QFileInfoList fileList =
        attachmentsDir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot);
    for (const QFileInfo &fileInfo : fileList) {
        freedBytes += fileInfo.size();
    }

    if (attachmentsDir.removeRecursively()) {
        attachmentsDir.mkpath(".");
        qDebug(
        ) << "[SecretChatManager] Cache successfully cleared. Freed bytes:"
          << freedBytes;
        emit secretChatsUpdated();
    } else {
        qCritical() << "[SecretChatManager] Failed to remove cache directory!";
        emit secretChatError("Ошибка очистки кэша файлов");
    }
}

QString SecretChatManager::getSecretChatsPreviews() const {
    return m_dbManager->getChatsJson(m_stateManager->getUserId());
}

QString SecretChatManager::fetchSecretChatHistory(
    const QString &chat_id,
    qint64 before_timestamp
) const {
    return m_dbManager->getMessagesJson(chat_id, 50, before_timestamp);
}

QString SecretChatManager::getOneMessage(const QString &message_id) const {
    return m_dbManager->getMessageJson(message_id);
}

void SecretChatManager::createSecretChatRequest(qint64 target_user_id) {
    QString checkEndpoint = "/users/" + QString::number(target_user_id);
    QNetworkReply *checkReply = m_connectionManager->get(checkEndpoint);

    connect(
        checkReply, &QNetworkReply::finished, this,
        [this, checkReply, target_user_id]() {
            checkReply->deleteLater();

            if (checkReply->error() != QNetworkReply::NoError) {
                QVariant httpStatus = checkReply->attribute(
                    QNetworkRequest::HttpStatusCodeAttribute
                );

                QByteArray serverResponseBody = checkReply->readAll();

                qCritical() << "[SecretChatManager] ERROR(user existing "
                               "checking): Server "
                               "returned an error."
                            << "\n  Qt Error Code:" << checkReply->error()
                            << "\n  Qt Error Msg:" << checkReply->errorString()
                            << "\n  HTTP Status:"
                            << (httpStatus.isValid() ? httpStatus.toInt() : 0)
                            << "\n  Server Body:" << serverResponseBody;
                emit secretChatError("Внутренняя ошибка сервера");
                return;
            }
            QByteArray responseData = checkReply->readAll();
            QJsonParseError parseError;
            QJsonDocument jsonDoc =
                QJsonDocument::fromJson(responseData, &parseError);

            if (parseError.error != QJsonParseError::NoError) {
                qCritical() << "[SecretChatManager] ERROR(user existing "
                               "checking): Failed to "
                               "parse server JSON."
                            << "Parse error:" << parseError.errorString()
                            << "Raw body:" << responseData;

                emit secretChatError("Некорректный ответ сервера");
                return;
            }
            QJsonObject jsonObj = jsonDoc.object();
            QString target_display_name =
                jsonObj.value("display_name").toString();

            QString chat_id =
                QUuid::createUuid().toString(QUuid::WithoutBraces);
            if (!m_dbManager->createChat(
                    chat_id, "secret", target_user_id, target_display_name,
                    QByteArray(), "pending"
                )) {
                qCritical() << "[SecretChatManager] CRITICAL ERROR: "
                               "Local DB failed "
                               "to save the chat!";
                emit secretChatError("Ошибка базы данных");
                return;
            }
            qDebug() << "[SecretChatManager] Chat" << chat_id
                     << "successfully saved to local DB with "
                        "'pending' status.";
            emit secretChatCreated(chat_id, target_display_name);
            emit secretChatsUpdated();

            QJsonObject requestBody;
            requestBody["target_user_id"] = target_user_id;
            requestBody["chat_id"] = chat_id;
            requestBody["public_key"] =
                QString::fromLatin1(m_publicKey.toBase64());
            QByteArray bodyData =
                QJsonDocument(requestBody).toJson(QJsonDocument::Compact);
            QNetworkReply *requestReply =
                m_connectionManager->post("/chats/secret/init", bodyData);

            connect(
                requestReply, &QNetworkReply::finished, this,
                [this, requestReply, chat_id]() {
                    requestReply->deleteLater();
                    if (requestReply->error() != QNetworkReply::NoError) {
                        QVariant httpStatus = requestReply->attribute(
                            QNetworkRequest::HttpStatusCodeAttribute
                        );
                        QByteArray serverResponseBody = requestReply->readAll();

                        qCritical()
                            << "[SecretChatManager] ERROR(request sending "
                               "stage): Server "
                               "returned an error."
                            << "\n  Qt Error Code:" << requestReply->error()
                            << "\n  Qt Error Msg:"
                            << requestReply->errorString() << "\n  HTTP Status:"
                            << (httpStatus.isValid() ? httpStatus.toInt() : 0)
                            << "\n  Server Body:" << serverResponseBody;
                        m_dbManager->deleteChat(chat_id);
                        emit secretChatError("Ошибка отправки приглашения");
                        return;
                    }
                    qDebug() << "[SecretChatManager] Invite successfully sent "
                                "to server. Waiting for WebSocket ACCEPT...";
                }
            );
        }
    );
}

void SecretChatManager::acceptSecretChatRequest(
    qint64 target_user_id,
    const QByteArray &other_public_key,
    const QString &chat_id
) {
    QString checkEndpoint = "/users/" + QString::number(target_user_id);
    QNetworkReply *checkReply = m_connectionManager->get(checkEndpoint);

    connect(
        checkReply, &QNetworkReply::finished, this,
        [this, checkReply, target_user_id, chat_id, other_public_key]() {
            checkReply->deleteLater();

            if (checkReply->error() != QNetworkReply::NoError) {
                QVariant httpStatus = checkReply->attribute(
                    QNetworkRequest::HttpStatusCodeAttribute
                );

                QByteArray serverResponseBody = checkReply->readAll();

                qCritical() << "[SecretChatManager] ERROR(user existing "
                               "checking): Server "
                               "returned an error."
                            << "\n  Qt Error Code:" << checkReply->error()
                            << "\n  Qt Error Msg:" << checkReply->errorString()
                            << "\n  HTTP Status:"
                            << (httpStatus.isValid() ? httpStatus.toInt() : 0)
                            << "\n  Server Body:" << serverResponseBody;
                emit secretChatError("Ошибка проверки пользователя");
                return;
            }
            QByteArray responseData = checkReply->readAll();
            QJsonParseError parseError;
            QJsonDocument jsonDoc =
                QJsonDocument::fromJson(responseData, &parseError);

            if (parseError.error != QJsonParseError::NoError) {
                qCritical() << "[SecretChatManager] ERROR(user existing "
                               "checking): Failed to "
                               "parse server JSON."
                            << "Parse error:" << parseError.errorString()
                            << "Raw body:" << responseData;

                emit secretChatError("Некорректный ответ сервера");
                return;
            }
            QJsonObject jsonObj = jsonDoc.object();
            QString target_display_name =
                jsonObj.value("display_name").toString();

            if (!m_dbManager->createChat(
                    chat_id, "secret", target_user_id, target_display_name,
                    QByteArray(), "pending"
                )) {
                qCritical() << "[SecretChatManager] CRITICAL ERROR: "
                               "Local DB failed "
                               "to save the chat!";
                emit secretChatError("Ошибка базы данных");
                return;
            }
            qDebug() << "[SecretChatManager] Chat" << chat_id
                     << "successfully saved to local DB with "
                        "'pending' status.";
            emit secretChatsUpdated();

            QJsonObject requestBody;
            requestBody["target_user_id"] = target_user_id;
            requestBody["chat_id"] = chat_id;
            requestBody["public_key"] =
                QString::fromLatin1(m_publicKey.toBase64());
            QByteArray bodyData =
                QJsonDocument(requestBody).toJson(QJsonDocument::Compact);
            QNetworkReply *requestReply =
                m_connectionManager->post("/chats/secret/accept", bodyData);

            connect(
                requestReply, &QNetworkReply::finished, this,
                [this, requestReply, chat_id, other_public_key]() {
                    requestReply->deleteLater();
                    if (requestReply->error() != QNetworkReply::NoError) {
                        QVariant httpStatus = requestReply->attribute(
                            QNetworkRequest::HttpStatusCodeAttribute
                        );
                        QByteArray serverResponseBody = requestReply->readAll();

                        qCritical()
                            << "[SecretChatManager] ERROR(request sending "
                               "stage): Server "
                               "returned an error."
                            << "\n  Qt Error Code:" << requestReply->error()
                            << "\n  Qt Error Msg:"
                            << requestReply->errorString() << "\n  HTTP Status:"
                            << (httpStatus.isValid() ? httpStatus.toInt() : 0)
                            << "\n  Server Body:" << serverResponseBody;
                        m_dbManager->deleteChat(chat_id);
                        emit secretChatsUpdated();
                        return;
                    }
                    this->initSecretChat(chat_id, other_public_key);
                }
            );
        }
    );
}

void SecretChatManager::initSecretChat(
    const QString &chat_id,
    const QByteArray &other_public_key
) {
    auto shared_key_res = crypto::CryptoManager::computeSharedSecret(
        m_privateKey, other_public_key
    );
    if (!shared_key_res.success) {
        qCritical() << "[SecretChatManager] Failed to calculate shared secret "
                       "for chat init."
                    << crypto::toString(shared_key_res.error);
        emit secretChatError("Ошибка инициализации секретного чата");
        return;
    }
    m_dbManager->updateChatStatus(chat_id, "active", shared_key_res.secret);
    emit secretChatsUpdated();
}

Q_INVOKABLE QString SecretChatManager::sendSecretMessage(
    const QString &chat_id,
    const QString &text,
    const QString &messageType
) {
    QByteArray sharedSecret = m_dbManager->getSharedSecret(chat_id);
    if (sharedSecret.isEmpty()) {
        qCritical() << "[SecretChatManager] Cannot send message: shared secret "
                       "is missing for chat"
                    << chat_id;
        return QString();
    }
    QString messageId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    qint64 timestamp = QDateTime::currentSecsSinceEpoch();

    bool saved = m_dbManager->saveMessage(
        messageId, chat_id, m_stateManager->getUserId(), messageType, text,
        timestamp
    );
    if (!saved) {
        qCritical() << "[SecretChatManager] Failed to save outgoing message to "
                       "local DB";
        return QString();
    }

    QJsonObject inner_payload;
    inner_payload["id"] = messageId;
    inner_payload["text"] = text;
    inner_payload["chat_id"] = chat_id;
    inner_payload["message_type"] = messageType;
    inner_payload["sent_at"] = timestamp;

    QByteArray innerBytes =
        QJsonDocument(inner_payload).toJson(QJsonDocument::Compact);
    auto encryptResult =
        crypto::CryptoManager::encryptMessage(innerBytes, sharedSecret);

    if (!encryptResult.success) {
        qCritical() << "[SecretChatManager] Payload encryption failed!";
        m_dbManager->deleteMessage(messageId);
        return QString();
    }

    auto chatObjString = m_dbManager->getChat(chat_id);
    QJsonParseError parseError;

    QJsonDocument doc =
        QJsonDocument::fromJson(chatObjString.toUtf8(), &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        qCritical() << "[SecretChatManager] Failed to parse chat JSON:"
                    << parseError.errorString();
        m_dbManager->deleteMessage(messageId);
        return QString();
    }

    if (!doc.isObject()) {
        qCritical() << "[SecretChatManager] Parsed JSON is not an object!";
        m_dbManager->deleteMessage(messageId);
        return QString();
    }

    QJsonObject jsonObj = doc.object();

    QJsonObject outerPayload;
    outerPayload["target_user_id"] =
        jsonObj.value("peer_id").toVariant().toLongLong();
    outerPayload["encrypted_payload"] =
        QString::fromLatin1(encryptResult.envelope.toBase64());

    QByteArray bodyData =
        QJsonDocument(outerPayload).toJson(QJsonDocument::Compact);
    QNetworkReply *reply =
        m_connectionManager->post("/chats/secret/message", bodyData);

    connect(reply, &QNetworkReply::finished, this, [this, reply, messageId]() {
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            QVariant httpStatus =
                reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
            qCritical(
            ) << "[SecretChatManager] Failed to send message via HTTP."
              << "\n Error:" << reply->errorString() << "\n HTTP Status:"
              << (httpStatus.isValid() ? httpStatus.toInt() : 0)
              << "\n Body:" << reply->readAll();
            this->m_dbManager->deleteMessage(messageId);
            return;
        }

        qDebug() << "[SecretChatManager] Message" << messageId
                 << "successfully sent to server!";
    });

    QJsonObject localMsg = inner_payload;
    localMsg["chat_id"] = chat_id;
    localMsg["sender_id"] = m_stateManager->getUserId();

    return QString::fromUtf8(
        QJsonDocument(localMsg).toJson(QJsonDocument::Compact)
    );
}

Q_INVOKABLE void SecretChatManager::markChatAsRead(const QString &chat_id) {
    if (!m_dbManager->markChatAsRead(chat_id, m_stateManager->getUserId())) {
        qCritical(
        ) << "[SecretChatManager] Failed to mark chat as read locally.";
        return;
    }
    emit secretChatsUpdated();

    auto chatObjString = m_dbManager->getChat(chat_id);
    QJsonParseError parseError;
    QJsonDocument doc =
        QJsonDocument::fromJson(chatObjString.toUtf8(), &parseError);

    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        qCritical() << "[SecretChatManager] Failed to parse chat JSON:"
                    << parseError.errorString();
        return;
    }
    QJsonObject jsonObj = doc.object();
    qint64 targetUserId = jsonObj.value("peer_id").toVariant().toLongLong();

    QByteArray sharedSecret = m_dbManager->getSharedSecret(chat_id);
    if (sharedSecret.isEmpty()) {
        qCritical() << "[SecretChatManager] Cannot send read receipt: shared "
                       "secret is missing!";
        return;
    }

    QJsonObject innerPayload;
    innerPayload["chat_id"] = chat_id;

    QByteArray innerBytes =
        QJsonDocument(innerPayload).toJson(QJsonDocument::Compact);
    auto encryptResult =
        crypto::CryptoManager::encryptMessage(innerBytes, sharedSecret);

    if (!encryptResult.success) {
        qCritical(
        ) << "[SecretChatManager] Failed to encrypt read receipt payload! "
          << crypto::toString(encryptResult.error);
        return;
    }

    QJsonObject outerPayload;
    outerPayload["target_user_id"] = targetUserId;
    outerPayload["encrypted_payload"] =
        QString::fromLatin1(encryptResult.envelope.toBase64());

    QByteArray bodyData =
        QJsonDocument(outerPayload).toJson(QJsonDocument::Compact);

    QNetworkReply *reply =
        m_connectionManager->post("/chats/secret/message/read", bodyData);

    connect(reply, &QNetworkReply::finished, this, [this, reply, chat_id]() {
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            QVariant httpStatus =
                reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
            qCritical(
            ) << "[SecretChatManager] Failed to send read receipt via HTTP."
              << "\n Error:" << reply->errorString() << "\n HTTP Status:"
              << (httpStatus.isValid() ? httpStatus.toInt() : 0)
              << "\n Body:" << reply->readAll();
            return;
        }

        qDebug() << "[SecretChatManager] Read receipt for chat" << chat_id
                 << "successfully sent to server!";
    });
}

Q_INVOKABLE void SecretChatManager::deleteSecretChat(const QString &chat_id) {
    if (m_dbManager->deleteChat(chat_id)) {
        emit secretChatsUpdated();
    } else {
        qCritical() << "[SecretChatManager] Failed to delete chat from DB!";
        emit secretChatError("Ошибка удаления чата");
    }
    auto chatObjString = m_dbManager->getChat(chat_id);
    QJsonParseError parseError;

    QJsonDocument doc =
        QJsonDocument::fromJson(chatObjString.toUtf8(), &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        qCritical() << "[SecretChatManager] Failed to parse chat JSON:"
                    << parseError.errorString();
    }

    if (!doc.isObject()) {
        qCritical() << "[SecretChatManager] Parsed JSON is not an object!";
    }

    QJsonObject jsonObj = doc.object();

    QJsonObject outerPayload;
    outerPayload["target_user_id"] =
        jsonObj.value("peer_id").toVariant().toLongLong();
    outerPayload["chat_id"] = chat_id;

    QByteArray bodyData =
        QJsonDocument(outerPayload).toJson(QJsonDocument::Compact);
    QNetworkReply *reply =
        m_connectionManager->post("/chats/secret/delete", bodyData);

    connect(reply, &QNetworkReply::finished, this, [this, reply, chat_id]() {
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            QVariant httpStatus =
                reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
            qCritical() << "[SecretChatManager] Failed to send info about "
                           "deleting via HTTP."
                        << "\n Error:" << reply->errorString()
                        << "\n HTTP Status:"
                        << (httpStatus.isValid() ? httpStatus.toInt() : 0)
                        << "\n Body:" << reply->readAll();
            this->m_dbManager->deleteMessage(chat_id);
            return;
        }

        qDebug() << "[SecretChatManager] Deleting " << chat_id
                 << "info successfully sent to server!";
    });
}

void SecretChatManager::processIncomingSecretPayload(const QJsonObject &envelope
) {
    int typeInt = envelope["message_type"].toInt();
    auto messageType = static_cast<api::v1::WebsocketMessageType>(typeInt);

    qDebug() << "[SecretChatManager] Processing secret payload type:"
             << typeInt;

    switch (messageType) {
        case api::v1::WebsocketMessageType::SECRET_CHAT_REQUEST: {
            qint64 senderId = envelope["sender_id"].toVariant().toLongLong();
            QString chatId = envelope["chat_id"].toString();
            QByteArray otherPubKey = QByteArray::fromBase64(
                envelope["public_key"].toString().toLatin1()
            );

            this->acceptSecretChatRequest(senderId, otherPubKey, chatId);
            break;
        }

        case api::v1::WebsocketMessageType::SECRET_CHAT_ACCEPT: {
            QString chatId = envelope["chat_id"].toString();
            QByteArray otherPubKey = QByteArray::fromBase64(
                envelope["public_key"].toString().toLatin1()
            );

            this->initSecretChat(chatId, otherPubKey);
            break;
        }

        case api::v1::WebsocketMessageType::SECRET_NEW_MESSAGE: {
            this->handleIncomingSecretMessage(envelope);
            break;
        }

        case api::v1::WebsocketMessageType::SECRET_MESSAGE_READ: {
            this->handleChatRead(envelope);
            break;
        }

        case api::v1::WebsocketMessageType::SECRET_CHAT_DELETE: {
            QString chatId = envelope["chat_id"].toString();
            m_dbManager->deleteChat(chatId);
            emit secretChatsUpdated();
            emit secretChatError("Собеседник удалил секретный чат");
            break;
        }

        default:
            qWarning() << "[SecretChatManager] Ignored unexpected type in "
                          "secret dispatcher.";
            break;
    }
}

}  // namespace client::core