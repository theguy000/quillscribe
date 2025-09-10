#include "Recording.h"
#include <QJsonObject>
#include <QFileInfo>
#include <QDir>

Recording::Recording() 
    : m_id(generateUuid())
    , m_sessionId("")
    , m_timestamp(QDateTime::currentDateTime())
    , m_duration(0)
    , m_filePath("")
    , m_fileSize(0)
    , m_sampleRate(16000)
    , m_language("en-US")
    , m_deviceName("")
    , m_status(RecordingStatus::Recording)
{
}

Recording::Recording(const QString& sessionId, const QString& filePath)
    : m_id(generateUuid())
    , m_sessionId(sessionId)
    , m_timestamp(QDateTime::currentDateTime())
    , m_duration(0)
    , m_filePath(filePath)
    , m_fileSize(0)
    , m_sampleRate(16000)
    , m_language("en-US")
    , m_deviceName("")
    , m_status(RecordingStatus::Recording)
{
    if (!filePath.isEmpty()) {
        m_fileSize = calculateActualFileSize();
    }
}

Recording::Recording(const QJsonObject& json)
    : Recording()
{
    fromJson(json);
}

Recording::Recording(const Recording& other)
    : m_id(other.m_id)
    , m_sessionId(other.m_sessionId)
    , m_timestamp(other.m_timestamp)
    , m_duration(other.m_duration)
    , m_filePath(other.m_filePath)
    , m_fileSize(other.m_fileSize)
    , m_sampleRate(other.m_sampleRate)
    , m_language(other.m_language)
    , m_deviceName(other.m_deviceName)
    , m_status(other.m_status)
{
}

Recording& Recording::operator=(const Recording& other) {
    if (this != &other) {
        m_id = other.m_id;
        m_sessionId = other.m_sessionId;
        m_timestamp = other.m_timestamp;
        m_duration = other.m_duration;
        m_filePath = other.m_filePath;
        m_fileSize = other.m_fileSize;
        m_sampleRate = other.m_sampleRate;
        m_language = other.m_language;
        m_deviceName = other.m_deviceName;
        m_status = other.m_status;
    }
    return *this;
}

QString Recording::getId() const {
    return m_id;
}

QJsonObject Recording::toJson() const {
    QJsonObject json;
    json["id"] = m_id;
    json["sessionId"] = m_sessionId;
    json["timestamp"] = m_timestamp.toString(Qt::ISODate);
    json["duration"] = static_cast<qint64>(m_duration);
    json["filePath"] = m_filePath;
    json["fileSize"] = static_cast<qint64>(m_fileSize);
    json["sampleRate"] = m_sampleRate;
    json["language"] = m_language;
    json["deviceName"] = m_deviceName;
    json["status"] = recordingStatusToString(m_status);
    return json;
}

bool Recording::fromJson(const QJsonObject& json) {
    if (!json.contains("id") || !isValidUuid(json["id"].toString())) {
        return false;
    }
    
    m_id = json["id"].toString();
    m_sessionId = json.value("sessionId").toString();
    
    QString timestampStr = json.value("timestamp").toString();
    m_timestamp = QDateTime::fromString(timestampStr, Qt::ISODate);
    if (!m_timestamp.isValid()) {
        m_timestamp = QDateTime::currentDateTime();
    }
    
    m_duration = json.value("duration").toVariant().toLongLong();
    m_filePath = json.value("filePath").toString();
    m_fileSize = json.value("fileSize").toVariant().toLongLong();
    m_sampleRate = json.value("sampleRate").toInt(16000);
    m_language = json.value("language").toString("en-US");
    m_deviceName = json.value("deviceName").toString();
    
    QString statusStr = json.value("status").toString("Recording");
    m_status = recordingStatusFromString(statusStr);
    
    return true;
}

bool Recording::isValid() const {
    // ID must be valid UUID
    if (!isValidUuid(m_id)) {
        return false;
    }
    
    // Session ID should be valid UUID if not empty
    if (!m_sessionId.isEmpty() && !isValidUuid(m_sessionId)) {
        return false;
    }
    
    // Timestamp must be valid
    if (!m_timestamp.isValid()) {
        return false;
    }
    
    // Duration validation depends on status
    if (!validateDuration()) {
        return false;
    }
    
    // File path validation for completed recordings
    if (m_status == RecordingStatus::Completed && m_filePath.isEmpty()) {
        return false;
    }
    
    // Sample rate validation
    if (!validateSampleRate()) {
        return false;
    }
    
    // Language code validation
    if (!isValidLanguageCode(m_language)) {
        return false;
    }
    
    return true;
}

void Recording::setSessionId(const QString& sessionId) {
    m_sessionId = sessionId;
}

void Recording::setTimestamp(const QDateTime& timestamp) {
    m_timestamp = timestamp;
}

void Recording::setDuration(qint64 duration) {
    m_duration = duration;
}

void Recording::setFilePath(const QString& filePath) {
    m_filePath = filePath;
    if (!filePath.isEmpty()) {
        m_fileSize = calculateActualFileSize();
    }
}

void Recording::setFileSize(qint64 fileSize) {
    m_fileSize = fileSize;
}

void Recording::setSampleRate(int sampleRate) {
    m_sampleRate = sampleRate;
}

void Recording::setLanguage(const QString& language) {
    m_language = language;
}

void Recording::setDeviceName(const QString& deviceName) {
    m_deviceName = deviceName;
}

void Recording::setStatus(RecordingStatus status) {
    m_status = status;
}

bool Recording::fileExists() const {
    return !m_filePath.isEmpty() && QFileInfo::exists(m_filePath);
}

qint64 Recording::calculateActualFileSize() const {
    if (m_filePath.isEmpty()) {
        return 0;
    }
    
    QFileInfo fileInfo(m_filePath);
    return fileInfo.exists() ? fileInfo.size() : 0;
}

bool Recording::validateAudioFile() const {
    if (!fileExists()) {
        return false;
    }
    
    // Check file extension (should be .wav for optimal whisper.cpp processing)
    QFileInfo fileInfo(m_filePath);
    QString extension = fileInfo.suffix().toLower();
    
    // Accept common audio formats, prefer WAV
    QStringList validExtensions = {"wav", "mp3", "flac", "m4a", "ogg"};
    if (!validExtensions.contains(extension)) {
        return false;
    }
    
    // Check minimum file size (should have some audio content)
    if (m_fileSize < 1024) { // Less than 1KB is likely not valid audio
        return false;
    }
    
    return true;
}

QString Recording::getDisplayName() const {
    if (!m_filePath.isEmpty()) {
        QFileInfo fileInfo(m_filePath);
        return fileInfo.baseName();
    }
    
    return QString("Recording %1").arg(m_timestamp.toString("yyyy-MM-dd hh:mm"));
}

double Recording::getDurationInSeconds() const {
    return m_duration / 1000.0;
}

QString Recording::getFormattedDuration() const {
    if (m_duration <= 0) {
        return "00:00";
    }
    
    qint64 seconds = m_duration / 1000;
    qint64 minutes = seconds / 60;
    seconds = seconds % 60;
    
    if (minutes < 60) {
        return QString("%1:%2").arg(minutes, 2, 10, QChar('0')).arg(seconds, 2, 10, QChar('0'));
    } else {
        qint64 hours = minutes / 60;
        minutes = minutes % 60;
        return QString("%1:%2:%3")
            .arg(hours, 2, 10, QChar('0'))
            .arg(minutes, 2, 10, QChar('0'))
            .arg(seconds, 2, 10, QChar('0'));
    }
}

bool Recording::canStartRecording() const {
    return m_status == RecordingStatus::Recording;
}

bool Recording::canStopRecording() const {
    return m_status == RecordingStatus::Recording;
}

bool Recording::canDelete() const {
    return m_status != RecordingStatus::Recording;
}

bool Recording::canTranscribe() const {
    return m_status == RecordingStatus::Completed && validateAudioFile();
}

bool Recording::operator==(const Recording& other) const {
    return m_id == other.m_id;
}

bool Recording::operator!=(const Recording& other) const {
    return !(*this == other);
}

bool Recording::validateDuration() const {
    switch (m_status) {
        case RecordingStatus::Recording:
            // Duration can be 0 while recording
            return m_duration >= 0;
        case RecordingStatus::Completed:
        case RecordingStatus::Processing:
            // Must have positive duration for completed/processing recordings
            return m_duration > 0;
        case RecordingStatus::Error:
        case RecordingStatus::Cancelled:
            // Duration can be any value for error/cancelled recordings
            return m_duration >= 0;
        default:
            return false;
    }
}

bool Recording::validateSampleRate() const {
    // Valid sample rates for audio recording (8kHz to 48kHz)
    return m_sampleRate >= 8000 && m_sampleRate <= 48000;
}

bool Recording::validateFileSize() const {
    // File size should be reasonable for audio files
    if (m_status == RecordingStatus::Completed && m_fileSize <= 0) {
        return false;
    }
    
    // Maximum reasonable file size: 1GB
    const qint64 maxFileSize = 1024 * 1024 * 1024;
    return m_fileSize <= maxFileSize;
}
