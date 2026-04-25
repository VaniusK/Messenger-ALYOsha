#include "MediaCacheManager.hpp"
#include <qimage.h>
#include <qnetworkrequest.h>
#include <qstandardpaths.h>
#include <QApplication>
#include <QFileDialog>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaObject>
#include <QUrl>
#include <QWidget>
#include <cerrno>
#include "ConnectionManager.hpp"

MediaCacheManager::MediaCacheManager(
    ConnectionManager *connection,
    QObject *parent
)
    : QObject(parent), m_connection(connection) {
}

QString MediaCacheManager::getOrPut(
    const QString &s3_object_key,
    const QString &download_url
) {
    auto cache_location =
        QStandardPaths::writableLocation(QStandardPaths::CacheLocation);

    QDir cache_dir(QDir::cleanPath(
        QDir(cache_location).path() + QDir::separator() + "media"
    ));
    if (!cache_dir.exists()) {
        cache_dir.mkdir(".");
    }

    QString extension = QFileInfo(s3_object_key).suffix();

    if (extension.endsWith("%22")) {
        extension.chop(3);
    }

    QString file_name = QCryptographicHash::hash(
                            s3_object_key.toUtf8(), QCryptographicHash::Md5
                        )
                            .toHex() +
                        "." + extension;

    QDir file_location(QDir(cache_dir).path() + QDir::separator() + file_name);

    QFileInfo info(file_location.path());
    if (info.exists() && info.size() > 0) {
        return QUrl::fromLocalFile(file_location.path()).toString();
    }

    QFile *file = new QFile(file_location.path());

    if (m_activeDownloads.contains(
            QUrl::fromLocalFile(file->fileName()).toString()
        )) {
        return QUrl::fromLocalFile(file_location.path()).toString();
    }

    if (!file->open(QIODevice::WriteOnly)) {
        qDebug() << "Couldn't open file " << file_name;
        return "";
    }

    QNetworkRequest request(download_url);
    auto reply = m_connection->networkManager()->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, file]() {
        return MediaCacheManager::onFinished(reply, file);
    });
    m_activeDownloads.insert(
        QUrl::fromLocalFile(file_location.path()).toString()
    );
    return QUrl::fromLocalFile(file_location.path()).toString();
}

void MediaCacheManager::onFinished(QNetworkReply *reply, QFile *file) {
    if (reply->error() == QNetworkReply::NoError) {
        file->write(reply->readAll());
        qDebug() << "Saved file " << file->fileName();
        emit imageLoaded(QUrl::fromLocalFile(file->fileName()).toString());
    } else {
        qDebug() << "Error while saving file:" << reply->errorString();
    }
    m_activeDownloads.erase(
        m_activeDownloads.find(QUrl::fromLocalFile(file->fileName()).toString())
    );

    file->close();
    file->deleteLater();
    reply->deleteLater();
}
