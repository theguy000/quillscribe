#pragma once

#include "BaseModel.h"
#include <QDateTime>
#include <QFileInfo>

/**
 * @brief Recording model class
 * 
 * Represents a voice recording session with metadata and audio data.
 * Includes validation for audio file properties and recording state management.
 */
class Recording : public BaseModel {
public:
    // Constructors
    Recording();
    Recording(const QString& sessionId, const QString& filePath);
    explicit Recording(const QJsonObject& json);
    
    // Copy constructor and assignment operator
    Recording(const Recording& other);
    Recording& operator=(const Recording& other);
    
    // BaseModel interface implementation
    QString getId() const override;
    QJsonObject toJson() const override;
    bool fromJson(const QJsonObject& json) override;
    bool isValid() const override;
    
    // Getters
    QString getSessionId() const { return m_sessionId; }
    QDateTime getTimestamp() const { return m_timestamp; }
    qint64 getDuration() const { return m_duration; }
    QString getFilePath() const { return m_filePath; }
    qint64 getFileSize() const { return m_fileSize; }
    int getSampleRate() const { return m_sampleRate; }
    QString getLanguage() const { return m_language; }
    QString getDeviceName() const { return m_deviceName; }
    RecordingStatus getStatus() const { return m_status; }
    
    // Setters
    void setSessionId(const QString& sessionId);
    void setTimestamp(const QDateTime& timestamp);
    void setDuration(qint64 duration);
    void setFilePath(const QString& filePath);
    void setFileSize(qint64 fileSize);
    void setSampleRate(int sampleRate);
    void setLanguage(const QString& language);
    void setDeviceName(const QString& deviceName);
    void setStatus(RecordingStatus status);
    
    // Utility methods
    bool fileExists() const;
    qint64 calculateActualFileSize() const;
    bool validateAudioFile() const;
    QString getDisplayName() const;
    double getDurationInSeconds() const;
    QString getFormattedDuration() const;
    
    // State management
    bool canStartRecording() const;
    bool canStopRecording() const;
    bool canDelete() const;
    bool canTranscribe() const;
    
    // Equality operators
    bool operator==(const Recording& other) const;
    bool operator!=(const Recording& other) const;

private:
    QString m_id;
    QString m_sessionId;
    QDateTime m_timestamp;
    qint64 m_duration;          // Duration in milliseconds
    QString m_filePath;
    qint64 m_fileSize;          // File size in bytes
    int m_sampleRate;           // Sample rate in Hz
    QString m_language;         // Language code (e.g., "en-US")
    QString m_deviceName;       // Audio input device name
    RecordingStatus m_status;
    
    // Validation helpers
    bool validateDuration() const;
    bool validateSampleRate() const;
    bool validateFileSize() const;
};
