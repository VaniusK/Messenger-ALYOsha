#pragma once
#include <QAudioInput>
#include <QMediaCaptureSession>
#include <QMediaRecorder>
#include <QObject>
#include <QTimer>
#include <QUrl>

class VoiceManager : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool isRecording READ isRecording NOTIFY recordingStateChanged)
    Q_PROPERTY(QString recordingDuration READ recordingDuration NOTIFY
                   recordingTimeUpdated)

public:
    explicit VoiceManager(QObject *parent = nullptr);
    ~VoiceManager();

    bool isRecording() const;
    QString recordingDuration() const;

public slots:
    void startRecording();
    void stopRecordingAndSend();
    void cancelRecording();

signals:
    void recordingStateChanged();
    void recordingTimeUpdated();
    void voiceMessageReady(const QString &audioFilePath);

private slots:
    void updateTimer();

private:
    QMediaCaptureSession m_session;
    QAudioInput *m_audioInput;
    QMediaRecorder *m_recorder;
    QTimer *m_timer;

    bool m_isRecording;
    int m_tenthsRecorded;
    QString m_tempFilePath;
};
