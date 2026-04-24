#pragma once
#include <ConnectionManager.hpp>
#include <QFile>
#include <QObject>
#include <QString>

class MediaCacheManager : public QObject {
    Q_OBJECT

public:
    explicit MediaCacheManager(
        ConnectionManager *connection,
        QObject *parent = nullptr
    );

    Q_INVOKABLE QString
    getOrPut(const QString &s3_object_key, const QString &download_url);

private:
    ConnectionManager *m_connection;

signals:
    void onImageLoaded();

private slots:

    void onFinished(QNetworkReply *reply, QFile *file);
};
