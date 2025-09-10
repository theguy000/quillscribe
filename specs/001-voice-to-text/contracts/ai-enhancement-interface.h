// AI Enhancement Interface Contract
// Contract for AI-powered text enhancement functionality

#pragma once

#include <QJsonObject>
#include <QObject>
#include <QString>
#include <QStringList>

enum class EnhancementMode {
    GrammarOnly,       // Fix grammar and punctuation only
    StyleImprovement,  // Improve clarity and flow
    Summarization,     // Condense key points
    Formalization,     // Make more professional/formal
    Custom             // User-defined enhancement prompt
};

enum class EnhancementProvider {
    GeminiPro,    // Primary: Google Gemini Pro
    GeminiFlash,  // Faster, lighter Gemini model
    LocalLLM,     // Future: Local model support
    Unknown
};

enum class EnhancementStatus { Pending, Processing, Completed, Failed, Cancelled };

enum class EnhancementError {
    NoError,
    NetworkError,
    AuthenticationError,
    TextTooLong,
    InvalidPrompt,
    ServiceUnavailable,
    QuotaExceeded,
    InvalidApiKey,
    TimeoutError,
    ContentFiltered,
    UnknownError
};

struct EnhancementSettings {
    EnhancementMode mode;
    QString customPrompt;       // Used when mode is Custom
    bool preserveFormatting;    // Keep original structure
    int maxOutputLength;        // Maximum enhanced text length
    double creativity;          // 0.0 (conservative) to 1.0 (creative)
    QString targetAudience;     // "general", "academic", "business", etc.
    QString tone;               // "professional", "casual", "formal", etc.
    QStringList preserveTerms;  // Technical terms to keep unchanged
};

struct EnhancementRequest {
    QString text;
    EnhancementSettings settings;
    EnhancementProvider preferredProvider;
    QString language = "en";  // Target language
    int timeoutMs = 10000;    // 10 second timeout
    int maxRetries = 2;
};

struct EnhancementResult {
    QString id;
    QString originalText;
    QString enhancedText;
    EnhancementMode mode;
    EnhancementProvider provider;
    qint64 processingTime;    // milliseconds
    double improvementScore;  // 0.0 to 1.0 (quality estimate)
    QJsonObject changes;      // Detailed change tracking
    QJsonObject metadata;     // Provider-specific data
    QString reasoning;        // Why changes were made (optional)
};

/**
 * @brief Interface for AI-powered text enhancement services
 *
 * Contract Requirements:
 * - FR-004: Must provide AI-powered text enhancement that improves clarity while preserving
 * original meaning
 * - FR-005: Must complete AI enhancement within 5 seconds for text under 500 words
 * - FR-014: Must provide different AI enhancement modes (grammar only, style improvement,
 * summarization)
 * - PR-002: AI enhancement must complete within 5 seconds for 500-word text
 */
class ITextEnhancementService : public QObject {
    Q_OBJECT

public:
    explicit ITextEnhancementService(QObject* parent = nullptr) : QObject(parent) {}
    virtual ~ITextEnhancementService() = default;

    // Provider Management
    virtual QList<EnhancementProvider> getAvailableProviders() const = 0;
    virtual bool setProvider(EnhancementProvider provider) = 0;
    virtual EnhancementProvider getCurrentProvider() const = 0;
    virtual bool isProviderAvailable(EnhancementProvider provider) const = 0;

    // Enhancement Modes
    virtual QList<EnhancementMode> getSupportedModes() const = 0;
    virtual QString getModeDescription(EnhancementMode mode) const = 0;
    virtual EnhancementSettings getDefaultSettings(EnhancementMode mode) const = 0;
    virtual bool validateSettings(const EnhancementSettings& settings) const = 0;

    // Enhancement Operations
    virtual QString submitEnhancement(const EnhancementRequest& request) = 0;
    virtual void cancelEnhancement(const QString& requestId) = 0;
    virtual EnhancementStatus getEnhancementStatus(const QString& requestId) const = 0;
    virtual EnhancementResult getEnhancementResult(const QString& requestId) const = 0;

    // Batch Operations
    virtual QStringList submitBatchEnhancement(const QList<EnhancementRequest>& requests) = 0;
    virtual QList<EnhancementResult> getBatchResults(const QStringList& requestIds) const = 0;

    // Text Analysis
    virtual int estimateWordCount(const QString& text) const = 0;
    virtual qint64 estimateProcessingTime(const QString& text, EnhancementMode mode) const = 0;
    virtual bool isTextTooLong(const QString& text) const = 0;
    virtual QString detectLanguage(const QString& text) const = 0;

    // Quality Assessment
    virtual double assessTextQuality(const QString& text) const = 0;
    virtual QStringList identifyIssues(const QString& text) const = 0;
    virtual QString suggestBestMode(const QString& text) const = 0;

    // Configuration
    virtual void setApiKey(const QString& apiKey) = 0;
    virtual void setDefaultSettings(const EnhancementSettings& settings) = 0;
    virtual void setTimeout(int timeoutMs) = 0;
    virtual void setMaxConcurrentRequests(int maxRequests) = 0;

    // Performance Tracking
    virtual qint64 getAverageProcessingTime(EnhancementProvider provider) const = 0;
    virtual double getProviderReliability(EnhancementProvider provider) const = 0;
    virtual int getQueueLength() const = 0;

    // Error Handling
    virtual EnhancementError getLastError() const = 0;
    virtual QString getErrorString() const = 0;
    virtual void clearErrorState() = 0;

    // Caching & Performance
    virtual void enableCaching(bool enable) = 0;
    virtual void clearCache() = 0;
    virtual qint64 getCacheSize() const = 0;

   signals:
    // Progress notifications
    void enhancementStarted(const QString& requestId, EnhancementProvider provider);
    void enhancementProgress(const QString& requestId, int progressPercent);
    void enhancementCompleted(const QString& requestId, const EnhancementResult& result);
    void enhancementFailed(const QString& requestId,
                           EnhancementError error,
                           const QString& errorMessage);
    void enhancementCancelled(const QString& requestId);

    // Provider status
    void providerStatusChanged(EnhancementProvider provider, bool available);
    void networkStatusChanged(bool online);

    // Performance metrics
    void processingTimeUpdated(EnhancementProvider provider, qint64 averageTime);
    void reliabilityUpdated(EnhancementProvider provider, double reliability);

   public slots:
    // Network management
    virtual void onNetworkStatusChanged(bool online) = 0;
    virtual void retryFailedEnhancements() = 0;

    // Settings management
    virtual void onSettingsChanged(const EnhancementSettings& settings) = 0;
};

/**
 * @brief Interface for enhancement profile management
 */
class IEnhancementProfileManager : public QObject {
    Q_OBJECT

   public:
    virtual ~IEnhancementProfileManager() = default;

    // Profile Management
    virtual QString createProfile(const QString& name, const EnhancementSettings& settings) = 0;
    virtual bool updateProfile(const QString& profileId, const EnhancementSettings& settings) = 0;
    virtual bool deleteProfile(const QString& profileId) = 0;
    virtual QStringList getProfileIds() const = 0;
    virtual QStringList getProfileNames() const = 0;

    // Profile Operations
    virtual EnhancementSettings getProfile(const QString& profileId) const = 0;
    virtual QString getProfileName(const QString& profileId) const = 0;
    virtual bool setDefaultProfile(const QString& profileId) = 0;
    virtual QString getDefaultProfile() const = 0;

    // Profile Usage
    virtual void recordProfileUsage(const QString& profileId) = 0;
    virtual QStringList getMostUsedProfiles(int count = 5) const = 0;
    virtual QDateTime getLastUsed(const QString& profileId) const = 0;

   signals:
    void profileCreated(const QString& profileId, const QString& name);
    void profileUpdated(const QString& profileId);
    void profileDeleted(const QString& profileId);
    void defaultProfileChanged(const QString& profileId);
};

/**
 * @brief Factory interface for creating enhancement services
 */
class ITextEnhancementServiceFactory {
   public:
    virtual ~ITextEnhancementServiceFactory() = default;
    virtual ITextEnhancementService* createService() = 0;
    virtual QList<EnhancementProvider> getSupportedProviders() const = 0;
    virtual bool isProviderSupported(EnhancementProvider provider) const = 0;
};

// Contract Test Requirements:
// 1. Test processing time <= 5s for 500-word text (PR-002)
// 2. Test meaning preservation in enhanced text (FR-004)
// 3. Test all enhancement modes functionality (FR-014)
// 4. Test provider fallback mechanism (Gemini Pro/Flash)
// 5. Test text length validation and limits
// 6. Test custom prompt functionality
// 7. Test batch enhancement operations
// 8. Test error handling for API failures
// 9. Test caching mechanism effectiveness
// 10. Test concurrent enhancement requests
// 11. Test profile management operations
// 12. Test quality assessment accuracy
// 13. Test language detection and support
// 14. Test timeout handling and retry logic
