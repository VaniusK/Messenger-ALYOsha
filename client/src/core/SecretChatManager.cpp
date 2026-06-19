#include "SecretChatManager.hpp"
#include <QDir>
#include <QFutureWatcher>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMimeDatabase>
#include <QSharedPointer>
#include <QUrl>
#include <QUuid>
#include <QtConcurrent>
#include <functional>
#include "CryptoManager.hpp"
#include "SecretDatabaseManager.hpp"
#include "StateManager.hpp"
#include "enums/WebsocketsMessagesTypes.h"

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

    if (m_stateManager->getUserId() <= 0) {
        qDebug(
        ) << "[SecretChatManager] Session init delayed: User ID not set yet.";
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
        qDebug() << "[SecretChatManager] Attachments directory does not exist, "
                    "nothing to clear:"
                 << attachmentsPath;
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
        m_dbManager->clearAllLocalPaths();
        emit secretChatsUpdated();
    } else {
        qCritical() << "[SecretChatManager] Failed to remove cache directory!";
        emit secretChatError("Ошибка очистки кэша файлов");
    }
}

QString SecretChatManager::getSecretChatsPreviews() const {
    qDebug() << "[SecretChatManager] Fetching secret chats previews for user:"
             << m_stateManager->getUserId();
    QString result = m_dbManager->getChatsJson(m_stateManager->getUserId());
    qDebug() << "[SecretChatManager] Successfully retrieved chats previews.";
    return result;
}

QString SecretChatManager::fetchSecretChatHistory(
    const QString &chat_id,
    qint64 before_timestamp
) {
    qDebug() << "[SecretChatManager] Fetching chat history for chat_id:"
             << chat_id << "before timestamp:" << before_timestamp;
    QString result =
        m_dbManager->getMessagesJson(chat_id, 50, before_timestamp);

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(result.toUtf8(), &parseError);

    if (parseError.error == QJsonParseError::NoError && doc.isArray()) {
        QJsonArray messages = doc.array();
        QJsonArray updatedMessages;

        for (const QJsonValue &msgVal : messages) {
            QJsonObject msg = msgVal.toObject();
            QJsonArray attachments = msg["attachments"].toArray();
            QJsonArray updatedAttachments;

            for (const QJsonValue &attVal : attachments) {
                QJsonObject att = attVal.toObject();
                QString localPath = att["local_path"].toString();
                QFileInfo fi(localPath);
                if (fi.isDir()) {
                    att["local_path"] = "";
                }

                if (!localPath.isEmpty() && (!fi.exists() || fi.isDir())) {
                    att["local_path"] = "";
                }

                if (localPath.isEmpty()) {
                    QString messageId = msg["id"].toString();
                    QString fileId = att["file_id"].toString();

                    qDebug() << "[SecretChatManager] Auto-downloading missing "
                                "attachment:"
                             << fileId;
                    this->downloadSecretAttachment(messageId, fileId);
                } else {
                    att["download_url"] =
                        QUrl::fromLocalFile(localPath).toString();
                }

                updatedAttachments.append(att);
            }

            msg["attachments"] = updatedAttachments;
            updatedMessages.append(msg);
        }

        QJsonDocument newDoc(updatedMessages);
        return QString::fromUtf8(newDoc.toJson(QJsonDocument::Compact));
    }

    return result;
}

void SecretChatManager::downloadSecretAttachment(
    const QString &message_id,
    const QString &file_id
) {
    if (m_activeDownloads.contains(file_id)) {
        qDebug() << "[Download] File" << file_id
                 << "is already downloading. Skipping duplicate request.";
        return;
    }

    QString attInfoString = m_dbManager->getAttachmentInfo(file_id);
    if (attInfoString.isEmpty()) {
        qCritical() << "[Download] Attachment info not found in DB!";
        return;
    }

    QJsonParseError parseError;
    QJsonDocument attDoc =
        QJsonDocument::fromJson(attInfoString.toUtf8(), &parseError);

    if (parseError.error != QJsonParseError::NoError || !attDoc.isObject()) {
        qCritical() << "[Download] Failed to parse attachment JSON from DB:"
                    << parseError.errorString();
        return;
    }

    QJsonObject attData = attDoc.object();
    if (attData.isEmpty()) {
        qCritical() << "[Download] Attachment info not found in DB!";
        return;
    }

    QString fileHash = attData["file_hash"].toString();
    QString s3Key = attData["s3_object_key"].toString();
    QByteArray fileKey =
        QByteArray::fromBase64(attData["file_key"].toString().toLatin1());
    QString finalPath =
        m_stateManager->getSecretAttachmentsDirectory() + "/" + file_id;

    if (QFile::exists(finalPath)) {
        qDebug() << "[Download] File already exists! Reusing instantly.";
        m_dbManager->updateAttachmentLocalPath(file_id, finalPath);
        emit attachmentDownloaded(
            message_id, file_id, QUrl::fromLocalFile(finalPath).toString()
        );
        return;
    }

    m_activeDownloads.insert(file_id);
    qDebug() << "[Download] Requesting download URL for attachment:" << file_id;

    QJsonArray keysArray;
    keysArray.append(s3Key);

    QJsonObject requestBody;
    requestBody["attachment_keys"] = keysArray;

    QByteArray postData =
        QJsonDocument(requestBody).toJson(QJsonDocument::Compact);
    QString urlStr = "/chats/secret/attachments/download";
    QNetworkReply *urlReply = m_connectionManager->post(urlStr, postData);

    connect(
        urlReply, &QNetworkReply::finished, this,
        [this, urlReply, message_id, file_id, fileKey, finalPath]() {
            urlReply->deleteLater();
            if (urlReply->error() != QNetworkReply::NoError) {
                qCritical() << "[Download] Failed to get download URL:"
                            << urlReply->errorString();
                m_activeDownloads.remove(file_id);
                emit attachmentDownloadFailed(message_id, file_id);
                return;
            }

            QByteArray responseBytes = urlReply->readAll();
            QJsonDocument responseDoc = QJsonDocument::fromJson(responseBytes);

            QJsonArray jsonRespArray = responseDoc.array();

            if (jsonRespArray.isEmpty()) {
                qCritical()
                    << "[Download] Server returned empty or invalid JSON array";
                m_activeDownloads.remove(file_id);
                emit attachmentDownloadFailed(message_id, file_id);
                return;
            }

            QString downloadUrl = jsonRespArray[0].toString();

            if (downloadUrl.isEmpty()) {
                qCritical()
                    << "[Download] download_url is empty in server response!";
                m_activeDownloads.remove(file_id);
                emit attachmentDownloadFailed(message_id, file_id);
                return;
            }

            // Дальше всё без изменений: начинаем скачивание зашифрованного
            // файла с S3
            QNetworkRequest dlReq((QUrl(downloadUrl)));
            QNetworkReply *dlReply =
                m_connectionManager->networkManager()->get(dlReq);

            QString tempEncPath =
                m_stateManager->getTempDirectory() + "/" + file_id + ".enc";
            QFile *encFile = new QFile(tempEncPath);

            if (!encFile->open(QIODevice::WriteOnly)) {
                m_activeDownloads.remove(file_id);
                dlReply->abort();
                dlReply->deleteLater();
                encFile->deleteLater();
                return;
            }

            connect(dlReply, &QNetworkReply::readyRead, [dlReply, encFile]() {
                encFile->write(dlReply->readAll());
            });
            connect(
                dlReply, &QNetworkReply::finished, this,
                [this, dlReply, encFile, tempEncPath, finalPath, message_id,
                 file_id, fileKey]() {
                    dlReply->deleteLater();
                    encFile->close();
                    encFile->deleteLater();

                    if (dlReply->error() != QNetworkReply::NoError) {
                        qCritical() << "[Download] S3 download failed:"
                                    << dlReply->errorString();
                        QFile::remove(tempEncPath);
                        m_activeDownloads.remove(file_id);
                        emit attachmentDownloadFailed(message_id, file_id);
                        return;
                    }

                    qDebug() << "[Download] Encrypted file downloaded. "
                                "Starting background decryption...";

                    auto *watcher = new QFutureWatcher<bool>(this);
                    connect(
                        watcher, &QFutureWatcher<bool>::finished, this,
                        [this, watcher, tempEncPath, finalPath, message_id,
                         file_id]() {
                            bool success = watcher->result();
                            watcher->deleteLater();

                            QFile::remove(tempEncPath);

                            m_activeDownloads.remove(file_id);

                            if (success) {
                                qDebug() << "[Download] Decryption successful! "
                                            "Saved to Blob Storage:"
                                         << finalPath;

                                m_dbManager->updateAttachmentLocalPath(
                                    file_id, finalPath
                                );

                                emit attachmentDownloaded(
                                    message_id, file_id,
                                    QUrl::fromLocalFile(finalPath).toString()
                                );
                            } else {
                                qCritical()
                                    << "[Download] File decryption failed!";
                                emit attachmentDownloadFailed(
                                    message_id, file_id
                                );
                            }
                        }
                    );

                    QFuture<bool> future =
                        QtConcurrent::run([tempEncPath, finalPath, fileKey]() {
                            auto res = crypto::CryptoManager::decryptFile(
                                tempEncPath, finalPath, fileKey
                            );
                            return res.success;
                        });

                    watcher->setFuture(future);
                }
            );
        }
    );
}

QString SecretChatManager::getOneMessage(const QString &message_id) const {
    qDebug() << "[SecretChatManager] Fetching message with id:" << message_id;
    QString result = m_dbManager->getMessageJson(message_id);
    qDebug() << "[SecretChatManager] Message retrieved successfully.";
    return result;
}

void SecretChatManager::createSecretChatRequest(qint64 target_user_id) {
    qDebug() << "[SecretChatManager] Creating secret chat request for user:"
             << target_user_id;
    QString checkEndpoint = "/users/" + QString::number(target_user_id);
    QNetworkReply *checkReply = m_connectionManager->get(checkEndpoint);
    qDebug() << "[SecretChatManager] Sent GET request to check user existence:"
             << checkEndpoint;

    connect(
        checkReply, &QNetworkReply::finished, this,
        [this, checkReply, target_user_id]() {
            checkReply->deleteLater();

            if (checkReply->error() != QNetworkReply::NoError) {
                QVariant httpStatus = checkReply->attribute(
                    QNetworkRequest::HttpStatusCodeAttribute
                );

                QByteArray serverResponseBody = checkReply->readAll();

                qCritical().noquote()
                    << QString(
                           "[SecretChatManager] ERROR (user existence check): "
                           "Server returned an error.\n"
                           "  Target User ID: %1\n"
                           "  Qt Error Code: %2\n"
                           "  Qt Error Msg: %3\n"
                           "  HTTP Status: %4\n"
                           "  Server Body: %5"
                       )
                           .arg(target_user_id)
                           .arg(checkReply->error())
                           .arg(checkReply->errorString())
                           .arg(httpStatus.isValid() ? httpStatus.toInt() : 0)
                           .arg(QString::fromUtf8(serverResponseBody));
                emit secretChatError("Внутренняя ошибка сервера");
                return;
            }
            QByteArray responseData = checkReply->readAll();
            QJsonParseError parseError;
            QJsonDocument jsonDoc =
                QJsonDocument::fromJson(responseData, &parseError);

            if (parseError.error != QJsonParseError::NoError) {
                qCritical().noquote()
                    << QString(
                           "[SecretChatManager] ERROR (user existence check): "
                           "Failed to parse server JSON.\n"
                           "  Target User ID: %1\n"
                           "  Parse error: %2\n"
                           "  Raw body: %3"
                       )
                           .arg(target_user_id)
                           .arg(parseError.errorString())
                           .arg(QString::fromUtf8(responseData));

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
            qDebug() << "[SecretChatManager] Sending secret chat init request "
                        "to server.";
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

                        qCritical().noquote()
                            << QString(
                                   "[SecretChatManager] ERROR (request sending "
                                   "stage): Server returned an error.\n"
                                   "  Chat ID: %1\n"
                                   "  Qt Error Code: %2\n"
                                   "  Qt Error Msg: %3\n"
                                   "  HTTP Status: %4\n"
                                   "  Server Body: %5"
                               )
                                   .arg(chat_id)
                                   .arg(requestReply->error())
                                   .arg(requestReply->errorString())
                                   .arg(
                                       httpStatus.isValid() ? httpStatus.toInt()
                                                            : 0
                                   )
                                   .arg(QString::fromUtf8(serverResponseBody));
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
    qDebug() << "[SecretChatManager] Accepting secret chat request for chat_id:"
             << chat_id << "from user:" << target_user_id;
    QString checkEndpoint = "/users/" + QString::number(target_user_id);
    QNetworkReply *checkReply = m_connectionManager->get(checkEndpoint);
    qDebug() << "[SecretChatManager] Checking if target user exists:"
             << checkEndpoint;

    connect(
        checkReply, &QNetworkReply::finished, this,
        [this, checkReply, target_user_id, chat_id, other_public_key]() {
            checkReply->deleteLater();

            if (checkReply->error() != QNetworkReply::NoError) {
                QVariant httpStatus = checkReply->attribute(
                    QNetworkRequest::HttpStatusCodeAttribute
                );

                QByteArray serverResponseBody = checkReply->readAll();

                qCritical().noquote()
                    << QString(
                           "[SecretChatManager] ERROR (user existence check): "
                           "Server returned an error.\n"
                           "  Target User ID: %1\n"
                           "  Qt Error Code: %2\n"
                           "  Qt Error Msg: %3\n"
                           "  HTTP Status: %4\n"
                           "  Server Body: %5"
                       )
                           .arg(target_user_id)
                           .arg(checkReply->error())
                           .arg(checkReply->errorString())
                           .arg(httpStatus.isValid() ? httpStatus.toInt() : 0)
                           .arg(QString::fromUtf8(serverResponseBody));
                emit secretChatError("Ошибка проверки пользователя");
                return;
            }
            QByteArray responseData = checkReply->readAll();
            QJsonParseError parseError;
            QJsonDocument jsonDoc =
                QJsonDocument::fromJson(responseData, &parseError);

            if (parseError.error != QJsonParseError::NoError) {
                qCritical().noquote()
                    << QString(
                           "[SecretChatManager] ERROR (user existence check): "
                           "Failed to parse server JSON.\n"
                           "  Target User ID: %1\n"
                           "  Parse error: %2\n"
                           "  Raw body: %3"
                       )
                           .arg(target_user_id)
                           .arg(parseError.errorString())
                           .arg(QString::fromUtf8(responseData));

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
            qDebug() << "[SecretChatManager] Sending secret chat accept "
                        "request to server.";
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

                        qCritical().noquote()
                            << QString(
                                   "[SecretChatManager] ERROR (request sending "
                                   "stage): Server returned an error.\n"
                                   "  Chat ID: %1\n"
                                   "  Qt Error Code: %2\n"
                                   "  Qt Error Msg: %3\n"
                                   "  HTTP Status: %4\n"
                                   "  Server Body: %5"
                               )
                                   .arg(chat_id)
                                   .arg(requestReply->error())
                                   .arg(requestReply->errorString())
                                   .arg(
                                       httpStatus.isValid() ? httpStatus.toInt()
                                                            : 0
                                   )
                                   .arg(QString::fromUtf8(serverResponseBody));
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
    qDebug() << "[SecretChatManager] Initializing secret chat:" << chat_id
             << "- Computing shared secret...";
    auto shared_key_res = crypto::CryptoManager::computeSharedSecret(
        m_privateKey, other_public_key
    );
    if (!shared_key_res.success) {
        qCritical() << "[SecretChatManager] Failed to calculate shared secret "
                       "for chat init."
                    << crypto::CryptoManager::toString(shared_key_res.error);
        emit secretChatError("Ошибка инициализации секретного чата");
        return;
    }
    qDebug() << "[SecretChatManager] Shared secret computed successfully. "
                "Updating chat status...";
    m_dbManager->updateChatStatus(chat_id, "active", shared_key_res.secret);
    qDebug() << "[SecretChatManager] Secret chat" << chat_id
             << "is now ACTIVE and ready for messaging.";
    emit secretChatsUpdated();
}

Q_INVOKABLE QString SecretChatManager::sendSecretMessage(
    const QString &chat_id,
    const QString &text,
    const QString &messageType,
    const QVector<QString> &filepaths
) {
    qDebug() << "[SecretChatManager] Sending secret message to chat:" << chat_id
             << "- Type:" << messageType;
    QByteArray sharedSecret = m_dbManager->getSharedSecret(chat_id);
    if (sharedSecret.isEmpty()) {
        qCritical() << "[SecretChatManager] Cannot send message: shared secret "
                       "is missing for chat"
                    << chat_id;
        return QString();
    }
    QString messageId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    qint64 timestamp = QDateTime::currentMSecsSinceEpoch();
    QDateTime dt = QDateTime::fromMSecsSinceEpoch(timestamp, Qt::UTC);
    qDebug() << "[SecretChatManager] Generated message ID:" << messageId
             << "- Saving to local DB...";

    bool saved = m_dbManager->saveMessage(
        messageId, chat_id, m_stateManager->getUserId(), messageType, text,
        timestamp
    );
    if (!saved) {
        qCritical() << "[SecretChatManager] Failed to save outgoing message to "
                       "local DB";
        return QString();
    }
    qDebug() << "[SecretChatManager] Message successfully saved to local DB.";

    QJsonArray uiAttachments;
    for (const QString &path : filepaths) {
        QJsonObject attObj;
        QFileInfo fi(path);
        attObj["file_name"] = fi.fileName();
        attObj["file_size_bytes"] = fi.size();
        attObj["local_path"] = path;
        attObj["download_url"] = QUrl::fromLocalFile(path).toString();
        attObj["file_type"] = SecretChatManager::getMimeType(path);
        uiAttachments.append(attObj);
    }

    QJsonObject localMsg;
    localMsg["id"] = messageId;
    localMsg["text"] = text;
    localMsg["chat_id"] = chat_id;
    localMsg["type"] = messageType;
    localMsg["sent_at"] = dt.toString(Qt::ISODateWithMs);
    localMsg["sender_id"] = m_stateManager->getUserId();
    localMsg["status"] = "pending";
    localMsg["attachments"] = uiAttachments;

    QString uiJson =
        QString::fromUtf8(QJsonDocument(localMsg).toJson(QJsonDocument::Compact)
        );

    QJsonArray attachments_data;

    auto sendNetworkRequest = [this, messageId, chat_id, text, messageType,
                               timestamp,
                               sharedSecret](QJsonArray remoteAttachments) {
        QJsonObject inner_payload;
        inner_payload["id"] = messageId;
        inner_payload["text"] = text;
        inner_payload["chat_id"] = chat_id;
        inner_payload["type"] = messageType;
        inner_payload["sent_at"] = timestamp;
        inner_payload["attachments"] = remoteAttachments;

        QByteArray innerBytes =
            QJsonDocument(inner_payload).toJson(QJsonDocument::Compact);
        qDebug() << "[SecretChatManager] Encrypting message payload...";
        auto encryptResult =
            crypto::CryptoManager::encryptMessage(innerBytes, sharedSecret);

        if (!encryptResult.success) {
            qCritical() << "[SecretChatManager] Payload encryption failed:"
                        << crypto::CryptoManager::toString(encryptResult.error);
            m_dbManager->deleteMessage(messageId);
            return;
        }

        auto chatObjString = m_dbManager->getChat(chat_id);
        QJsonParseError parseError;

        QJsonDocument doc =
            QJsonDocument::fromJson(chatObjString.toUtf8(), &parseError);

        if (parseError.error != QJsonParseError::NoError) {
            qCritical() << "[SecretChatManager] Failed to parse chat JSON:"
                        << parseError.errorString();
            m_dbManager->deleteMessage(messageId);
            return;
        }

        if (!doc.isObject()) {
            qCritical() << "[SecretChatManager] Parsed JSON is not an object!";
            m_dbManager->deleteMessage(messageId);
            return;
        }

        QJsonObject jsonObj = doc.object();

        QJsonObject outerPayload;
        outerPayload["target_user_id"] =
            jsonObj.value("peer_id").toVariant().toLongLong();
        outerPayload["chat_id"] = chat_id;
        outerPayload["encrypted_payload"] =
            QString::fromLatin1(encryptResult.envelope);

        QByteArray bodyData =
            QJsonDocument(outerPayload).toJson(QJsonDocument::Compact);
        qDebug(
        ) << "[SecretChatManager] Sending encrypted message to server...";
        QNetworkReply *reply =
            m_connectionManager->post("/chats/secret/message", bodyData);

        connect(
            reply, &QNetworkReply::finished, this,
            [this, reply, messageId, chat_id]() {
                reply->deleteLater();

                if (reply->error() != QNetworkReply::NoError) {
                    QVariant httpStatus = reply->attribute(
                        QNetworkRequest::HttpStatusCodeAttribute
                    );
                    QByteArray serverResponseBody = reply->readAll();
                    qCritical().noquote()
                        << QString(
                               "[SecretChatManager] Failed to send message via "
                               "HTTP.\n"
                               "  Message ID: %1\n"
                               "  Chat ID: %2\n"
                               "  Error: %3\n"
                               "  HTTP Status: %4\n"
                               "  Body: %5"
                           )
                               .arg(messageId)
                               .arg(chat_id)
                               .arg(reply->errorString())
                               .arg(
                                   httpStatus.isValid() ? httpStatus.toInt() : 0
                               )
                               .arg(QString::fromUtf8(serverResponseBody));
                    this->m_dbManager->deleteMessage(messageId);
                    return;
                }

                qDebug() << "[SecretChatManager] Message" << messageId
                         << "successfully sent to server!";
            }
        );
        return;
    };
    if (!filepaths.isEmpty()) {
        qDebug() << "[SecretChatManager] Initiating file uploads for"
                 << filepaths.size() << "attachments...";
        encodeAndUploadSecretFiles(messageId, filepaths, sendNetworkRequest);
    } else {
        sendNetworkRequest(QJsonArray());
    }

    return uiJson;
}

QString SecretChatManager::getMimeType(const QString &filePath) {
    if (filePath.isEmpty() || !QFileInfo::exists(filePath)) {
        return "application/octet-stream";
    }

    QMimeDatabase db;
    QMimeType mime = db.mimeTypeForFile(filePath);

    return mime.isValid() ? mime.name() : "application/octet-stream";
}

struct UploadJob {
    QString id;
    QString original_path;
    QString cache_path;
    QString file_hash;
    QString temp_enc_path;
    QString file_name;
    qint64 file_size_bytes;
    QString file_type;
    QByteArray file_key;
    QString s3_object_key;
    QString upload_url;
};

void SecretChatManager::encodeAndUploadSecretFiles(
    const QString &message_id,
    const QVector<QString> &filepaths,
    std::function<void(QJsonArray)> on_complete
) {
    if (filepaths.isEmpty()) {
        qDebug() << "[SecretChatManager] No files to encode/upload. Proceeding "
                    "immediately.";
        on_complete(QJsonArray());
        return;
    }

    auto *watcher = new QFutureWatcher<QVector<UploadJob>>(this);

    connect(
        watcher, &QFutureWatcher<QVector<UploadJob>>::finished, this,
        [this, watcher, message_id, on_complete]() {
            QVector<UploadJob> jobs = watcher->result();
            watcher->deleteLater();
            qDebug() << "[SecretChatManager] Encoding finished. Successfully "
                        "processed"
                     << jobs.size() << "jobs.";

            if (jobs.isEmpty()) {
                qWarning(
                ) << "[SecretChatManager] All upload jobs failed during "
                     "encryption/reading. Returning empty attachments.";
                on_complete(QJsonArray());
                return;
            }

            QString urlStr = "/chats/secret/attachments/upload?count=" +
                             QString::number(jobs.size());
            qDebug() << "[SecretChatManager] Requesting S3 upload URLs for"
                     << jobs.size() << "files...";
            QNetworkReply *urlReply = m_connectionManager->get(urlStr);
            connect(
                urlReply, &QNetworkReply::finished, this,
                [this, urlReply, message_id, jobs, on_complete]() mutable {
                    urlReply->deleteLater();
                    if (urlReply->error() != QNetworkReply::NoError) {
                        qCritical().noquote()
                            << QString(
                                   "[SecretChatManager] Failed to get S3 "
                                   "URLs:\n  Error: %1"
                               )
                                   .arg(urlReply->errorString());
                        on_complete(QJsonArray());
                        return;
                    }
                    qDebug() << "[SecretChatManager] Successfully retrieved S3 "
                                "upload URLs.";
                    QJsonArray serverLinks =
                        QJsonDocument::fromJson(urlReply->readAll()).array();
                    for (int i = 0; i < jobs.size(); ++i) {
                        jobs[i].upload_url =
                            serverLinks[i].toObject()["upload_url"].toString();
                        jobs[i].s3_object_key =
                            serverLinks[i].toObject()["s3_key"].toString();
                    }

                    struct UploadState {
                        int pending = 0;
                        QJsonArray payload;
                    };
                    QSharedPointer<UploadState> state(new UploadState);
                    state->pending = jobs.size();

                    for (const auto &job : jobs) {
                        qDebug() << "[SecretChatManager] Saving attachment "
                                    "info to local db "
                                 << job.file_name;
                        bool success = m_dbManager->saveAttachment(
                            job.id, message_id, job.file_name,
                            job.file_size_bytes, job.file_type,
                            job.s3_object_key, job.cache_path, job.file_key
                        );
                        if (!success) {
                            qCritical() << "[SecretChatManager] Failed to save "
                                           "attachment info to local db";
                            continue;
                        }
                        qDebug() << "[SecretChatManager] Uploading encrypted "
                                    "file to S3:"
                                 << job.file_name
                                 << "with key:" << job.s3_object_key;
                        QFile *encFile = new QFile(job.temp_enc_path);
                        encFile->open(QIODevice::ReadOnly);

                        QNetworkRequest req(QUrl(job.upload_url));
                        req.setHeader(
                            QNetworkRequest::ContentTypeHeader,
                            "application/octet-stream"
                        );
                        QNetworkReply *uploadReply =
                            m_connectionManager->networkManager()->put(
                                req, encFile
                            );

                        connect(
                            uploadReply, &QNetworkReply::finished,
                            [this, uploadReply, encFile, job, message_id, state,
                             on_complete]() {
                                uploadReply->deleteLater();
                                encFile->close();
                                encFile->deleteLater();

                                if (uploadReply->error() ==
                                    QNetworkReply::NoError) {
                                    qDebug() << "[SecretChatManager] "
                                                "Successfully uploaded file:"
                                             << job.file_name;

                                    QJsonObject attJson;
                                    attJson["file_id"] = job.id;
                                    attJson["file_name"] = job.file_name;
                                    attJson["file_size_bytes"] =
                                        job.file_size_bytes;
                                    attJson["s3_object_key"] =
                                        job.s3_object_key;
                                    attJson["file_key"] = QString::fromLatin1(
                                        job.file_key.toBase64()
                                    );
                                    attJson["file_type"] = job.file_type;
                                    attJson["file_hash"] = job.file_hash;

                                    state->payload.append(attJson);
                                } else {
                                    qCritical().noquote()
                                        << QString(
                                               "[SecretChatManager] S3 PUT "
                                               "failed for %1:\n  Error: %2"
                                           )
                                               .arg(
                                                   job.file_name,
                                                   uploadReply->errorString()
                                               );
                                }

                                QFile::remove(job.temp_enc_path);

                                state->pending--;
                                if (state->pending == 0) {
                                    qDebug()
                                        << "[SecretChatManager] All uploads "
                                           "finished. Returning payload.";
                                    on_complete(state->payload);
                                }
                            }
                        );
                    }
                }
            );
        }
    );

    QString cacheDir = m_stateManager->getSecretAttachmentsDirectory();
    QString tempDir = m_stateManager->getTempDirectory();

    QFuture<QVector<UploadJob>> future = QtConcurrent::run([filepaths, cacheDir,
                                                            tempDir]() {
        qDebug() << "[SecretChatManager] Worker thread started for processing"
                 << filepaths.size() << "files.";
        QVector<UploadJob> jobs;

        for (const QString &path : filepaths) {
            UploadJob job;
            job.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
            job.original_path = path;

            QFileInfo fileInfo(path);
            job.file_name = fileInfo.fileName();
            job.file_size_bytes = fileInfo.size();
            job.file_type = SecretChatManager::getMimeType(path);

            job.file_key = crypto::CryptoManager::generateFileKey();

            QFile file(path);
            if (!file.open(QIODevice::ReadOnly)) {
                qCritical()
                    << "[SecretChatManager] Cannot open file for upload:"
                    << path;
                continue;
            }

            QCryptographicHash hasher(QCryptographicHash::Sha256);
            hasher.addData(&file);
            QString fileHash = hasher.result().toHex();
            file.close();

            job.file_hash = fileHash;
            job.cache_path = cacheDir + "/" + fileHash;
            if (!QFile::exists(job.cache_path)) {
                QFile::copy(path, job.cache_path);
            }

            job.temp_enc_path = tempDir + "/" + job.id + ".enc";
            qDebug() << "[SecretChatManager] Encrypting file:" << job.file_name;
            auto encryptResult = crypto::CryptoManager::encryptFile(
                job.cache_path, job.temp_enc_path, job.file_key
            );

            if (!encryptResult.success) {
                qCritical()
                    << "[SecretChatManager] Failed to encrypt file:" << path
                    << "-"
                    << crypto::CryptoManager::toString(encryptResult.error);
                continue;
            }
            qDebug() << "[SecretChatManager] Successfully encrypted file:"
                     << job.file_name;

            jobs.append(job);
        }

        return jobs;
    });
    watcher->setFuture(future);
}

Q_INVOKABLE void SecretChatManager::markChatAsRead(const QString &chat_id) {
    qDebug() << "[SecretChatManager] Marking chat as read:" << chat_id;
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

    qDebug() << "[SecretChatManager] Encrypting read receipt...";
    QJsonObject innerPayload;
    innerPayload["chat_id"] = chat_id;

    QByteArray innerBytes =
        QJsonDocument(innerPayload).toJson(QJsonDocument::Compact);
    auto encryptResult =
        crypto::CryptoManager::encryptMessage(innerBytes, sharedSecret);

    if (!encryptResult.success) {
        qCritical(
        ) << "[SecretChatManager] Failed to encrypt read receipt payload! "
          << crypto::CryptoManager::toString(encryptResult.error);
        return;
    }

    QJsonObject outerPayload;
    outerPayload["target_user_id"] = targetUserId;
    outerPayload["chat_id"] = chat_id;
    outerPayload["encrypted_payload"] =
        QString::fromLatin1(encryptResult.envelope);

    QByteArray bodyData =
        QJsonDocument(outerPayload).toJson(QJsonDocument::Compact);
    qDebug() << "[SecretChatManager] Sending read receipt to server...";

    QNetworkReply *reply =
        m_connectionManager->post("/chats/secret/message/read", bodyData);

    connect(reply, &QNetworkReply::finished, this, [this, reply, chat_id]() {
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            QVariant httpStatus =
                reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
            QByteArray serverResponseBody = reply->readAll();
            qCritical().noquote()
                << QString(
                       "[SecretChatManager] Failed to send read receipt via "
                       "HTTP.\n"
                       "  Chat ID: %1\n"
                       "  Error: %2\n"
                       "  HTTP Status: %3\n"
                       "  Body: %4"
                   )
                       .arg(chat_id)
                       .arg(reply->errorString())
                       .arg(httpStatus.isValid() ? httpStatus.toInt() : 0)
                       .arg(QString::fromUtf8(serverResponseBody));
            return;
        }

        qDebug() << "[SecretChatManager] Read receipt for chat" << chat_id
                 << "successfully sent to server!";
    });
}

Q_INVOKABLE void SecretChatManager::deleteSecretChat(const QString &chat_id) {
    qDebug() << "[SecretChatManager] Deleting secret chat:" << chat_id;
    auto chatObjString = m_dbManager->getChat(chat_id);
    if (m_dbManager->deleteChat(chat_id)) {
        qDebug(
        ) << "[SecretChatManager] Chat removed from local DB successfully.";
        emit secretChatsUpdated();
        emit secretChatDeleted(chat_id);
    } else {
        qCritical() << "[SecretChatManager] Failed to delete chat from DB!";
        emit secretChatError("Ошибка удаления чата");
    }

    QJsonParseError parseError;

    QJsonDocument doc =
        QJsonDocument::fromJson(chatObjString.toUtf8(), &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        qCritical() << "[SecretChatManager] Failed to parse chat JSON:"
                    << parseError.errorString();
        return;
    }

    if (!doc.isObject()) {
        qCritical() << "[SecretChatManager] Parsed JSON is not an object!";
        return;
    }

    QJsonObject jsonObj = doc.object();

    QJsonObject outerPayload;
    outerPayload["target_user_id"] =
        jsonObj.value("peer_id").toVariant().toLongLong();
    outerPayload["chat_id"] = chat_id;

    QByteArray bodyData =
        QJsonDocument(outerPayload).toJson(QJsonDocument::Compact);
    qDebug() << "[SecretChatManager] Sending delete chat request to server...";
    QNetworkReply *reply =
        m_connectionManager->post("/chats/secret/delete", bodyData);

    connect(reply, &QNetworkReply::finished, this, [this, reply, chat_id]() {
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            QVariant httpStatus =
                reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
            QByteArray serverResponseBody = reply->readAll();
            qCritical().noquote()
                << QString(
                       "[SecretChatManager] Failed to send info about deleting "
                       "via HTTP.\n"
                       "  Chat ID: %1\n"
                       "  Error: %2\n"
                       "  HTTP Status: %3\n"
                       "  Body: %4"
                   )
                       .arg(chat_id)
                       .arg(reply->errorString())
                       .arg(httpStatus.isValid() ? httpStatus.toInt() : 0)
                       .arg(QString::fromUtf8(serverResponseBody));
            return;
        }

        qDebug() << "[SecretChatManager] Deleting info for chat" << chat_id
                 << "successfully sent to server!";
    });
}

void SecretChatManager::handleIncomingSecretMessage(const QJsonObject &envelope
) {
    QString chatId = envelope["chat_id"].toString();
    qint64 senderId = envelope["sender_id"].toVariant().toLongLong();

    if (senderId == m_stateManager->getUserId()) {
        return;
    }

    QByteArray sharedSecret = m_dbManager->getSharedSecret(chatId);
    if (sharedSecret.isEmpty()) {
        qCritical() << "[SecretChatManager] Shared secret missing for chat"
                    << chatId;
        return;
    }

    QByteArray encryptedPayload =
        envelope["encrypted_payload"].toString().toLatin1();
    auto decryptResult =
        crypto::CryptoManager::decryptMessage(encryptedPayload, sharedSecret);
    if (!decryptResult.success) {
        qCritical() << "[SecretChatManager] Failed to decrypt incoming message!"
                    << crypto::CryptoManager::toString(decryptResult.error);
        return;
    }

    QJsonParseError parseError;

    QJsonDocument doc =
        QJsonDocument::fromJson(decryptResult.plainText, &parseError);

    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        qCritical() << "[SecretChatManager] Failed to parse decrypted payload:"
                    << parseError.errorString();
        return;
    }

    QJsonObject innerPayload = doc.object();
    QString messageId = innerPayload["id"].toString();
    QString text = innerPayload["text"].toString();
    QString messageType = innerPayload["type"].toString();
    qint64 timestamp = innerPayload["sent_at"].toVariant().toLongLong();
    QDateTime dt = QDateTime::fromMSecsSinceEpoch(timestamp, Qt::UTC);

    bool saved = m_dbManager->saveMessage(
        messageId, chatId, senderId, messageType, text, timestamp
    );
    if (!saved) {
        qCritical(
        ) << "[SecretChatManager] Failed to save incoming message to DB";
        return;
    }

    QJsonArray attachments = innerPayload["attachments"].toArray();
    QJsonArray uiAttachments;
    QString cacheDir = m_stateManager->getSecretAttachmentsDirectory();

    for (const QJsonValue &val : attachments) {
        QJsonObject attObj = val.toObject();
        QString fileId = attObj["file_id"].toString();
        QString fileName = attObj["file_name"].toString();
        qint64 fileSizeBytes =
            attObj["file_size_bytes"].toVariant().toLongLong();
        QString s3Key = attObj["s3_object_key"].toString();
        QString fileHash = attObj["file_hash"].toString();
        QString fileType = attObj["file_type"].toString();
        QByteArray fileKey =
            QByteArray::fromBase64(attObj["file_key"].toString().toLatin1());
        QString expectedCachePath = cacheDir + "/" + fileId;
        QString localPath = "";

        if (fileType.isEmpty()) {
            fileType = SecretChatManager::getMimeType(expectedCachePath);
        }

        m_dbManager->saveAttachment(
            fileId, messageId, fileName, fileSizeBytes, fileType, s3Key,
            localPath, fileKey
        );

        attObj["local_path"] = localPath;
        if (!localPath.isEmpty()) {
            attObj["download_url"] = QUrl::fromLocalFile(localPath).toString();
        }
        uiAttachments.append(attObj);
    }
    QJsonObject localMsg;
    localMsg["id"] = messageId;
    localMsg["text"] = text;
    localMsg["chat_id"] = chatId;
    localMsg["type"] = messageType;
    localMsg["sent_at"] = dt.toString(Qt::ISODateWithMs);
    localMsg["sender_id"] = senderId;
    localMsg["attachments"] = uiAttachments;

    qDebug() << "[SecretChatManager] Successfully processed incoming message"
             << messageId;
    emit secretMessageReceived(localMsg);
    emit secretChatsUpdated();
}

void SecretChatManager::handleChatRead(const QJsonObject &envelope) {
    QString chatId = envelope["chat_id"].toString();
    qint64 senderId = envelope["sender_id"].toVariant().toLongLong();

    QByteArray sharedSecret = m_dbManager->getSharedSecret(chatId);
    if (sharedSecret.isEmpty()) {
        return;
    }

    QByteArray encryptedPayload = QByteArray::fromBase64(
        envelope["encrypted_payload"].toString().toLatin1()
    );
    auto decryptResult =
        crypto::CryptoManager::decryptMessage(encryptedPayload, sharedSecret);

    if (!decryptResult.success) {
        qCritical(
        ) << "[SecretChatManager] Failed to decrypt read receipt! Ignoring."
          << crypto::CryptoManager::toString(decryptResult.error);
        return;
    }

    if (!m_dbManager->markChatAsRead(chatId, senderId)) {
        qCritical(
        ) << "[SecretChatManager] Failed to update message statuses in DB";
        return;
    }

    qDebug() << "[SecretChatManager] Chat" << chatId
             << "marked as read by peer.";
    emit secretChatsUpdated();
}

void SecretChatManager::processIncomingSecretPayload(const QJsonObject &envelope
) {
    int typeInt = envelope["message_type"].toInt();
    auto messageType = static_cast<api::v1::WebsocketMessageType>(typeInt);

    qDebug().noquote()
        << QString(
               "[SecretChatManager] ====== Processing secret payload ======\n"
               "  Message Type: %1"
           )
               .arg(typeInt);

    switch (messageType) {
        case api::v1::WebsocketMessageType::SECRET_CHAT_REQUEST: {
            qint64 senderId = envelope["sender_id"].toVariant().toLongLong();
            QString chatId = envelope["chat_id"].toString();
            QByteArray otherPubKey = QByteArray::fromBase64(
                envelope["public_key"].toString().toLatin1()
            );
            qDebug(
            ) << "[SecretChatManager] Received SECRET_CHAT_REQUEST from user:"
              << senderId << "Chat ID:" << chatId;

            this->acceptSecretChatRequest(senderId, otherPubKey, chatId);
            break;
        }

        case api::v1::WebsocketMessageType::SECRET_CHAT_ACCEPT: {
            QString chatId = envelope["chat_id"].toString();
            QByteArray otherPubKey = QByteArray::fromBase64(
                envelope["public_key"].toString().toLatin1()
            );
            qDebug(
            ) << "[SecretChatManager] Received SECRET_CHAT_ACCEPT for chat:"
              << chatId;

            this->initSecretChat(chatId, otherPubKey);
            break;
        }

        case api::v1::WebsocketMessageType::SECRET_NEW_MESSAGE: {
            qDebug() << "[SecretChatManager] Received SECRET_NEW_MESSAGE - "
                        "processing...";
            this->handleIncomingSecretMessage(envelope);
            break;
        }

        case api::v1::WebsocketMessageType::SECRET_MESSAGE_READ: {
            qDebug() << "[SecretChatManager] Received SECRET_MESSAGE_READ - "
                        "processing...";
            this->handleChatRead(envelope);
            break;
        }

        case api::v1::WebsocketMessageType::SECRET_CHAT_DELETE: {
            QString chatId = envelope["chat_id"].toString();
            qDebug(
            ) << "[SecretChatManager] Received SECRET_CHAT_DELETE for chat:"
              << chatId;
            m_dbManager->deleteChat(chatId);
            emit secretChatsUpdated();
            emit secretChatDeleted(chatId);
            emit secretChatError("Собеседник удалил секретный чат");
            break;
        }

        default:
            qWarning() << "[SecretChatManager] Ignored unexpected type in "
                          "secret dispatcher. Type:"
                       << typeInt;
            break;
    }
    qDebug() << "[SecretChatManager] ====== Secret payload processing "
                "completed ======";
}

}  // namespace client::core
