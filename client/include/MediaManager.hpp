#pragma once
#include <QObject>
#include <QString>
#include "ConnectionManager.hpp"
#include "StateManager.hpp"

class MediaManager : public QObject {
    Q_OBJECT

public:
    explicit MediaManager(
        ConnectionManager *connection,
        StateManager *state,
        QObject *parent = nullptr
    );

    Q_INVOKABLE void uploadFile(
        const QString &chatId,
        const QString &localFilePath,
        bool uploadAsFile,
        const QString &caption,
        const QString &messageType
    );

    Q_INVOKABLE void openFileDialog(const QString &type);
    Q_INVOKABLE void downloadFile(const QString &url, const QString &fileName);

signals:
    void fileDialogOpened();
    void fileDialogClosed();
    void fileSelected(
        const QString &filePath,
        const QString &fileType,
        qint64 fileSize,
        const QString &fileName
    );
    void uploadProgress(int percent);
    void uploadFinished();
    void uploadFailed(const QString &errorMessage);

private:
    ConnectionManager *m_connection;
    StateManager *m_state;
};
