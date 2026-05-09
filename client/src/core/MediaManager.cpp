#include "MediaManager.hpp"
#include <QApplication>
#include <QFileDialog>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaObject>
#include <QUrl>
#include <QWidget>
#include <cerrno>
#include "ConnectionManager.hpp"
#include "StateManager.hpp"

MediaManager::MediaManager(
    ConnectionManager *connection,
    StateManager *state,
    QObject *parent
)
    : QObject(parent), m_connection(connection), m_state(state) {
}

void MediaManager::uploadFile(
    const QString &chatId,
    const QString &localFilePath,
    bool uploadAsFile,
    const QString &caption,
    const QString &messageType
) {
    QString localPath = QUrl(localFilePath).isLocalFile()
                            ? QUrl(localFilePath).toLocalFile()
                            : localFilePath;
    QFileInfo info(localPath);
    QString fileName = info.fileName();
    qint64 fileSize = info.size();

    QFile file(localPath);
    if (!file.open(QIODevice::ReadOnly)) {
        emit uploadFailed("Не удалось открыть файл: " + localPath);
        return;
    }
    QByteArray fileData = file.readAll();
    file.close();

    emit uploadProgress(5);

    QJsonObject fileJson;
    fileJson["original_filename"] = fileName;
    fileJson["file_size_bytes"] = fileSize;

    QJsonArray filesArray;
    filesArray.append(fileJson);

    QJsonObject preReqJson;
    preReqJson["chat_id"] = chatId.toLongLong();
    preReqJson["message_type"] = messageType;
    preReqJson["files"] = filesArray;

    // POST
    QNetworkReply *preReply = m_connection->post(
        "/chats/attachments/presigned-links", QJsonDocument(preReqJson).toJson()
    );

    connect(
        preReply, &QNetworkReply::finished, this,
        [this, preReply, chatId, caption, messageType, fileData]() {
            preReply->deleteLater();
            if (preReply->error() != QNetworkReply::NoError) {
                emit uploadFailed(
                    "Ошибка получения presigned-link: " +
                    QString::fromUtf8(preReply->readAll())
                );
                return;
            }

            QJsonObject preResp =
                QJsonDocument::fromJson(preReply->readAll()).object();
            QJsonArray attachmentsArray = preResp["attachments"].toArray();
            if (attachmentsArray.isEmpty()) {
                emit uploadFailed(
                    "Сервер не вернул данные для загрузки вложения"
                );
                return;
            }

            QJsonObject attachInfo = attachmentsArray[0].toObject();
            QString uploadUrl = attachInfo["upload_url"].toString();
            QString contentType = attachInfo["content_type"].toString();
            QString token = attachInfo["token"].toString();

            emit uploadProgress(15);

            QNetworkRequest s3Request((QUrl(uploadUrl)));
            s3Request.setHeader(
                QNetworkRequest::ContentTypeHeader, contentType
            );

            QNetworkReply *putReply =
                m_connection->networkManager()->put(s3Request, fileData);
            connect(
                putReply, &QNetworkReply::uploadProgress, this,
                [this](qint64 sent, qint64 total) {
                    if (total > 0) {
                        emit uploadProgress(
                            15 + static_cast<int>(70.0 * sent / total)
                        );
                    }
                }
            );

            connect(
                putReply, &QNetworkReply::finished, this,
                [this, putReply, chatId, caption, messageType, token]() {
                    putReply->deleteLater();
                    if (putReply->error() != QNetworkReply::NoError) {
                        emit uploadFailed(
                            "Ошибка загрузки файла в s3: " +
                            putReply->errorString()
                        );
                        return;
                    }

                    emit uploadProgress(90);

                    QJsonObject msgJson;
                    msgJson["text"] = caption.trimmed();
                    msgJson["type"] = messageType;

                    QJsonArray attachmentTokens;
                    attachmentTokens.append(token);
                    msgJson["attachment_tokens"] = attachmentTokens;

                    QNetworkReply *msgReply = m_connection->post(
                        "/chats/" + chatId + "/messages",
                        QJsonDocument(msgJson).toJson()
                    );

                    connect(
                        msgReply, &QNetworkReply::finished, this,
                        [this, msgReply]() {
                            msgReply->deleteLater();
                            if (msgReply->error() != QNetworkReply::NoError) {
                                emit uploadFailed(
                                    "Ошибка отправки сообщения: " +
                                    QString::fromUtf8(msgReply->readAll())
                                );
                            } else {
                                emit uploadProgress(100);
                                emit uploadFinished();
                            }
                        }
                    );
                }
            );
        }
    );
};

void MediaManager::openFileDialog(const QString &type) {
    QMetaObject::invokeMethod(
        this,
        [this, type]() {
            QString filter = (type == "image")
                                 ? "Images and Videos (*.png *.jpg *.jpeg "
                                   "*.gif *.mp4 *.mov *.avi)"
                                 : "All Files (*.*)";

            emit fileDialogOpened();
            QString filePath = QFileDialog::getOpenFileName(
                nullptr,
                type == "image" ? "Выберите фото или видео"
                                : "Выберите документ",
                QString(), filter, nullptr, QFileDialog::DontUseNativeDialog
            );
            emit fileDialogClosed();

            if (!filePath.isEmpty()) {
                QFileInfo info(filePath);
                QString fileUrl = QUrl::fromLocalFile(filePath).toString();
                emit fileSelected(fileUrl, type, info.size(), info.fileName());
            }
        },
        Qt::QueuedConnection
    );
}

void MediaManager::downloadFile(const QString &url, const QString &fileName) {
    QMetaObject::invokeMethod(
        this,
        [this, url, fileName]() {
            emit fileDialogOpened();
            QString savePath = QFileDialog::getSaveFileName(
                nullptr, "Сохранить файл", fileName, "All Files (*.*)", nullptr,
                QFileDialog::DontUseNativeDialog
            );
            emit fileDialogClosed();

            if (savePath.isEmpty()) {
                return;
            }

            QNetworkRequest request((QUrl(url)));
            QNetworkReply *reply = m_connection->networkManager()->get(request);

            connect(reply, &QNetworkReply::finished, this, [reply, savePath]() {
                reply->deleteLater();
                if (reply->error() == QNetworkReply::NoError) {
                    QFile file(savePath);
                    if (file.open(QIODevice::WriteOnly)) {
                        file.write(reply->readAll());
                        file.close();
                    }
                }
            });
        },
        Qt::QueuedConnection
    );
}
