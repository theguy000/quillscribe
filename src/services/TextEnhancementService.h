#pragma once

#include "../models/BaseModel.h"
#include "../../specs/001-voice-to-text/contracts/ai-enhancement-interface.h"
#include <QObject>
#include <QString>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTimer>
#include <QElapsedTimer>
#include <QMutex>
#include <QMutexLocker>
#include <QMap>
#include <QQueue>
#include <QAtomicInt>
#include <QSettings>

/**
 * @brief TextEnhancementService implementation using Google Gemini API
 * 
 * Provides AI-powered text enhancement functionality using Google's Gemini models.
 * Supports multiple enhancement modes and concurrent processing.
 */
class TextEnhancementService : public ITextEnhancementService {
    Q_OBJECT

public:
    explicit TextEnhancementService(QObject* parent = nullptr);
    ~TextEnhancementService() override;

    // Provider Management
    QList<EnhancementProvider> getAvailableProviders() const override;
    bool setProvider(EnhancementProvider provider) override;
    EnhancementProvider getCurrentProvider() const override;
    bool isProviderAvailable(EnhancementProvider provider) const override;

    // Enhancement Modes
    QList<EnhancementMode> getSupportedModes() const override;
    QString getModeDescription(EnhancementMode mode) const override;
    EnhancementSettings getDefaultSettings(EnhancementMode mode) const override;
    bool validateSettings(const EnhancementSettings& settings) const override;

    // Enhancement Operations
    QString submitEnhancement(const EnhancementRequest& request) override;
    void cancelEnhancement(const QString& requestId) override;
    EnhancementStatus getEnhancementStatus(const QString& requestId) const override;
    EnhancementResult getEnhancementResult(const QString& requestId) const override;

    // Batch Operations
    QStringList submitBatchEnhancement(const QList<EnhancementRequest>& requests) override;
    QList<EnhancementResult> getBatchResults(const QStringList& requestIds) const override;

    // Text Analysis
    int estimateWordCount(const QString& text) const override;
    qint64 estimateProcessingTime(const QString& text, EnhancementMode mode) const override;
    bool isTextTooLong(const QString& text) const override;
    QString detectLanguage(const QString& text) const override;

    // Quality Assessment
    double assessTextQuality(const QString& text) const override;
    QStringList identifyIssues(const QString& text) const override;
    QString suggestBestMode(const QString& text) const override;

    // Configuration
    void setApiKey(const QString& apiKey) override;
    void setDefaultSettings(const EnhancementSettings& settings) override;
    void setTimeout(int timeoutMs) override;
    void setMaxConcurrentRequests(int maxRequests) override;

    // Performance Tracking
    qint64 getAverageProcessingTime(EnhancementProvider provider) const override;
    double getProviderReliability(EnhancementProvider provider) const override;
    int getQueueLength() const override;

    // Error Handling
    EnhancementError getLastError() const override;
    QString getErrorString() const override;
    void clearErrorState() override;

    // Caching & Performance
    void enableCaching(bool enable) override;
    void clearCache() override;
    qint64 getCacheSize() const override;

public slots:
    void onNetworkStatusChanged(bool online) override;
    void retryFailedEnhancements() override;
    void onSettingsChanged(const EnhancementSettings& settings) override;

private slots:
    void handleNetworkReplyFinished();
    void handleNetworkError(QNetworkReply::NetworkError error);
    void handleTimeout();
    void cleanupCompletedRequests();
    void processEnhancementRequest(const QString& requestId);
    void handleTaskFailed(const QString& requestId, EnhancementError error, const QString& errorMessage);

private:
    // Core components
    QNetworkAccessManager* m_networkManager;
    QTimer* m_timeoutTimer;
    QTimer* m_cleanupTimer;
    QSettings* m_settings;

    // Configuration
    QString m_apiKey;
    EnhancementProvider m_currentProvider;
    EnhancementSettings m_defaultSettings;
    int m_timeoutMs;
    int m_maxConcurrentRequests;
    bool m_cachingEnabled;
    bool m_isOnline;

    // Request management
    struct RequestInfo {
        EnhancementRequest request;
        EnhancementStatus status;
        EnhancementResult result;
        QElapsedTimer timer;
        QNetworkReply* networkReply;
        bool hasResult;
        int retryCount;
    };

    mutable QMutex m_requestsMutex;
    QMap<QString, RequestInfo> m_activeRequests;
    QQueue<QString> m_pendingRequests;
    QQueue<QString> m_failedRequests;
    QAtomicInt m_requestCounter;

    // Performance tracking
    QMap<EnhancementProvider, QList<qint64>> m_processingTimes;
    QMap<EnhancementProvider, QList<bool>> m_successRates;

    // Caching
    struct CacheEntry {
        EnhancementResult result;
        QDateTime timestamp;
        qint64 accessCount;
    };
    
    mutable QMutex m_cacheMutex;
    QMap<QString, CacheEntry> m_cache;
    qint64 m_maxCacheSize;
    int m_cacheExpiryHours;

    // Error handling
    EnhancementError m_lastError;
    QString m_errorString;

    // Helper methods
    QString generateRequestId();
    QString generateCacheKey(const EnhancementRequest& request) const;
    bool isRequestCached(const QString& cacheKey) const;
    EnhancementResult getCachedResult(const QString& cacheKey);
    void cacheResult(const QString& cacheKey, const EnhancementResult& result);
    void evictOldCacheEntries();

    // Gemini API integration
    QString buildGeminiPrompt(const EnhancementRequest& request) const;
    QJsonObject buildGeminiRequestJson(const EnhancementRequest& request) const;
    QString getGeminiApiUrl(EnhancementProvider provider) const;
    QNetworkRequest buildGeminiRequest(EnhancementProvider provider) const;
    
    // Response processing
    EnhancementResult parseGeminiResponse(const QByteArray& responseData, 
                                        const EnhancementRequest& originalRequest,
                                        const QString& requestId) const;
    bool validateGeminiResponse(const QJsonObject& response) const;
    QString extractEnhancedText(const QJsonObject& response) const;
    double calculateImprovementScore(const QString& original, const QString& enhanced) const;
    QJsonObject analyzeTextChanges(const QString& original, const QString& enhanced) const;

    // Text analysis helpers
    int countWords(const QString& text) const;
    int countSentences(const QString& text) const;
    int countParagraphs(const QString& text) const;
    double calculateReadabilityScore(const QString& text) const;
    QStringList extractKeywords(const QString& text) const;
    QString detectTextLanguage(const QString& text) const;

    // Enhancement mode helpers
    QString getGrammarPrompt() const;
    QString getStylePrompt() const;
    QString getSummarizationPrompt() const;
    QString getFormalizationPrompt() const;
    QString buildCustomPrompt(const EnhancementSettings& settings) const;

    // Quality assessment helpers
    double assessGrammarQuality(const QString& text) const;
    double assessStyleQuality(const QString& text) const;
    double assessClarityScore(const QString& text) const;
    QStringList findGrammarIssues(const QString& text) const;
    QStringList findStyleIssues(const QString& text) const;
    QStringList findReadabilityIssues(const QString& text) const;

    // Network and error handling
    void setError(EnhancementError error, const QString& errorMessage);
    EnhancementError mapNetworkError(QNetworkReply::NetworkError error) const;
    bool shouldRetryRequest(const RequestInfo& info) const;
    void scheduleRetry(const QString& requestId, int delayMs = 5000);

    // Request lifecycle management
    void setRequestStatus(const QString& requestId, EnhancementStatus status);
    void setRequestResult(const QString& requestId, const EnhancementResult& result);
    bool isRequestValid(const QString& requestId) const;
    void removeCompletedRequest(const QString& requestId);
    void processNextPendingRequest();

    // Performance tracking
    void updateProcessingTime(EnhancementProvider provider, qint64 processingTime);
    void updateSuccessRate(EnhancementProvider provider, bool success);
    void recordProviderUsage(EnhancementProvider provider);

    // Settings management
    void loadSettings();
    void saveSettings();
    void initializeDefaultSettings();

    // Constants
    static constexpr int DEFAULT_TIMEOUT_MS = 10000;
    static constexpr int DEFAULT_MAX_CONCURRENT = 3;
    static constexpr int MAX_TEXT_LENGTH = 10000; // 10k characters
    static constexpr int MAX_WORD_COUNT = 2000;   // ~2k words
    static constexpr int CLEANUP_INTERVAL_MS = 300000; // 5 minutes
    static constexpr int MAX_RETRY_COUNT = 3;
    static constexpr int CACHE_EXPIRY_HOURS = 24;
    static constexpr qint64 MAX_CACHE_SIZE_MB = 50;

    // Gemini API endpoints
    static constexpr const char* GEMINI_PRO_ENDPOINT = "https://generativelanguage.googleapis.com/v1beta/models/gemini-pro:generateContent";
    static constexpr const char* GEMINI_FLASH_ENDPOINT = "https://generativelanguage.googleapis.com/v1beta/models/gemini-1.5-flash:generateContent";
};
