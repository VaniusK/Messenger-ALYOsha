#include "VoiceManager.hpp"
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QMediaFormat>
#include <QStandardPaths>

VoiceManager::VoiceManager(QObject *parent)
    : QObject(parent), m_isRecording(false), m_tenthsRecorded(0) {
    m_audioInput = new QAudioInput(this);
    m_audioInput->setVolume(1.0);

    m_recorder = new QMediaRecorder(this);

    QMediaFormat format;
    format.setFileFormat(QMediaFormat::Wave);
    format.setAudioCodec(QMediaFormat::AudioCodec::Wave);
    m_recorder->setMediaFormat(format);

    connect(
        m_recorder, &QMediaRecorder::errorOccurred, this,
        [this](QMediaRecorder::Error error, const QString &errorString) {
            qDebug() << "[VoiceManager] QMediaRecorder Error:" << errorString;
        }
    );

    m_session.setAudioInput(m_audioInput);
    m_session.setRecorder(m_recorder);

    QString tempDir =
        QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    m_tempFilePath = QDir(tempDir).filePath("voice_msg_temp.wav");

    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &VoiceManager::updateTimer);
}

VoiceManager::~VoiceManager() {
    if (m_isRecording) {
        m_recorder->stop();
    }
}

bool VoiceManager::isRecording() const {
    return m_isRecording;
}

QString VoiceManager::recordingDuration() const {
    int totalSeconds = m_tenthsRecorded / 10;
    int minutes = totalSeconds / 60;
    int seconds = totalSeconds % 60;
    int tenths = m_tenthsRecorded % 10;
    return QString("%1:%2,%3")
        .arg(minutes)
        .arg(seconds, 2, 10, QChar('0'))
        .arg(tenths);
}

void VoiceManager::startRecording() {
    if (m_isRecording) {
        return;
    }

    QString timestamp = QString::number(QDateTime::currentMSecsSinceEpoch());
    QString tempDir =
        QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    m_tempFilePath = QDir(tempDir).filePath("voice_" + timestamp + ".wav");

    m_recorder->setOutputLocation(QUrl::fromLocalFile(m_tempFilePath));
    m_recorder->record();
    m_isRecording = true;
    m_tenthsRecorded = 0;
    emit recordingStateChanged();
    emit recordingTimeUpdated();
    m_timer->start(100);
}

void VoiceManager::stopRecordingAndSend() {
    if (!m_isRecording) {
        return;
    }

    m_recorder->stop();
    m_timer->stop();
    m_isRecording = false;
    emit recordingStateChanged();

    QString url = QUrl::fromLocalFile(m_tempFilePath).toString();
    emit voiceMessageReady(url);
}

void VoiceManager::cancelRecording() {
    if (!m_isRecording) {
        return;
    }

    m_recorder->stop();
    m_timer->stop();
    m_isRecording = false;
    emit recordingStateChanged();
    QFile::remove(m_tempFilePath);
}

void VoiceManager::updateTimer() {
    m_tenthsRecorded++;
    emit recordingTimeUpdated();
}
