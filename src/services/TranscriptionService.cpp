#include "TranscriptionService.h"
#include "StorageManager.h"
#include "../models/Transcription.h"
#include <whisper.h>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QProcess>
#include <QCoreApplication>
#include <QUuid>
#include <QtMath>

// TranscriptionTask Implementation
TranscriptionTask::TranscriptionTask(const TranscriptionRequest& request, const QString& requestId)
    : m_request(request), m_requestId(requestId)
{
    setAutoDelete(false); // We'll manage deletion ourselves
}

void TranscriptionTask::run() {
    // This will be called from a worker thread
    // The actual whisper.cpp processing will be handled by TranscriptionService
    // This is a placeholder - the main processing logic is in TranscriptionService::processWithWhisper
}

// TranscriptionService Implementation
TranscriptionService::TranscriptionService(QObject* parent)
    : ITranscriptionService(parent)
    , m_threadPool(new QThreadPool(this))
    , m_networkManager(new QNetworkAccessManager(this))
    , m_currentProvider(TranscriptionProvider::WhisperCppBase)
    , m_lastError(TranscriptionError::NoError)
    , m_defaultLanguage("en")
    , m_timeoutMs(DEFAULT_TIMEOUT_MS)
    , m_maxConcurrentRequests(DEFAULT_MAX_CONCURRENT)
    , m_requestCounter(0)
    , m_storageManager(nullptr)
{
    // Configure thread pool
    m_threadPool->setMaxThreadCount(DEFAULT_THREAD_COUNT);
    
    // Setup cleanup timer
    m_cleanupTimer = new QTimer(this);
    m_cleanupTimer->setInterval(CLEANUP_INTERVAL_MS);
    connect(m_cleanupTimer, &QTimer::timeout, this, &TranscriptionService::cleanupCompletedTasks);
    m_cleanupTimer->start();
    
    // Initialize model paths and sizes
    initializeModelInfo();
    
    // Create models directory if it doesn't exist
    createModelsDirectory();
    
    // Initialize default provider if model is available
    if (isModelDownloaded(m_currentProvider)) {
        initializeProvider(m_currentProvider);
    }
}

TranscriptionService::~TranscriptionService() {
    // Stop all active tasks
    m_threadPool->clear();
    m_threadPool->waitForDone(5000); // Wait up to 5 seconds
    
    // Cleanup loaded models
    QMutexLocker locker(&m_modelsMutex);
    for (auto it = m_loadedModels.begin(); it != m_loadedModels.end(); ++it) {
        if (it.value()) {
            whisper_free(it.value());
        }
    }
    m_loadedModels.clear();
}

TranscriptionService::TranscriptionService(IStorageManager* storageManager, QObject* parent)
    : TranscriptionService(parent)
{
    m_storageManager = storageManager;
}

QList<TranscriptionProvider> TranscriptionService::getAvailableProviders() const {
    return {
        TranscriptionProvider::WhisperCppTiny,
        TranscriptionProvider::WhisperCppBase,
        TranscriptionProvider::WhisperCppSmall,
        TranscriptionProvider::WhisperCppMedium,
        TranscriptionProvider::WhisperCppLarge
    };
}

bool TranscriptionService::setProvider(TranscriptionProvider provider) {
    if (provider == TranscriptionProvider::Unknown) {
        setError(TranscriptionError::ModelNotFound, "Invalid provider");
        return false;
    }
    
    if (!isModelDownloaded(provider)) {
        setError(TranscriptionError::ModelNotFound, "Model not downloaded for provider");
        return false;
    }
    
    if (m_currentProvider != provider) {
        // Cleanup current provider
        cleanupProvider(m_currentProvider);
        
        // Initialize new provider
        if (initializeProvider(provider)) {
            m_currentProvider = provider;
            clearErrorState();
            return true;
        }
        
        // If initialization failed, revert to previous provider
        initializeProvider(m_currentProvider);
        return false;
    }
    
    return true;
}

TranscriptionProvider TranscriptionService::getCurrentProvider() const {
    return m_currentProvider;
}

bool TranscriptionService::isProviderAvailable(TranscriptionProvider provider) const {
    return getAvailableProviders().contains(provider) && isModelDownloaded(provider);
}

bool TranscriptionService::isOfflineCapable() const {
    return true; // All whisper.cpp providers work offline
}

bool TranscriptionService::downloadModel(TranscriptionProvider model) {
    if (!getAvailableProviders().contains(model)) {
        setError(TranscriptionError::ModelNotFound, "Invalid model type");
        return false;
    }
    
    if (isModelDownloaded(model)) {
        return true; // Already downloaded
    }
    
    emit modelDownloadStarted(model);
    downloadModelAsync(model);
    return true;
}

bool TranscriptionService::isModelDownloaded(TranscriptionProvider model) const {
    QString modelPath = getModelPath(model);
    return !modelPath.isEmpty() && QFile::exists(modelPath) && validateModelFile(modelPath);
}

void TranscriptionService::removeModel(TranscriptionProvider model) {
    QString modelPath = getModelPath(model);
    if (!modelPath.isEmpty() && QFile::exists(modelPath)) {
        // Unload model if it's currently loaded
        unloadWhisperModel(model);
        
        // Remove model file
        if (QFile::remove(modelPath)) {
            QMutexLocker locker(&m_modelsMutex);
            m_modelPaths.remove(model);
        } else {
            qWarning() << "Failed to remove model file:" << modelPath;
        }
    }
}

qint64 TranscriptionService::getModelSize(TranscriptionProvider model) const {
    QMutexLocker locker(&m_modelsMutex);
    return m_modelSizes.value(model, 0);
}

QString TranscriptionService::getModelPath(TranscriptionProvider model) const {
    QMutexLocker locker(&m_modelsMutex);
    
    if (m_modelPaths.contains(model)) {
        return m_modelPaths.value(model);
    }
    
    // Generate expected path
    QString modelsDir = getModelsDirectory();
    QString fileName = getModelFileName(model);
    QString fullPath = QDir(modelsDir).absoluteFilePath(fileName);
    
    if (QFile::exists(fullPath)) {
        m_modelPaths[model] = fullPath;
        return fullPath;
    }
    
    return QString();
}

QStringList TranscriptionService::getSupportedLanguages() const {
    // whisper.cpp supports many languages
    return {
        "en", "zh", "de", "es", "ru", "ko", "fr", "ja", "pt", "tr", "pl", "ca", "nl", 
        "ar", "sv", "it", "id", "hi", "fi", "vi", "he", "uk", "el", "ms", "cs", "ro", 
        "da", "hu", "ta", "no", "th", "ur", "hr", "bg", "lt", "la", "mi", "ml", "cy", 
        "sk", "te", "fa", "lv", "bn", "sr", "az", "sl", "kn", "et", "mk", "br", "eu", 
        "is", "hy", "ne", "mn", "bs", "kk", "sq", "sw", "gl", "mr", "pa", "si", "km", 
        "sn", "yo", "so", "af", "oc", "ka", "be", "tg", "sd", "gu", "am", "yi", "lo", 
        "uz", "fo", "ht", "ps", "tk", "nn", "mt", "sa", "lb", "my", "bo", "tl", "mg", 
        "as", "tt", "haw", "ln", "ha", "ba", "jw", "su"
    };
}

QString TranscriptionService::detectLanguage(const QString& audioFilePath) {
    return detectLanguageFromAudio(audioFilePath);
}

void TranscriptionService::setDefaultLanguage(const QString& languageCode) {
    if (getSupportedLanguages().contains(languageCode)) {
        m_defaultLanguage = languageCode;
    } else {
        qWarning() << "Unsupported language code:" << languageCode;
    }
}

QString TranscriptionService::submitTranscription(const TranscriptionRequest& request) {
    // Validate request
    if (request.audioFilePath.isEmpty() || !QFile::exists(request.audioFilePath)) {
        setError(TranscriptionError::InvalidAudioFile, "Audio file not found: " + request.audioFilePath);
        return QString();
    }
    
    if (!isFormatSupported(QFileInfo(request.audioFilePath).suffix())) {
        setError(TranscriptionError::AudioFormatError, "Unsupported audio format");
        return QString();
    }
    
    // Check if provider is available
    TranscriptionProvider provider = request.preferredProvider;
    if (provider == TranscriptionProvider::Unknown) {
        provider = m_currentProvider;
    }
    
    if (!isProviderAvailable(provider)) {
        setError(TranscriptionError::ModelNotFound, "Provider not available");
        return QString();
    }
    
    // Generate request ID
    QString requestId = generateRequestId();
    
    // Create request info
    RequestInfo info;
    info.request = request;
    info.status = TranscriptionStatus::Pending;
    info.hasResult = false;
    info.timer.start();
    
    {
        QMutexLocker locker(&m_requestsMutex);
        m_activeRequests[requestId] = info;
        
        // Check if we can process immediately or need to queue
        if (m_activeRequests.size() <= m_maxConcurrentRequests) {
            setRequestStatus(requestId, TranscriptionStatus::Processing);
            
            // Process in thread pool
            QTimer::singleShot(0, this, [this, request, requestId]() {
                TranscriptionResult result = processWithWhisper(request, requestId);
                if (result.id.isEmpty()) {
                    // Error occurred
                    handleTaskFailed(requestId, m_lastError, m_errorString);
                } else {
                    handleTaskCompleted(requestId, result);
                }
            });
        } else {
            m_pendingRequests.enqueue(requestId);
        }
    }
    
    emit transcriptionStarted(requestId, provider);
    return requestId;
}

void TranscriptionService::cancelTranscription(const QString& requestId) {
    QMutexLocker locker(&m_requestsMutex);
    
    if (m_activeRequests.contains(requestId)) {
        RequestInfo& info = m_activeRequests[requestId];
        if (info.status == TranscriptionStatus::Pending || info.status == TranscriptionStatus::Processing) {
            info.status = TranscriptionStatus::Cancelled;
            emit transcriptionCancelled(requestId);
        }
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

TranscriptionStatus TranscriptionService::getTranscriptionStatus(const QString& requestId) const {
    QMutexLocker locker(&m_requestsMutex);
    
    if (m_activeRequests.contains(requestId)) {
        return m_activeRequests.value(requestId).status;
    }
    
    return TranscriptionStatus::Failed;
}

TranscriptionResult TranscriptionService::getTranscriptionResult(const QString& requestId) const {
    QMutexLocker locker(&m_requestsMutex);
    
    if (m_activeRequests.contains(requestId)) {
        const RequestInfo& info = m_activeRequests.value(requestId);
        if (info.hasResult) {
            return info.result;
        }
    }
    
    return TranscriptionResult();
}

QStringList TranscriptionService::submitBatchTranscription(const QList<TranscriptionRequest>& requests) {
    QStringList requestIds;
    
    for (const auto& request : requests) {
        QString requestId = submitTranscription(request);
        if (!requestId.isEmpty()) {
            requestIds.append(requestId);
        }
    }
    
    return requestIds;
}

QList<TranscriptionResult> TranscriptionService::getBatchResults(const QStringList& requestIds) const {
    QList<TranscriptionResult> results;
    
    for (const auto& requestId : requestIds) {
        TranscriptionResult result = getTranscriptionResult(requestId);
        if (!result.id.isEmpty()) {
            results.append(result);
        }
    }
    
    return results;
}

void TranscriptionService::setMaxConcurrentRequests(int maxRequests) {
    m_maxConcurrentRequests = qMax(1, maxRequests);
}

void TranscriptionService::setTimeout(int timeoutMs) {
    m_timeoutMs = qMax(1000, timeoutMs);
}

void TranscriptionService::setThreadCount(int threadCount) {
    m_threadPool->setMaxThreadCount(qMax(1, threadCount));
}

double TranscriptionService::getProviderAccuracy(TranscriptionProvider provider) const {
    return m_accuracyRatings.value(provider, 0.95); // Default to 95%
}

qint64 TranscriptionService::getAverageProcessingTime(TranscriptionProvider provider) const {
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

int TranscriptionService::getQueueLength() const {
    QMutexLocker locker(&m_requestsMutex);
    return m_pendingRequests.size();
}

QStringList TranscriptionService::getSupportedFormats() const {
    return {"wav", "mp3", "flac", "m4a", "ogg"};
}

bool TranscriptionService::isFormatSupported(const QString& format) const {
    return getSupportedFormats().contains(format.toLower());
}

QString TranscriptionService::getRecommendedFormat() const {
    return "wav";
}

TranscriptionError TranscriptionService::getLastError() const {
    return m_lastError;
}

QString TranscriptionService::getErrorString() const {
    return m_errorString;
}

void TranscriptionService::clearErrorState() {
    m_lastError = TranscriptionError::NoError;
    m_errorString.clear();
}

void TranscriptionService::clearCache() {
    // Whisper.cpp doesn't have a traditional cache to clear
    // This could be used to clear temporary audio files if needed
    qDebug() << "Cache cleared (whisper.cpp doesn't use cache)";
}

void TranscriptionService::preloadModel(TranscriptionProvider model) {
    if (isModelDownloaded(model)) {
        loadWhisperModel(model);
    }
}

void TranscriptionService::handleTaskCompleted(const QString& requestId, const TranscriptionResult& result) {
    {
        QMutexLocker locker(&m_requestsMutex);
        if (m_activeRequests.contains(requestId)) {
            setRequestResult(requestId, result);
            setRequestStatus(requestId, TranscriptionStatus::Completed);
        }
    }
    
    // Save transcription to storage
    saveTranscriptionToStorage(requestId, result);
    
    emit transcriptionCompleted(requestId, result);
    
    // Process next pending request
    processNextPendingRequest();
}

void TranscriptionService::handleTaskFailed(const QString& requestId, TranscriptionError error, const QString& errorMessage) {
    {
        QMutexLocker locker(&m_requestsMutex);
        if (m_activeRequests.contains(requestId)) {
            setRequestStatus(requestId, TranscriptionStatus::Failed);
        }
    }
    
    emit transcriptionFailed(requestId, error, errorMessage);
    
    // Process next pending request
    processNextPendingRequest();
}

void TranscriptionService::handleTaskProgress(const QString& requestId, int progressPercent) {
    emit transcriptionProgress(requestId, progressPercent);
}

void TranscriptionService::handleModelDownloadProgress() {
    // Handle model download progress updates
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (reply) {
        qint64 bytesReceived = reply->bytesAvailable();
        qint64 bytesTotal = reply->header(QNetworkRequest::ContentLengthHeader).toLongLong();
        
        if (bytesTotal > 0) {
            int progress = static_cast<int>((bytesReceived * 100) / bytesTotal);
            // Emit progress signal based on which model is being downloaded
            // This would need additional tracking to determine which model
        }
    }
}

void TranscriptionService::cleanupCompletedTasks() {
    QMutexLocker locker(&m_requestsMutex);
    
    QStringList toRemove;
    for (auto it = m_activeRequests.begin(); it != m_activeRequests.end(); ++it) {
        const RequestInfo& info = it.value();
        if (info.status == TranscriptionStatus::Completed || 
            info.status == TranscriptionStatus::Failed ||
            info.status == TranscriptionStatus::Cancelled) {
            
            // Keep completed requests for a while, then remove old ones
            if (info.timer.elapsed() > 300000) { // 5 minutes
                toRemove.append(it.key());
            }
        }
    }
    
    for (const QString& requestId : toRemove) {
        m_activeRequests.remove(requestId);
    }
}

// Private helper methods implementation would continue here...
// This is a simplified version focusing on the main structure

QString TranscriptionService::generateRequestId() {
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

bool TranscriptionService::initializeProvider(TranscriptionProvider provider) {
    return loadWhisperModel(provider) != nullptr;
}

void TranscriptionService::cleanupProvider(TranscriptionProvider provider) {
    unloadWhisperModel(provider);
}

whisper_context* TranscriptionService::loadWhisperModel(TranscriptionProvider provider) {
    QMutexLocker locker(&m_modelsMutex);
    
    if (m_loadedModels.contains(provider) && m_loadedModels.value(provider)) {
        return m_loadedModels.value(provider);
    }
    
    QString modelPath = getModelPath(provider);
    if (modelPath.isEmpty()) {
        setError(TranscriptionError::ModelNotFound, "Model file not found");
        return nullptr;
    }
    
    whisper_context_params params = whisper_context_default_params();
    whisper_context* ctx = whisper_init_from_file_with_params(modelPath.toUtf8().constData(), params);
    if (!ctx) {
        setError(TranscriptionError::ModelLoadError, "Failed to load whisper model");
        return nullptr;
    }
    
    m_loadedModels[provider] = ctx;
    return ctx;
}

void TranscriptionService::unloadWhisperModel(TranscriptionProvider provider) {
    QMutexLocker locker(&m_modelsMutex);
    
    if (m_loadedModels.contains(provider) && m_loadedModels.value(provider)) {
        whisper_free(m_loadedModels.value(provider));
        m_loadedModels.remove(provider);
    }
}

// Additional helper methods would be implemented here...
// This provides the core structure for the TranscriptionService

void TranscriptionService::initializeModelInfo() {
    QMutexLocker locker(&m_modelsMutex);
    
    m_modelSizes[TranscriptionProvider::WhisperCppTiny] = WHISPER_TINY_SIZE;
    m_modelSizes[TranscriptionProvider::WhisperCppBase] = WHISPER_BASE_SIZE;
    m_modelSizes[TranscriptionProvider::WhisperCppSmall] = WHISPER_SMALL_SIZE;
    m_modelSizes[TranscriptionProvider::WhisperCppMedium] = WHISPER_MEDIUM_SIZE;
    m_modelSizes[TranscriptionProvider::WhisperCppLarge] = WHISPER_LARGE_SIZE;
}

QString TranscriptionService::getModelFileName(TranscriptionProvider provider) const {
    switch (provider) {
        case TranscriptionProvider::WhisperCppTiny:
            return "ggml-tiny.bin";
        case TranscriptionProvider::WhisperCppBase:
            return "ggml-base.bin";
        case TranscriptionProvider::WhisperCppSmall:
            return "ggml-small.bin";
        case TranscriptionProvider::WhisperCppMedium:
            return "ggml-medium.bin";
        case TranscriptionProvider::WhisperCppLarge:
            return "ggml-large.bin";
        default:
            return QString();
    }
}

QString TranscriptionService::getModelsDirectory() const {
    return QDir(QCoreApplication::applicationDirPath()).absoluteFilePath("models/whisper");
}

bool TranscriptionService::createModelsDirectory() const {
    QString modelsDir = getModelsDirectory();
    return QDir().mkpath(modelsDir);
}

bool TranscriptionService::validateModelFile(const QString& modelPath) const {
    QFileInfo fileInfo(modelPath);
    return fileInfo.exists() && fileInfo.isReadable() && fileInfo.size() > 0;
}

TranscriptionResult TranscriptionService::processWithWhisper(const TranscriptionRequest& request, const QString& requestId) {
    // This is a simplified implementation
    // Full implementation would include actual whisper.cpp processing
    TranscriptionResult result;
    result.id = requestId;
    result.provider = request.preferredProvider;
    result.confidence = 0.95;
    result.language = request.language.isEmpty() ? m_defaultLanguage : request.language;
    result.processingTime = 1000; // Placeholder
    result.text = "Sample transcription text"; // Placeholder
    
    return result;
}

void TranscriptionService::setError(TranscriptionError error, const QString& errorMessage) {
    m_lastError = error;
    m_errorString = errorMessage;
    qWarning() << "TranscriptionService error:" << errorMessage;
}

void TranscriptionService::setRequestStatus(const QString& requestId, TranscriptionStatus status) {
    if (m_activeRequests.contains(requestId)) {
        m_activeRequests[requestId].status = status;
    }
}

void TranscriptionService::setRequestResult(const QString& requestId, const TranscriptionResult& result) {
    if (m_activeRequests.contains(requestId)) {
        m_activeRequests[requestId].result = result;
        m_activeRequests[requestId].hasResult = true;
    }
}

void TranscriptionService::processNextPendingRequest() {
    QMutexLocker locker(&m_requestsMutex);
    
    if (!m_pendingRequests.isEmpty() && 
        m_activeRequests.size() <= m_maxConcurrentRequests) {
        
        QString requestId = m_pendingRequests.dequeue();
        if (m_activeRequests.contains(requestId)) {
            const TranscriptionRequest& request = m_activeRequests[requestId].request;
            setRequestStatus(requestId, TranscriptionStatus::Processing);
            
            // Process the request
            QTimer::singleShot(0, this, [this, request, requestId]() {
                TranscriptionResult result = processWithWhisper(request, requestId);
                if (result.id.isEmpty()) {
                    handleTaskFailed(requestId, m_lastError, m_errorString);
                } else {
                    handleTaskCompleted(requestId, result);
                }
            });
        }
    }
}

void TranscriptionService::downloadModelAsync(TranscriptionProvider model) {
    // Placeholder for model download implementation
    // In a full implementation, this would download models from appropriate sources
    emit modelDownloadCompleted(model);
}

QString TranscriptionService::detectLanguageFromAudio(const QString& audioPath) {
    Q_UNUSED(audioPath)
    // Placeholder - would use whisper.cpp language detection
    return m_defaultLanguage;
}

// Helper function to convert provider enum to string
QString transcriptionProviderToString(TranscriptionProvider provider) {
    switch (provider) {
        case TranscriptionProvider::WhisperCppTiny: return "WhisperCpp-Tiny";
        case TranscriptionProvider::WhisperCppBase: return "WhisperCpp-Base";
        case TranscriptionProvider::WhisperCppSmall: return "WhisperCpp-Small";
        case TranscriptionProvider::WhisperCppMedium: return "WhisperCpp-Medium";
        case TranscriptionProvider::WhisperCppLarge: return "WhisperCpp-Large";
        default: return "Unknown";
    }
}

// Storage management methods
void TranscriptionService::setStorageManager(IStorageManager* storageManager) {
    m_storageManager = storageManager;
}

IStorageManager* TranscriptionService::getStorageManager() const {
    return m_storageManager;
}

void TranscriptionService::saveTranscriptionToStorage(const QString& requestId, const TranscriptionResult& result) {
    if (!m_storageManager) {
        return;
    }

    auto* transcriptionStorage = m_storageManager->getTranscriptionStorage();
    if (!transcriptionStorage) {
        return;
    }

    // Get recording ID from the request context
    QString recordingId;
    {
        QMutexLocker locker(&m_requestsMutex);
        if (m_activeRequests.contains(requestId)) {
            const TranscriptionRequest& request = m_activeRequests[requestId].request;
            // Extract recording ID from audio file path or use a default approach
            QFileInfo fileInfo(request.audioFilePath);
            recordingId = fileInfo.baseName(); // Use filename as recording ID for now
        }
    }
    
    if (recordingId.isEmpty()) {
        return; // Cannot save without recording ID
    }
    
    // Create transcription model from result
    Transcription transcription(recordingId, result.text);
    // Note: Transcription ID is auto-generated in constructor
    transcription.setConfidence(result.confidence);
    transcription.setProvider(transcriptionProviderToString(result.provider));
    transcription.setLanguage(result.language);
    transcription.setProcessingTime(result.processingTime);
    transcription.setStatus(TranscriptionStatus::Completed);
    transcription.setCreatedAt(QDateTime::currentDateTime());

    // Word timestamps are already in JSON format
    transcription.setWordTimestamps(result.wordTimestamps);

    QString savedId = transcriptionStorage->saveTranscription(transcription);
    qDebug() << "Saved transcription to storage with ID:" << savedId;
}

void TranscriptionService::updateTranscriptionInStorage(const QString& transcriptionId, const TranscriptionResult& result) {
    if (!m_storageManager || transcriptionId.isEmpty()) {
        return;
    }

    auto* transcriptionStorage = m_storageManager->getTranscriptionStorage();
    if (!transcriptionStorage) {
        return;
    }

    // Get existing transcription and update it
    Transcription transcription = transcriptionStorage->getTranscription(transcriptionId);
    if (transcription.isValid()) {
        transcription.setText(result.text);
        transcription.setConfidence(result.confidence);
        transcription.setProcessingTime(result.processingTime);

        // Update word timestamps (already in JSON format)
        transcription.setWordTimestamps(result.wordTimestamps);

        transcriptionStorage->updateTranscription(transcription);
        qDebug() << "Updated transcription in storage:" << transcriptionId;
    }
}
