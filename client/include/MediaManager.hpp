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

    Q_INVOKABLE void
    uploadFile(const QString &localFilePath, const QString &fileType);

    Q_INVOKABLE void openFileDialog(const QString &type);

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
    void uploadFinished(
        const QString &fileUrl,
        const QString &fileType,
        qint64 fileSize,
        const QString &fileName
    );
    void uploadFailed(const QString &errorMessage);

private:
    ConnectionManager *m_connection;
    StateManager *m_state;
};
