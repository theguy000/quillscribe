#include "TextEnhancementService.h"
#include <QDebug>
#include <QUrl>
#include <QUrlQuery>
#include <QNetworkRequest>
#include <QCoreApplication>
#include <QDir>
#include <QStandardPaths>
#include <QRegularExpression>
#include <QTextBoundaryFinder>
#include <QCryptographicHash>
#include <QDateTime>
#include <QUuid>
#include <QtMath>
#include <algorithm>

TextEnhancementService::TextEnhancementService(QObject* parent)
    : ITextEnhancementService(parent)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_timeoutTimer(new QTimer(this))
    , m_cleanupTimer(new QTimer(this))
    , m_settings(new QSettings(this))
    , m_currentProvider(EnhancementProvider::GeminiPro)
    , m_timeoutMs(DEFAULT_TIMEOUT_MS)
    , m_maxConcurrentRequests(DEFAULT_MAX_CONCURRENT)
    , m_cachingEnabled(true)
    , m_isOnline(true)
    , m_requestCounter(0)
    , m_maxCacheSize(MAX_CACHE_SIZE_MB * 1024 * 1024)
    , m_cacheExpiryHours(CACHE_EXPIRY_HOURS)
    , m_lastError(EnhancementError::NoError)
{
    // Setup network manager
    connect(m_networkManager, &QNetworkAccessManager::finished,
            this, &TextEnhancementService::handleNetworkReplyFinished);

    // Setup timeout timer
    m_timeoutTimer->setSingleShot(true);
    connect(m_timeoutTimer, &QTimer::timeout, this, &TextEnhancementService::handleTimeout);

    // Setup cleanup timer
    m_cleanupTimer->setInterval(CLEANUP_INTERVAL_MS);
    connect(m_cleanupTimer, &QTimer::timeout, this, &TextEnhancementService::cleanupCompletedRequests);
    m_cleanupTimer->start();

    // Load settings and initialize
    loadSettings();
    initializeDefaultSettings();

    qDebug() << "TextEnhancementService initialized with provider:" << static_cast<int>(m_currentProvider);
}

TextEnhancementService::~TextEnhancementService() {
    // Cancel all active requests
    QMutexLocker locker(&m_requestsMutex);
    for (auto it = m_activeRequests.begin(); it != m_activeRequests.end(); ++it) {
        if (it.value().networkReply) {
            it.value().networkReply->abort();
        }
    }
    m_activeRequests.clear();
    
    saveSettings();
}

QList<EnhancementProvider> TextEnhancementService::getAvailableProviders() const {
    return {
        EnhancementProvider::GeminiPro,
        EnhancementProvider::GeminiFlash
    };
}

bool TextEnhancementService::setProvider(EnhancementProvider provider) {
    if (!getAvailableProviders().contains(provider)) {
        setError(EnhancementError::ServiceUnavailable, "Invalid enhancement provider");
        return false;
    }

    if (m_apiKey.isEmpty()) {
        setError(EnhancementError::InvalidApiKey, "API key not configured");
        return false;
    }

    m_currentProvider = provider;
    clearErrorState();
    return true;
}

EnhancementProvider TextEnhancementService::getCurrentProvider() const {
    return m_currentProvider;
}

bool TextEnhancementService::isProviderAvailable(EnhancementProvider provider) const {
    return getAvailableProviders().contains(provider) && !m_apiKey.isEmpty() && m_isOnline;
}

QList<EnhancementMode> TextEnhancementService::getSupportedModes() const {
    return {
        EnhancementMode::GrammarOnly,
        EnhancementMode::StyleImprovement,
        EnhancementMode::Summarization,
        EnhancementMode::Formalization,
        EnhancementMode::Custom
    };
}

QString TextEnhancementService::getModeDescription(EnhancementMode mode) const {
    switch (mode) {
        case EnhancementMode::GrammarOnly:
            return "Fix grammar, punctuation, and spelling errors only";
        case EnhancementMode::StyleImprovement:
            return "Improve clarity, flow, and sentence structure";
        case EnhancementMode::Summarization:
            return "Condense text while preserving key points";
        case EnhancementMode::Formalization:
            return "Make text more professional and formal";
        case EnhancementMode::Custom:
            return "Apply custom enhancement instructions";
        default:
            return "Unknown enhancement mode";
    }
}

EnhancementSettings TextEnhancementService::getDefaultSettings(EnhancementMode mode) const {
    EnhancementSettings settings;
    settings.mode = mode;
    settings.preserveFormatting = true;
    settings.maxOutputLength = 2000;
    settings.creativity = 0.3; // Conservative by default
    settings.targetAudience = "general";
    settings.tone = "professional";

    switch (mode) {
        case EnhancementMode::GrammarOnly:
            settings.creativity = 0.1; // Very conservative
            break;
        case EnhancementMode::StyleImprovement:
            settings.creativity = 0.5; // Moderate creativity
            break;
        case EnhancementMode::Summarization:
            settings.creativity = 0.2;
            settings.maxOutputLength = 500; // Shorter output for summaries
            break;
        case EnhancementMode::Formalization:
            settings.creativity = 0.3;
            settings.tone = "formal";
            break;
        case EnhancementMode::Custom:
            settings.creativity = 0.4;
            break;
    }

    return settings;
}

bool TextEnhancementService::validateSettings(const EnhancementSettings& settings) const {
    if (settings.maxOutputLength <= 0 || settings.maxOutputLength > MAX_TEXT_LENGTH) {
        return false;
    }

    if (settings.creativity < 0.0 || settings.creativity > 1.0) {
        return false;
    }

    if (settings.mode == EnhancementMode::Custom && settings.customPrompt.isEmpty()) {
        return false;
    }

    return true;
}

QString TextEnhancementService::submitEnhancement(const EnhancementRequest& request) {
    // Validate request
    if (request.text.isEmpty()) {
        setError(EnhancementError::InvalidPrompt, "Empty text provided");
        return QString();
    }

    if (isTextTooLong(request.text)) {
        setError(EnhancementError::TextTooLong, "Text exceeds maximum length");
        return QString();
    }

    if (!validateSettings(request.settings)) {
        setError(EnhancementError::InvalidPrompt, "Invalid enhancement settings");
        return QString();
    }

    if (!isProviderAvailable(request.preferredProvider)) {
        setError(EnhancementError::ServiceUnavailable, "Enhancement provider not available");
        return QString();
    }

    // Check cache first if caching is enabled
    if (m_cachingEnabled) {
        QString cacheKey = generateCacheKey(request);
        if (isRequestCached(cacheKey)) {
            QString requestId = generateRequestId();
            EnhancementResult cachedResult = getCachedResult(cacheKey);
            cachedResult.id = requestId; // Update ID for this request
            
            // Create request info for cached result
            RequestInfo info;
            info.request = request;
            info.status = EnhancementStatus::Completed;
            info.result = cachedResult;
            info.hasResult = true;
            info.networkReply = nullptr;
            info.retryCount = 0;
            info.timer.start();

            {
                QMutexLocker locker(&m_requestsMutex);
                m_activeRequests[requestId] = info;
            }

            // Emit completion immediately
            QTimer::singleShot(0, this, [this, requestId, cachedResult]() {
                emit enhancementCompleted(requestId, cachedResult);
            });

            return requestId;
        }
    }

    // Generate request ID and create request info
    QString requestId = generateRequestId();
    RequestInfo info;
    info.request = request;
    info.status = EnhancementStatus::Pending;
    info.hasResult = false;
    info.networkReply = nullptr;
    info.retryCount = 0;
    info.timer.start();

    {
        QMutexLocker locker(&m_requestsMutex);
        m_activeRequests[requestId] = info;

        // Check if we can process immediately or need to queue
        int activeCount = 0;
        for (const auto& req : m_activeRequests) {
            if (req.status == EnhancementStatus::Processing) {
                activeCount++;
            }
        }

        if (activeCount < m_maxConcurrentRequests) {
            processEnhancementRequest(requestId);
        } else {
            m_pendingRequests.enqueue(requestId);
        }
    }

    return requestId;
}

void TextEnhancementService::cancelEnhancement(const QString& requestId) {
    QMutexLocker locker(&m_requestsMutex);

    if (m_activeRequests.contains(requestId)) {
        RequestInfo& info = m_activeRequests[requestId];
        
        if (info.networkReply) {
            info.networkReply->abort();
        }
        
        info.status = EnhancementStatus::Cancelled;
        emit enhancementCancelled(requestId);
    }

    // Remove from pending queue
    QQueue<QString> newQueue;
    while (!m_pendingRequests.isEmpty()) {
        QString id = m_pendingRequests.dequeue();
        if (id != requestId) {
            newQueue.enqueue(id);
        }
    }
    m_pendingRequests = newQueue;
}

EnhancementStatus TextEnhancementService::getEnhancementStatus(const QString& requestId) const {
    QMutexLocker locker(&m_requestsMutex);

    if (m_activeRequests.contains(requestId)) {
        return m_activeRequests.value(requestId).status;
    }

    return EnhancementStatus::Failed;
}

EnhancementResult TextEnhancementService::getEnhancementResult(const QString& requestId) const {
    QMutexLocker locker(&m_requestsMutex);

    if (m_activeRequests.contains(requestId)) {
        const RequestInfo& info = m_activeRequests.value(requestId);
        if (info.hasResult) {
            return info.result;
        }
    }

    return EnhancementResult();
}

QStringList TextEnhancementService::submitBatchEnhancement(const QList<EnhancementRequest>& requests) {
    QStringList requestIds;

    for (const auto& request : requests) {
        QString requestId = submitEnhancement(request);
        if (!requestId.isEmpty()) {
            requestIds.append(requestId);
        }
    }

    return requestIds;
}

QList<EnhancementResult> TextEnhancementService::getBatchResults(const QStringList& requestIds) const {
    QList<EnhancementResult> results;

    for (const auto& requestId : requestIds) {
        EnhancementResult result = getEnhancementResult(requestId);
        if (!result.id.isEmpty()) {
            results.append(result);
        }
    }

    return results;
}

int TextEnhancementService::estimateWordCount(const QString& text) const {
    return countWords(text);
}

qint64 TextEnhancementService::estimateProcessingTime(const QString& text, EnhancementMode mode) const {
    int wordCount = countWords(text);
    qint64 baseTime = 2000; // 2 seconds base

    // Adjust based on word count
    baseTime += (wordCount / 100) * 500; // Add 500ms per 100 words

    // Adjust based on mode complexity
    switch (mode) {
        case EnhancementMode::GrammarOnly:
            baseTime *= 0.8; // Faster
            break;
        case EnhancementMode::StyleImprovement:
            baseTime *= 1.2; // Slower
            break;
        case EnhancementMode::Summarization:
            baseTime *= 1.1;
            break;
        case EnhancementMode::Formalization:
            baseTime *= 1.0;
            break;
        case EnhancementMode::Custom:
            baseTime *= 1.3; // Custom prompts may be more complex
            break;
    }

    // Adjust based on provider
    if (getCurrentProvider() == EnhancementProvider::GeminiFlash) {
        baseTime *= 0.7; // Flash is faster
    }

    return qBound(1000LL, baseTime, 30000LL); // Between 1-30 seconds
}

bool TextEnhancementService::isTextTooLong(const QString& text) const {
    return text.length() > MAX_TEXT_LENGTH || countWords(text) > MAX_WORD_COUNT;
}

QString TextEnhancementService::detectLanguage(const QString& text) const {
    return detectTextLanguage(text);
}

double TextEnhancementService::assessTextQuality(const QString& text) const {
    double grammarScore = assessGrammarQuality(text);
    double styleScore = assessStyleQuality(text);
    double clarityScore = assessClarityScore(text);

    // Weighted average
    return (grammarScore * 0.4 + styleScore * 0.3 + clarityScore * 0.3);
}

QStringList TextEnhancementService::identifyIssues(const QString& text) const {
    QStringList issues;
    
    issues.append(findGrammarIssues(text));
    issues.append(findStyleIssues(text));
    issues.append(findReadabilityIssues(text));
    
    return issues;
}

QString TextEnhancementService::suggestBestMode(const QString& text) const {
    double quality = assessTextQuality(text);
    QStringList issues = identifyIssues(text);
    
    // Count different types of issues
    int grammarIssues = 0;
    int styleIssues = 0;
    
    for (const QString& issue : issues) {
        if (issue.contains("grammar", Qt::CaseInsensitive) || 
            issue.contains("spelling", Qt::CaseInsensitive)) {
            grammarIssues++;
        } else {
            styleIssues++;
        }
    }
    
    if (grammarIssues > styleIssues * 2) {
        return "GrammarOnly";
    } else if (countWords(text) > 500) {
        return "Summarization";
    } else if (quality < 0.6) {
        return "StyleImprovement";
    } else {
        return "Formalization";
    }
}

void TextEnhancementService::setApiKey(const QString& apiKey) {
    m_apiKey = apiKey;
    m_settings->setValue("apiKey", apiKey);
    clearErrorState();
}

void TextEnhancementService::setDefaultSettings(const EnhancementSettings& settings) {
    if (validateSettings(settings)) {
        m_defaultSettings = settings;
        // Save settings
        m_settings->beginGroup("defaultSettings");
        m_settings->setValue("maxOutputLength", settings.maxOutputLength);
        m_settings->setValue("creativity", settings.creativity);
        m_settings->setValue("targetAudience", settings.targetAudience);
        m_settings->setValue("tone", settings.tone);
        m_settings->setValue("preserveFormatting", settings.preserveFormatting);
        m_settings->endGroup();
    }
}

void TextEnhancementService::setTimeout(int timeoutMs) {
    m_timeoutMs = qMax(1000, timeoutMs);
}

void TextEnhancementService::setMaxConcurrentRequests(int maxRequests) {
    m_maxConcurrentRequests = qMax(1, maxRequests);
}

qint64 TextEnhancementService::getAverageProcessingTime(EnhancementProvider provider) const {
    const auto& times = m_processingTimes.value(provider);
    if (times.isEmpty()) {
        return 0;
    }

    qint64 sum = 0;
    for (qint64 time : times) {
        sum += time;
    }

    return sum / times.size();
}

double TextEnhancementService::getProviderReliability(EnhancementProvider provider) const {
    const auto& rates = m_successRates.value(provider);
    if (rates.isEmpty()) {
        return 1.0;
    }

    int successCount = 0;
    for (bool success : rates) {
        if (success) successCount++;
    }

    return static_cast<double>(successCount) / rates.size();
}

int TextEnhancementService::getQueueLength() const {
    QMutexLocker locker(&m_requestsMutex);
    return m_pendingRequests.size();
}

EnhancementError TextEnhancementService::getLastError() const {
    return m_lastError;
}

QString TextEnhancementService::getErrorString() const {
    return m_errorString;
}

void TextEnhancementService::clearErrorState() {
    m_lastError = EnhancementError::NoError;
    m_errorString.clear();
}

void TextEnhancementService::enableCaching(bool enable) {
    m_cachingEnabled = enable;
    if (!enable) {
        clearCache();
    }
}

void TextEnhancementService::clearCache() {
    QMutexLocker locker(&m_cacheMutex);
    m_cache.clear();
}

qint64 TextEnhancementService::getCacheSize() const {
    QMutexLocker locker(&m_cacheMutex);
    
    qint64 totalSize = 0;
    for (const auto& entry : m_cache) {
        totalSize += entry.result.originalText.toUtf8().size();
        totalSize += entry.result.enhancedText.toUtf8().size();
    }
    
    return totalSize;
}

void TextEnhancementService::onNetworkStatusChanged(bool online) {
    m_isOnline = online;
    if (online) {
        retryFailedEnhancements();
    }
    emit networkStatusChanged(online);
}

void TextEnhancementService::retryFailedEnhancements() {
    QMutexLocker locker(&m_requestsMutex);
    
    while (!m_failedRequests.isEmpty()) {
        QString requestId = m_failedRequests.dequeue();
        if (m_activeRequests.contains(requestId)) {
            RequestInfo& info = m_activeRequests[requestId];
            if (shouldRetryRequest(info)) {
                scheduleRetry(requestId);
            }
        }
    }
}

void TextEnhancementService::onSettingsChanged(const EnhancementSettings& settings) {
    setDefaultSettings(settings);
}

// Private helper methods implementation

QString TextEnhancementService::generateRequestId() {
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

QString TextEnhancementService::generateCacheKey(const EnhancementRequest& request) const {
    QString keyData = request.text + 
                     QString::number(static_cast<int>(request.settings.mode)) +
                     request.settings.customPrompt +
                     QString::number(request.settings.creativity) +
                     request.settings.targetAudience +
                     request.settings.tone;
    
    return QCryptographicHash::hash(keyData.toUtf8(), QCryptographicHash::Md5).toHex();
}

bool TextEnhancementService::isRequestCached(const QString& cacheKey) const {
    QMutexLocker locker(&m_cacheMutex);
    
    if (!m_cache.contains(cacheKey)) {
        return false;
    }
    
    const CacheEntry& entry = m_cache.value(cacheKey);
    QDateTime expiryTime = entry.timestamp.addSecs(m_cacheExpiryHours * 3600);
    
    return QDateTime::currentDateTime() < expiryTime;
}

EnhancementResult TextEnhancementService::getCachedResult(const QString& cacheKey) {
    QMutexLocker locker(&m_cacheMutex);
    
    if (m_cache.contains(cacheKey)) {
        CacheEntry& entry = m_cache[cacheKey];
        entry.accessCount++;
        return entry.result;
    }
    
    return EnhancementResult();
}

void TextEnhancementService::cacheResult(const QString& cacheKey, const EnhancementResult& result) {
    if (!m_cachingEnabled) return;
    
    QMutexLocker locker(&m_cacheMutex);
    
    CacheEntry entry;
    entry.result = result;
    entry.timestamp = QDateTime::currentDateTime();
    entry.accessCount = 1;
    
    m_cache[cacheKey] = entry;
    
    // Check if we need to evict old entries
    if (getCacheSize() > m_maxCacheSize) {
        evictOldCacheEntries();
    }
}

void TextEnhancementService::evictOldCacheEntries() {
    // Remove oldest entries first
    QStringList keysToRemove;
    QDateTime cutoff = QDateTime::currentDateTime().addSecs(-m_cacheExpiryHours * 3600);
    
    for (auto it = m_cache.begin(); it != m_cache.end(); ++it) {
        if (it.value().timestamp < cutoff) {
            keysToRemove.append(it.key());
        }
    }
    
    for (const QString& key : keysToRemove) {
        m_cache.remove(key);
    }
    
    // If still over limit, remove least accessed entries
    while (getCacheSize() > m_maxCacheSize && !m_cache.isEmpty()) {
        auto minIt = std::min_element(m_cache.begin(), m_cache.end(),
                                    [](const CacheEntry& a, const CacheEntry& b) {
                                        return a.accessCount < b.accessCount;
                                    });
        m_cache.erase(minIt);
    }
}

int TextEnhancementService::countWords(const QString& text) const {
    if (text.isEmpty()) return 0;
    
    QTextBoundaryFinder finder(QTextBoundaryFinder::Word, text);
    int wordCount = 0;
    
    while (finder.toNextBoundary() != -1) {
        if (finder.boundaryReasons() & QTextBoundaryFinder::StartOfItem) {
            QString word = text.mid(finder.position(), 
                                  finder.toNextBoundary() - finder.position());
            if (!word.trimmed().isEmpty() && word.contains(QRegularExpression("[a-zA-Z]"))) {
                wordCount++;
            }
        }
    }
    
    return wordCount;
}

QString TextEnhancementService::buildGeminiPrompt(const EnhancementRequest& request) const {
    QString prompt;
    
    switch (request.settings.mode) {
        case EnhancementMode::GrammarOnly:
            prompt = getGrammarPrompt();
            break;
        case EnhancementMode::StyleImprovement:
            prompt = getStylePrompt();
            break;
        case EnhancementMode::Summarization:
            prompt = getSummarizationPrompt();
            break;
        case EnhancementMode::Formalization:
            prompt = getFormalizationPrompt();
            break;
        case EnhancementMode::Custom:
            prompt = buildCustomPrompt(request.settings);
            break;
    }
    
    // Add context based on settings
    if (!request.settings.targetAudience.isEmpty()) {
        prompt += QString("\n\nTarget audience: %1").arg(request.settings.targetAudience);
    }
    
    if (!request.settings.tone.isEmpty()) {
        prompt += QString("\nDesired tone: %1").arg(request.settings.tone);
    }
    
    if (request.settings.preserveFormatting) {
        prompt += "\n\nPreserve the original formatting and structure.";
    }
    
    prompt += "\n\nText to enhance:\n" + request.text;
    
    return prompt;
}

QString TextEnhancementService::getGrammarPrompt() const {
    return "Fix any grammar, punctuation, and spelling errors in the following text. "
           "Maintain the original meaning and style. Only make minimal necessary changes.";
}

QString TextEnhancementService::getStylePrompt() const {
    return "Improve the clarity, flow, and readability of the following text. "
           "Enhance sentence structure and word choice while preserving the original meaning. "
           "Make it more engaging and easier to read.";
}

QString TextEnhancementService::getSummarizationPrompt() const {
    return "Summarize the following text, preserving the key points and main ideas. "
           "Make it concise while ensuring no important information is lost. "
           "Maintain a clear and logical flow.";
}

QString TextEnhancementService::getFormalizationPrompt() const {
    return "Rewrite the following text in a more professional and formal tone. "
           "Use appropriate business language while maintaining clarity and readability. "
           "Ensure the content remains accurate and complete.";
}

QString TextEnhancementService::buildCustomPrompt(const EnhancementSettings& settings) const {
    return settings.customPrompt;
}

void TextEnhancementService::setError(EnhancementError error, const QString& errorMessage) {
    m_lastError = error;
    m_errorString = errorMessage;
    qWarning() << "TextEnhancementService error:" << errorMessage;
}

void TextEnhancementService::loadSettings() {
    m_apiKey = m_settings->value("apiKey", "").toString();
    
    m_settings->beginGroup("defaultSettings");
    m_defaultSettings.maxOutputLength = m_settings->value("maxOutputLength", 2000).toInt();
    m_defaultSettings.creativity = m_settings->value("creativity", 0.3).toDouble();
    m_defaultSettings.targetAudience = m_settings->value("targetAudience", "general").toString();
    m_defaultSettings.tone = m_settings->value("tone", "professional").toString();
    m_defaultSettings.preserveFormatting = m_settings->value("preserveFormatting", true).toBool();
    m_settings->endGroup();
}

void TextEnhancementService::saveSettings() {
    m_settings->setValue("apiKey", m_apiKey);
}

void TextEnhancementService::initializeDefaultSettings() {
    m_defaultSettings = getDefaultSettings(EnhancementMode::StyleImprovement);
}

// Simplified implementations for complex text analysis methods
double TextEnhancementService::assessGrammarQuality(const QString& text) const {
    // Simple heuristic - count common grammar issues
    int issueCount = 0;
    
    // Check for basic issues (this is simplified)
    QStringList commonIssues = {"teh", "recieve", "occured", "seperate", "definately"};
    for (const QString& issue : commonIssues) {
        if (text.contains(issue, Qt::CaseInsensitive)) {
            issueCount++;
        }
    }
    
    // Return score based on issues found
    return qMax(0.0, 1.0 - (static_cast<double>(issueCount) / 10.0));
}

double TextEnhancementService::assessStyleQuality(const QString& text) const {
    // Simple readability assessment
    int wordCount = countWords(text);
    int sentenceCount = countSentences(text);
    
    if (sentenceCount == 0) return 0.5;
    
    double avgWordsPerSentence = static_cast<double>(wordCount) / sentenceCount;
    
    // Optimal range is 15-20 words per sentence
    double score = 1.0 - qAbs(avgWordsPerSentence - 17.5) / 17.5;
    return qBound(0.0, score, 1.0);
}

double TextEnhancementService::assessClarityScore(const QString& text) const {
    // Simple clarity metric based on word length and sentence complexity
    int totalChars = text.length();
    int wordCount = countWords(text);
    
    if (wordCount == 0) return 0.5;
    
    double avgWordLength = static_cast<double>(totalChars) / wordCount;
    
    // Optimal average word length is around 4-6 characters
    double score = 1.0 - qAbs(avgWordLength - 5.0) / 5.0;
    return qBound(0.0, score, 1.0);
}

int TextEnhancementService::countSentences(const QString& text) const {
    return text.count('.') + text.count('!') + text.count('?');
}

QStringList TextEnhancementService::findGrammarIssues(const QString& text) const {
    QStringList issues;
    
    // Simple grammar issue detection (placeholder)
    if (text.contains(QRegularExpression("\\bteh\\b"))) {
        issues << "Spelling: 'teh' should be 'the'";
    }
    
    if (text.contains(QRegularExpression("\\bit's\\s+own"))) {
        issues << "Grammar: Consider 'its own' instead of 'it's own'";
    }
    
    return issues;
}

QStringList TextEnhancementService::findStyleIssues(const QString& text) const {
    QStringList issues;
    
    // Check for overly long sentences
    QStringList sentences = text.split(QRegularExpression("[.!?]"));
    for (const QString& sentence : sentences) {
        if (countWords(sentence.trimmed()) > 25) {
            issues << "Style: Consider breaking up long sentences";
        }
    }
    
    return issues;
}

QStringList TextEnhancementService::findReadabilityIssues(const QString& text) const {
    QStringList issues;
    
    // Check for passive voice (simplified)
    if (text.contains(QRegularExpression("\\b(was|were)\\s+\\w+ed\\b"))) {
        issues << "Readability: Consider using active voice";
    }
    
    return issues;
}

QString TextEnhancementService::detectTextLanguage(const QString& text) const {
    // Simple language detection (placeholder)
    // In a real implementation, this would use a language detection library
    Q_UNUSED(text)
    return "en"; // Default to English
}

// Network handling methods (simplified)
void TextEnhancementService::handleNetworkReplyFinished() {
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    
    // Find the corresponding request
    QString requestId;
    {
        QMutexLocker locker(&m_requestsMutex);
        for (auto it = m_activeRequests.begin(); it != m_activeRequests.end(); ++it) {
            if (it.value().networkReply == reply) {
                requestId = it.key();
                break;
            }
        }
    }
    
    if (!requestId.isEmpty()) {
        if (reply->error() == QNetworkReply::NoError) {
            // Process successful response
            QByteArray responseData = reply->readAll();
            // Parse and handle response (implementation details omitted for brevity)
        } else {
            // Handle error
            EnhancementError error = mapNetworkError(reply->error());
            handleTaskFailed(requestId, error, reply->errorString());
        }
    }
    
    reply->deleteLater();
}

void TextEnhancementService::handleNetworkError(QNetworkReply::NetworkError error) {
    qWarning() << "Network error:" << error;
}

void TextEnhancementService::handleTimeout() {
    // Handle request timeouts
    qWarning() << "Enhancement request timed out";
}

void TextEnhancementService::cleanupCompletedRequests() {
    QMutexLocker locker(&m_requestsMutex);
    
    QStringList toRemove;
    for (auto it = m_activeRequests.begin(); it != m_activeRequests.end(); ++it) {
        const RequestInfo& info = it.value();
        if ((info.status == EnhancementStatus::Completed || 
             info.status == EnhancementStatus::Failed ||
             info.status == EnhancementStatus::Cancelled) &&
            info.timer.elapsed() > 300000) { // 5 minutes
            
            toRemove.append(it.key());
        }
    }
    
    for (const QString& requestId : toRemove) {
        m_activeRequests.remove(requestId);
    }
}

// Placeholder implementations for complex methods
void TextEnhancementService::processEnhancementRequest(const QString& requestId) {
    // This would contain the actual Gemini API call implementation
    // For now, it's a placeholder
    Q_UNUSED(requestId)
}

EnhancementError TextEnhancementService::mapNetworkError(QNetworkReply::NetworkError error) const {
    switch (error) {
        case QNetworkReply::AuthenticationRequiredError:
            return EnhancementError::AuthenticationError;
        case QNetworkReply::TimeoutError:
            return EnhancementError::TimeoutError;
        case QNetworkReply::NetworkSessionFailedError:
        case QNetworkReply::ConnectionRefusedError:
            return EnhancementError::NetworkError;
        case QNetworkReply::ContentNotFoundError:
            return EnhancementError::ServiceUnavailable;
        default:
            return EnhancementError::NetworkError;
    }
}

bool TextEnhancementService::shouldRetryRequest(const RequestInfo& info) const {
    return info.retryCount < MAX_RETRY_COUNT && 
           info.status == EnhancementStatus::Failed &&
           m_isOnline;
}

void TextEnhancementService::scheduleRetry(const QString& requestId, int delayMs) {
    QTimer::singleShot(delayMs, this, [this, requestId]() {
        QMutexLocker locker(&m_requestsMutex);
        if (m_activeRequests.contains(requestId)) {
            RequestInfo& info = m_activeRequests[requestId];
            info.retryCount++;
            processEnhancementRequest(requestId);
        }
    });
}

void TextEnhancementService::handleTaskFailed(const QString& requestId, EnhancementError error, const QString& errorMessage) {
    {
        QMutexLocker locker(&m_requestsMutex);
        if (m_activeRequests.contains(requestId)) {
            RequestInfo& info = m_activeRequests[requestId];
            info.status = EnhancementStatus::Failed;
            
            if (shouldRetryRequest(info)) {
                m_failedRequests.enqueue(requestId);
            }
        }
    }
    
    emit enhancementFailed(requestId, error, errorMessage);
}

void TextEnhancementService::setRequestStatus(const QString& requestId, EnhancementStatus status) {
    if (m_activeRequests.contains(requestId)) {
        m_activeRequests[requestId].status = status;
    }
}

void TextEnhancementService::setRequestResult(const QString& requestId, const EnhancementResult& result) {
    if (m_activeRequests.contains(requestId)) {
        m_activeRequests[requestId].result = result;
        m_activeRequests[requestId].hasResult = true;
    }
}
