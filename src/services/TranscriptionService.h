#pragma once

#include "../models/BaseModel.h"
#include "../../specs/001-voice-to-text/contracts/transcription-service-interface.h"
#include "../../specs/001-voice-to-text/contracts/storage-interface.h"
#include <QObject>
#include <QString>
#include <QThread>
#include <QThreadPool>
#include <QRunnable>
#include <QMutex>
#include <QMutexLocker>
#include <QWaitCondition>
#include <QTimer>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QElapsedTimer>
#include <QMap>
#include <QQueue>
#include <QAtomicInt>

// Forward declaration for whisper.cpp
struct whisper_context;
struct whisper_full_params;

/**
 * @brief Transcription task for threaded processing
 */
class TranscriptionTask : public QObject, public QRunnable {
    Q_OBJECT

public:
    TranscriptionTask(const TranscriptionRequest& request, const QString& requestId);
    void run() override;

signals:
    void taskCompleted(const QString& requestId, const TranscriptionResult& result);
    void taskFailed(const QString& requestId, TranscriptionError error, const QString& errorMessage);
    void taskProgress(const QString& requestId, int progressPercent);

private:
    TranscriptionRequest m_request;
    QString m_requestId;
};

/**
 * @brief TranscriptionService implementation using whisper.cpp
 * 
 * Provides speech-to-text transcription functionality using local whisper models.
 * Supports multiple model sizes and concurrent transcription requests.
 */
class TranscriptionService : public ITranscriptionService {
    Q_OBJECT

public:
    explicit TranscriptionService(QObject* parent = nullptr);
    explicit TranscriptionService(IStorageManager* storageManager, QObject* parent = nullptr);
    ~TranscriptionService() override;

    // Storage management
    void setStorageManager(IStorageManager* storageManager);
    IStorageManager* getStorageManager() const;

    // Provider Management
    QList<TranscriptionProvider> getAvailableProviders() const override;
    bool setProvider(TranscriptionProvider provider) override;
    TranscriptionProvider getCurrentProvider() const override;
    bool isProviderAvailable(TranscriptionProvider provider) const override;
    bool isOfflineCapable() const override;

    // Model Management (whisper.cpp specific)
    bool downloadModel(TranscriptionProvider model) override;
    bool isModelDownloaded(TranscriptionProvider model) const override;
    void removeModel(TranscriptionProvider model) override;
    qint64 getModelSize(TranscriptionProvider model) const override;
    QString getModelPath(TranscriptionProvider model) const override;

    // Language Support
    QStringList getSupportedLanguages() const override;
    QString detectLanguage(const QString& audioFilePath) override;
    void setDefaultLanguage(const QString& languageCode) override;

    // Transcription Operations
    QString submitTranscription(const TranscriptionRequest& request) override;
    void cancelTranscription(const QString& requestId) override;
    TranscriptionStatus getTranscriptionStatus(const QString& requestId) const override;
    TranscriptionResult getTranscriptionResult(const QString& requestId) const override;

    // Batch Operations
    QStringList submitBatchTranscription(const QList<TranscriptionRequest>& requests) override;
    QList<TranscriptionResult> getBatchResults(const QStringList& requestIds) const override;

    // Configuration
    void setMaxConcurrentRequests(int maxRequests) override;
    void setTimeout(int timeoutMs) override;
    void setThreadCount(int threadCount) override;

    // Quality & Performance
    double getProviderAccuracy(TranscriptionProvider provider) const override;
    qint64 getAverageProcessingTime(TranscriptionProvider provider) const override;
    int getQueueLength() const override;

    // Audio Format Support
    QStringList getSupportedFormats() const override;
    bool isFormatSupported(const QString& format) const override;
    QString getRecommendedFormat() const override;

    // Error Handling
    TranscriptionError getLastError() const override;
    QString getErrorString() const override;
    void clearErrorState() override;

public slots:
    void clearCache() override;
    void preloadModel(TranscriptionProvider model) override;

private slots:
    void handleTaskCompleted(const QString& requestId, const TranscriptionResult& result);
    void handleTaskFailed(const QString& requestId, TranscriptionError error, const QString& errorMessage);
    void handleTaskProgress(const QString& requestId, int progressPercent);
    void handleModelDownloadProgress();
    void cleanupCompletedTasks();
    void initializeModelInfo();
    void processNextPendingRequest();

private:
    // Core components
    QThreadPool* m_threadPool;
    QNetworkAccessManager* m_networkManager;
    
    // State management
    TranscriptionProvider m_currentProvider;
    TranscriptionError m_lastError;
    QString m_errorString;
    QString m_defaultLanguage;
    int m_timeoutMs;
    int m_maxConcurrentRequests;
    
    // Request tracking
    struct RequestInfo {
        TranscriptionRequest request;
        TranscriptionStatus status;
        TranscriptionResult result;
        QElapsedTimer timer;
        bool hasResult;
    };
    
    mutable QMutex m_requestsMutex;
    QMap<QString, RequestInfo> m_activeRequests;
    QQueue<QString> m_pendingRequests;
    QAtomicInt m_requestCounter;
    
    // Model management
    mutable QMutex m_modelsMutex;
    QMap<TranscriptionProvider, whisper_context*> m_loadedModels;
    QMap<TranscriptionProvider, QString> m_modelPaths;
    QMap<TranscriptionProvider, qint64> m_modelSizes;
    
    // Performance tracking
    QMap<TranscriptionProvider, QList<qint64>> m_processingTimes;
    QMap<TranscriptionProvider, double> m_accuracyRatings;
    
    // Cleanup timer
    QTimer* m_cleanupTimer;
    
    // Storage integration
    IStorageManager* m_storageManager;
    
    // Storage integration methods
    void saveTranscriptionToStorage(const TranscriptionResult& result);
    void saveTranscriptionToStorage(const QString& requestId, const TranscriptionResult& result);
    void updateTranscriptionInStorage(const QString& transcriptionId, const TranscriptionResult& result);
    
    // Helper methods
    QString generateRequestId();
    bool initializeProvider(TranscriptionProvider provider);
    void cleanupProvider(TranscriptionProvider provider);
    
    // Whisper.cpp integration
    whisper_context* loadWhisperModel(TranscriptionProvider provider);
    void unloadWhisperModel(TranscriptionProvider provider);
    bool isWhisperModelLoaded(TranscriptionProvider provider) const;
    TranscriptionResult processWithWhisper(const TranscriptionRequest& request, const QString& requestId);
    
    // Model file management
    QString getModelFileName(TranscriptionProvider provider) const;
    QString getModelsDirectory() const;
    bool validateModelFile(const QString& modelPath) const;
    bool createModelsDirectory() const;
    
    // Audio preprocessing
    bool preprocessAudioFile(const QString& inputPath, QString& outputPath);
    bool convertToWavFormat(const QString& inputPath, const QString& outputPath);
    bool validateAudioFormat(const QString& audioPath);
    
    // Language detection and processing
    QString detectLanguageFromAudio(const QString& audioPath);
    QStringList extractWordTimestamps(whisper_context* ctx, int segmentCount);
    
    // Performance tracking
    void updateProcessingTime(TranscriptionProvider provider, qint64 processingTime);
    void updateAccuracyRating(TranscriptionProvider provider, double accuracy);
    
    // Request management
    void setRequestStatus(const QString& requestId, TranscriptionStatus status);
    void setRequestResult(const QString& requestId, const TranscriptionResult& result);
    bool isRequestValid(const QString& requestId) const;
    void removeCompletedRequest(const QString& requestId);
    
    // Error handling
    void setError(TranscriptionError error, const QString& errorMessage);
    TranscriptionError mapWhisperError(int whisperError) const;
    
    // Model download helpers
    void downloadModelAsync(TranscriptionProvider model);
    QString getModelDownloadUrl(TranscriptionProvider model) const;
    bool verifyModelIntegrity(const QString& modelPath) const;
    
    // Constants
    static constexpr int DEFAULT_TIMEOUT_MS = 30000;
    static constexpr int DEFAULT_MAX_CONCURRENT = 2;
    static constexpr int DEFAULT_THREAD_COUNT = 2;
    static constexpr int CLEANUP_INTERVAL_MS = 60000; // 1 minute
    static constexpr int MAX_COMPLETED_REQUESTS = 100;
    
    // Model size constants (approximate sizes in bytes)
    static constexpr qint64 WHISPER_TINY_SIZE = 39 * 1024 * 1024;    // ~39MB
    static constexpr qint64 WHISPER_BASE_SIZE = 142 * 1024 * 1024;   // ~142MB
    static constexpr qint64 WHISPER_SMALL_SIZE = 244 * 1024 * 1024;  // ~244MB
    static constexpr qint64 WHISPER_MEDIUM_SIZE = 769 * 1024 * 1024; // ~769MB
    static constexpr qint64 WHISPER_LARGE_SIZE = 1550 * 1024 * 1024; // ~1.5GB
};
