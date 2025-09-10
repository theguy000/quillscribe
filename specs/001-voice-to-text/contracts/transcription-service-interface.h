// Transcription Service Interface Contract
// Contract for speech-to-text transcription functionality

#pragma once

#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>
#include <QNetworkReply>

enum class TranscriptionProvider {
    WhisperCpp,         // Primary: whisper.cpp with local models
    WhisperCppTiny,     // Fast, lower accuracy model
    WhisperCppBase,     // Balanced speed/accuracy  
    WhisperCppSmall,    // Good accuracy, moderate speed
    WhisperCppMedium,   // High accuracy, slower
    WhisperCppLarge,    // Best accuracy, slowest
    Unknown
};

enum class TranscriptionStatus {
    Pending,
    Processing,
    Completed,
    Failed,
    Cancelled
};

enum class TranscriptionError {
    NoError,
    ModelNotFound,
    ModelLoadError,
    AudioFormatError,
    FileTooLarge,
    ProcessingError,
    InsufficientMemory,
    InvalidAudioFile,
    TimeoutError,
    UnknownError
};

struct TranscriptionResult {
    QString id;
    QString text;
    double confidence;          // 0.0 to 1.0
    QString language;
    qint64 processingTime;      // milliseconds
    QJsonArray wordTimestamps;  // Optional word-level timing
    TranscriptionProvider provider;
    QJsonObject metadata;       // Provider-specific data
};

struct TranscriptionRequest {
    QString audioFilePath;
    QString language;           // "auto" for auto-detection
    TranscriptionProvider preferredProvider;
    QJsonObject options;        // Provider-specific options
    int maxRetries = 3;
    int timeoutMs = 30000;      // 30 second timeout
};

/**
 * @brief Interface for speech-to-text transcription services
 * 
 * Contract Requirements:
 * - FR-002: Must transcribe speech to text with 95% accuracy
 * - FR-003: Must complete transcription within 2 seconds for recordings under 1 minute
 * - FR-008: Must support partial offline functionality
 * - FR-009: Must handle multiple languages (mainly English)
 * - PR-001: Transcription must complete within 2 seconds for 1-minute recordings
 */
class ITranscriptionService : public QObject {
    Q_OBJECT

public:
    explicit ITranscriptionService(QObject* parent = nullptr) : QObject(parent) {}
    virtual ~ITranscriptionService() = default;

    // Provider Management
    virtual QList<TranscriptionProvider> getAvailableProviders() const = 0;
    virtual bool setProvider(TranscriptionProvider provider) = 0;
    virtual TranscriptionProvider getCurrentProvider() const = 0;
    virtual bool isProviderAvailable(TranscriptionProvider provider) const = 0;
    virtual bool isOfflineCapable() const = 0;

    // Model Management (whisper.cpp specific)
    virtual bool downloadModel(TranscriptionProvider model) = 0;
    virtual bool isModelDownloaded(TranscriptionProvider model) const = 0;
    virtual void removeModel(TranscriptionProvider model) = 0;
    virtual qint64 getModelSize(TranscriptionProvider model) const = 0;
    virtual QString getModelPath(TranscriptionProvider model) const = 0;

    // Language Support
    virtual QStringList getSupportedLanguages() const = 0;
    virtual QString detectLanguage(const QString& audioFilePath) = 0;
    virtual void setDefaultLanguage(const QString& languageCode) = 0;

    // Transcription Operations
    virtual QString submitTranscription(const TranscriptionRequest& request) = 0;
    virtual void cancelTranscription(const QString& requestId) = 0;
    virtual TranscriptionStatus getTranscriptionStatus(const QString& requestId) const = 0;
    virtual TranscriptionResult getTranscriptionResult(const QString& requestId) const = 0;

    // Batch Operations
    virtual QStringList submitBatchTranscription(const QList<TranscriptionRequest>& requests) = 0;
    virtual QList<TranscriptionResult> getBatchResults(const QStringList& requestIds) const = 0;

    // Configuration
    virtual void setMaxConcurrentRequests(int maxRequests) = 0;
    virtual void setTimeout(int timeoutMs) = 0;
    virtual void setThreadCount(int threadCount) = 0;

    // Quality & Performance
    virtual double getProviderAccuracy(TranscriptionProvider provider) const = 0;
    virtual qint64 getAverageProcessingTime(TranscriptionProvider provider) const = 0;
    virtual int getQueueLength() const = 0;

    // Audio Format Support
    virtual QStringList getSupportedFormats() const = 0;
    virtual bool isFormatSupported(const QString& format) const = 0;
    virtual QString getRecommendedFormat() const = 0; // "wav"

    // Error Handling
    virtual TranscriptionError getLastError() const = 0;
    virtual QString getErrorString() const = 0;
    virtual void clearErrorState() = 0;

signals:
    // Progress notifications
    void transcriptionStarted(const QString& requestId, TranscriptionProvider provider);
    void transcriptionProgress(const QString& requestId, int progressPercent);
    void transcriptionCompleted(const QString& requestId, const TranscriptionResult& result);
    void transcriptionFailed(const QString& requestId, TranscriptionError error, const QString& errorMessage);
    void transcriptionCancelled(const QString& requestId);

    // Model management
    void modelDownloadStarted(TranscriptionProvider model);
    void modelDownloadProgress(TranscriptionProvider model, int progressPercent);
    void modelDownloadCompleted(TranscriptionProvider model);
    void modelDownloadFailed(TranscriptionProvider model, const QString& errorMessage);

    // Performance metrics
    void processingTimeUpdated(TranscriptionProvider provider, qint64 averageTime);
    void accuracyUpdated(TranscriptionProvider provider, double accuracy);

public slots:
    // Cache management
    virtual void clearCache() = 0;
    virtual void preloadModel(TranscriptionProvider model) = 0;
};

/**
 * @brief Factory interface for creating transcription services
 */
class ITranscriptionServiceFactory {
public:
    virtual ~ITranscriptionServiceFactory() = default;
    virtual ITranscriptionService* createService() = 0;
    virtual QList<TranscriptionProvider> getSupportedProviders() const = 0;
    virtual bool isProviderSupported(TranscriptionProvider provider) const = 0;
};

// Contract Test Requirements:
// 1. Test transcription accuracy >= 95% (FR-002)
// 2. Test processing time <= 2s for 1-minute audio (PR-001)
// 3. Test model download and management
// 4. Test offline functionality (FR-008)
// 5. Test language detection and multi-language support (FR-009)
// 6. Test audio format compatibility
// 7. Test error handling for model loading failures
// 8. Test concurrent transcription requests
// 9. Test timeout handling and retry logic
// 10. Test memory usage with large audio files
// 11. Test different whisper model sizes and performance
// 12. Test model switching during operation

