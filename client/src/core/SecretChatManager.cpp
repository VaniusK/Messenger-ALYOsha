#include "include/SecretChatManager.hpp"
#include <qglobal.h>
#include <qjsondocument.h>
#include <qjsonobject.h>
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
        // TODO emit
        qCritical() << "[SecretChatManager] StateManager isn't initialized!";
        return;
    }

    QString attachmentsPath = m_stateManager->getSecretAttachmentsDirectory();
    QDir attachmentsDir(attachmentsPath);

    if (!attachmentsDir.exists()) {
        // TODO emit
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
        // TODO emit
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
                // TODO emit
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

                // TODO emit
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
                // TODO emit
            }
            qDebug() << "[SecretChatManager] Chat" << chat_id
                     << "successfully saved to local DB with "
                        "'pending' status.";
            // TODO emit

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
                        // TODO emit
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
                // TODO emit
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

                // TODO emit
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
                // TODO emit
            }
            qDebug() << "[SecretChatManager] Chat" << chat_id
                     << "successfully saved to local DB with "
                        "'pending' status.";
            // TODO emit

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
                        // TODO emit
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
        return;
    }
    m_dbManager->updateChatStatus(chat_id, "active", shared_key_res.secret);
}

Q_INVOKABLE QString SecretChatManager::sendSecretMessage(
    const QString &chat_id,
    const QString &text,
    const QString &type,
    const QJsonArray &attachments
) {
    return "67";
}

Q_INVOKABLE void SecretChatManager::markChatAsRead(const QString &chat_id) {
    if (!m_dbManager->markChatAsRead(chat_id, m_stateManager->getUserId())) {
        qCritical() << "[SecretChatManager] Failed to mark chat as read.";
    }
    emit secretChatsUpdated();
}

Q_INVOKABLE void SecretChatManager::deleteSecretChat(const QString &chat_id) {
    if (!m_dbManager->deleteChat(chat_id)) {
        qCritical() << "[SecretChatManager] Failed to delete chat.";
        return;
    }
    emit secretChatsUpdated();
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
            // TODO
            // this->handleIncomingSecretMessage(envelope);
            break;
        }

        case api::v1::WebsocketMessageType::SECRET_MESSAGE_READ: {
            // TODO
            // this->markLocalMessagesAsRead(envelope);
            break;
        }

        case api::v1::WebsocketMessageType::SECRET_CHAT_DELETE: {
            QString chatId = envelope["chat_id"].toString();
            m_dbManager->deleteChat(chatId);
            // TODO emit
            break;
        }

        default:
            qWarning() << "[SecretChatManager] Ignored unexpected type in "
                          "secret dispatcher.";
            break;
    }
}

}  // namespace client::core