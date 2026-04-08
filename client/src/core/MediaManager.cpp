#include "MediaManager.hpp"
#include <qobject.h>
#include <qurl.h>
#include <QApplication>
#include <QFileDialog>
#include <QFileInfo>
#include <QMetaObject>
#include <QTimer>
#include <QUrl>
#include <QWidget>
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
    const QString &localFilePath,
    const QString &fileType
) {
    QFileInfo info(localFilePath);
    QString fileName = info.fileName();
    qint64 fileSize = info.size();

    QTimer *timer = new QTimer(this);
    int *progress = new int(0);

    connect(
        timer, &QTimer::timeout, this,
        [this, timer, progress, localFilePath, fileType, fileSize, fileName]() {
            *progress += 25;
            emit uploadProgress(*progress);

            if (*progress >= 100) {
                timer->stop();
                timer->deleteLater();
                delete progress;

                QString fileUrl = QUrl::fromLocalFile(localFilePath).toString();
                emit uploadFinished(fileUrl, fileType, fileSize, fileName);
            }
        }
    );

    timer->start(200);
    emit uploadProgress(0);
}

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
