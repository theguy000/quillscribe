#pragma once

#include "BaseModel.h"
#include <QDateTime>
#include <QJsonArray>

/**
 * @brief Transcription model class
 * 
 * Represents the text output from speech-to-text conversion.
 * Includes confidence scoring, provider information, and word-level timing data.
 */
class Transcription : public BaseModel {
public:
    // Constructors
    Transcription();
    Transcription(const QString& recordingId, const QString& text);
    explicit Transcription(const QJsonObject& json);
    
    // Copy constructor and assignment operator
    Transcription(const Transcription& other);
    Transcription& operator=(const Transcription& other);
    
    // BaseModel interface implementation
    QString getId() const override;
    QJsonObject toJson() const override;
    bool fromJson(const QJsonObject& json) override;
    bool isValid() const override;
    
    // Getters
    QString getRecordingId() const { return m_recordingId; }
    QString getText() const { return m_text; }
    double getConfidence() const { return m_confidence; }
    QString getProvider() const { return m_provider; }
    QString getLanguage() const { return m_language; }
    qint64 getProcessingTime() const { return m_processingTime; }
    QJsonArray getWordTimestamps() const { return m_wordTimestamps; }
    QDateTime getCreatedAt() const { return m_createdAt; }
    TranscriptionStatus getStatus() const { return m_status; }
    
    // Setters
    void setRecordingId(const QString& recordingId);
    void setText(const QString& text);
    void setConfidence(double confidence);
    void setProvider(const QString& provider);
    void setLanguage(const QString& language);
    void setProcessingTime(qint64 processingTime);
    void setWordTimestamps(const QJsonArray& wordTimestamps);
    void setCreatedAt(const QDateTime& createdAt);
    void setStatus(TranscriptionStatus status);
    
    // Utility methods
    int getWordCount() const;
    int getCharacterCount() const;
    double getConfidencePercentage() const;
    QString getConfidenceLevel() const; // "High", "Medium", "Low"
    QStringList getSupportedProviders() const;
    bool hasWordTimestamps() const;
    QString getFormattedProcessingTime() const;
    QString getDisplayText(int maxLength = -1) const;
    
    // Text analysis
    QStringList extractWords() const;
    QStringList extractSentences() const;
    QString getFirstSentence() const;
    QString getSummaryPreview(int maxWords = 20) const;
    
    // State management
    bool canRetry() const;
    bool canEdit() const;
    bool canEnhance() const;
    bool isProcessing() const;
    bool isCompleted() const;
    bool hasFailed() const;
    
    // Word timestamp utilities
    struct WordTimestamp {
        QString word;
        double startTime;
        double endTime;
        double confidence;
        
        QJsonObject toJson() const;
        static WordTimestamp fromJson(const QJsonObject& json);
    };
    
    QList<WordTimestamp> getWordTimestampList() const;
    void setWordTimestampList(const QList<WordTimestamp>& timestamps);
    
    // Equality operators
    bool operator==(const Transcription& other) const;
    bool operator!=(const Transcription& other) const;

private:
    QString m_id;
    QString m_recordingId;          // Foreign key to Recording
    QString m_text;                 // Transcribed text content
    double m_confidence;            // Overall confidence score (0.0-1.0)
    QString m_provider;             // STT provider used
    QString m_language;             // Detected language
    qint64 m_processingTime;        // Time taken for transcription (ms)
    QJsonArray m_wordTimestamps;    // Word-level timing data
    QDateTime m_createdAt;          // When transcription was completed
    TranscriptionStatus m_status;   // Processing state
    
    // Validation helpers
    bool validateConfidence() const;
    bool validateProcessingTime() const;
    bool validateProvider() const;
    bool validateWordTimestamps() const;
};
