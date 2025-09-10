#include "Transcription.h"
#include <QJsonObject>
#include <QRegularExpression>
#include <QStringList>

Transcription::Transcription()
    : m_id(generateUuid())
    , m_recordingId("")
    , m_text("")
    , m_confidence(0.0)
    , m_provider("")
    , m_language("en-US")
    , m_processingTime(0)
    , m_wordTimestamps(QJsonArray())
    , m_createdAt(QDateTime::currentDateTime())
    , m_status(TranscriptionStatus::Pending)
{
}

Transcription::Transcription(const QString& recordingId, const QString& text)
    : m_id(generateUuid())
    , m_recordingId(recordingId)
    , m_text(text)
    , m_confidence(0.0)
    , m_provider("")
    , m_language("en-US")
    , m_processingTime(0)
    , m_wordTimestamps(QJsonArray())
    , m_createdAt(QDateTime::currentDateTime())
    , m_status(TranscriptionStatus::Pending)
{
}

Transcription::Transcription(const QJsonObject& json)
    : Transcription()
{
    fromJson(json);
}

Transcription::Transcription(const Transcription& other)
    : m_id(other.m_id)
    , m_recordingId(other.m_recordingId)
    , m_text(other.m_text)
    , m_confidence(other.m_confidence)
    , m_provider(other.m_provider)
    , m_language(other.m_language)
    , m_processingTime(other.m_processingTime)
    , m_wordTimestamps(other.m_wordTimestamps)
    , m_createdAt(other.m_createdAt)
    , m_status(other.m_status)
{
}

Transcription& Transcription::operator=(const Transcription& other) {
    if (this != &other) {
        m_id = other.m_id;
        m_recordingId = other.m_recordingId;
        m_text = other.m_text;
        m_confidence = other.m_confidence;
        m_provider = other.m_provider;
        m_language = other.m_language;
        m_processingTime = other.m_processingTime;
        m_wordTimestamps = other.m_wordTimestamps;
        m_createdAt = other.m_createdAt;
        m_status = other.m_status;
    }
    return *this;
}

QString Transcription::getId() const {
    return m_id;
}

QJsonObject Transcription::toJson() const {
    QJsonObject json;
    json["id"] = m_id;
    json["recordingId"] = m_recordingId;
    json["text"] = m_text;
    json["confidence"] = m_confidence;
    json["provider"] = m_provider;
    json["language"] = m_language;
    json["processingTime"] = static_cast<qint64>(m_processingTime);
    json["wordTimestamps"] = m_wordTimestamps;
    json["createdAt"] = m_createdAt.toString(Qt::ISODate);
    json["status"] = transcriptionStatusToString(m_status);
    return json;
}

bool Transcription::fromJson(const QJsonObject& json) {
    if (!json.contains("id") || !isValidUuid(json["id"].toString())) {
        return false;
    }
    
    m_id = json["id"].toString();
    m_recordingId = json.value("recordingId").toString();
    m_text = json.value("text").toString();
    m_confidence = json.value("confidence").toDouble();
    m_provider = json.value("provider").toString();
    m_language = json.value("language").toString("en-US");
    m_processingTime = json.value("processingTime").toVariant().toLongLong();
    m_wordTimestamps = json.value("wordTimestamps").toArray();
    
    QString createdAtStr = json.value("createdAt").toString();
    m_createdAt = QDateTime::fromString(createdAtStr, Qt::ISODate);
    if (!m_createdAt.isValid()) {
        m_createdAt = QDateTime::currentDateTime();
    }
    
    QString statusStr = json.value("status").toString("Pending");
    m_status = transcriptionStatusFromString(statusStr);
    
    return true;
}

bool Transcription::isValid() const {
    // ID must be valid UUID
    if (!isValidUuid(m_id)) {
        return false;
    }
    
    // Recording ID must be valid UUID
    if (!isValidUuid(m_recordingId)) {
        return false;
    }
    
    // Text validation depends on status
    if (m_status == TranscriptionStatus::Completed && m_text.isEmpty()) {
        return false;
    }
    
    // Confidence validation
    if (!validateConfidence()) {
        return false;
    }
    
    // Processing time validation
    if (!validateProcessingTime()) {
        return false;
    }
    
    // Provider validation for completed transcriptions
    if (m_status == TranscriptionStatus::Completed && !validateProvider()) {
        return false;
    }
    
    // Language code validation
    if (!isValidLanguageCode(m_language)) {
        return false;
    }
    
    // Created at must be valid
    if (!m_createdAt.isValid()) {
        return false;
    }
    
    return true;
}

void Transcription::setRecordingId(const QString& recordingId) {
    m_recordingId = recordingId;
}

void Transcription::setText(const QString& text) {
    m_text = text;
}

void Transcription::setConfidence(double confidence) {
    m_confidence = qBound(0.0, confidence, 1.0);
}

void Transcription::setProvider(const QString& provider) {
    m_provider = provider;
}

void Transcription::setLanguage(const QString& language) {
    m_language = language;
}

void Transcription::setProcessingTime(qint64 processingTime) {
    m_processingTime = processingTime;
}

void Transcription::setWordTimestamps(const QJsonArray& wordTimestamps) {
    m_wordTimestamps = wordTimestamps;
}

void Transcription::setCreatedAt(const QDateTime& createdAt) {
    m_createdAt = createdAt;
}

void Transcription::setStatus(TranscriptionStatus status) {
    m_status = status;
}

int Transcription::getWordCount() const {
    if (m_text.isEmpty()) {
        return 0;
    }
    
    return m_text.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts).size();
}

int Transcription::getCharacterCount() const {
    return m_text.length();
}

double Transcription::getConfidencePercentage() const {
    return m_confidence * 100.0;
}

QString Transcription::getConfidenceLevel() const {
    if (m_confidence >= 0.8) {
        return "High";
    } else if (m_confidence >= 0.6) {
        return "Medium";
    } else {
        return "Low";
    }
}

QStringList Transcription::getSupportedProviders() const {
    return QStringList{
        "whisper-cpp-tiny",
        "whisper-cpp-base", 
        "whisper-cpp-small",
        "whisper-cpp-medium",
        "whisper-cpp-large"
    };
}

bool Transcription::hasWordTimestamps() const {
    return !m_wordTimestamps.isEmpty();
}

QString Transcription::getFormattedProcessingTime() const {
    if (m_processingTime < 1000) {
        return QString("%1ms").arg(m_processingTime);
    } else if (m_processingTime < 60000) {
        return QString("%1.%2s").arg(m_processingTime / 1000).arg((m_processingTime % 1000) / 100);
    } else {
        qint64 minutes = m_processingTime / 60000;
        qint64 seconds = (m_processingTime % 60000) / 1000;
        return QString("%1m %2s").arg(minutes).arg(seconds);
    }
}

QString Transcription::getDisplayText(int maxLength) const {
    if (maxLength <= 0 || m_text.length() <= maxLength) {
        return m_text;
    }
    
    return m_text.left(maxLength - 3) + "...";
}

QStringList Transcription::extractWords() const {
    if (m_text.isEmpty()) {
        return QStringList();
    }
    
    return m_text.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
}

QStringList Transcription::extractSentences() const {
    if (m_text.isEmpty()) {
        return QStringList();
    }
    
    // Split by sentence endings, keeping the delimiter
    QRegularExpression sentencePattern("[.!?]+");
    QStringList sentences = m_text.split(sentencePattern, Qt::SkipEmptyParts);
    
    // Clean up and filter out very short fragments
    QStringList cleanSentences;
    for (const QString& sentence : sentences) {
        QString cleaned = sentence.trimmed();
        if (!cleaned.isEmpty() && cleaned.length() > 3) {
            cleanSentences.append(cleaned);
        }
    }
    
    return cleanSentences;
}

QString Transcription::getFirstSentence() const {
    QStringList sentences = extractSentences();
    return sentences.isEmpty() ? "" : sentences.first();
}

QString Transcription::getSummaryPreview(int maxWords) const {
    QStringList words = extractWords();
    if (words.size() <= maxWords) {
        return m_text;
    }
    
    return words.mid(0, maxWords).join(" ") + "...";
}

bool Transcription::canRetry() const {
    return m_status == TranscriptionStatus::Failed || m_status == TranscriptionStatus::Cancelled;
}

bool Transcription::canEdit() const {
    return m_status == TranscriptionStatus::Completed;
}

bool Transcription::canEnhance() const {
    return m_status == TranscriptionStatus::Completed && !m_text.isEmpty();
}

bool Transcription::isProcessing() const {
    return m_status == TranscriptionStatus::Processing;
}

bool Transcription::isCompleted() const {
    return m_status == TranscriptionStatus::Completed;
}

bool Transcription::hasFailed() const {
    return m_status == TranscriptionStatus::Failed;
}

QJsonObject Transcription::WordTimestamp::toJson() const {
    QJsonObject json;
    json["word"] = word;
    json["startTime"] = startTime;
    json["endTime"] = endTime;
    json["confidence"] = confidence;
    return json;
}

Transcription::WordTimestamp Transcription::WordTimestamp::fromJson(const QJsonObject& json) {
    WordTimestamp timestamp;
    timestamp.word = json.value("word").toString();
    timestamp.startTime = json.value("startTime").toDouble();
    timestamp.endTime = json.value("endTime").toDouble();
    timestamp.confidence = json.value("confidence").toDouble();
    return timestamp;
}

QList<Transcription::WordTimestamp> Transcription::getWordTimestampList() const {
    QList<WordTimestamp> timestamps;
    
    for (const QJsonValue& value : m_wordTimestamps) {
        if (value.isObject()) {
            timestamps.append(WordTimestamp::fromJson(value.toObject()));
        }
    }
    
    return timestamps;
}

void Transcription::setWordTimestampList(const QList<WordTimestamp>& timestamps) {
    QJsonArray jsonArray;
    
    for (const WordTimestamp& timestamp : timestamps) {
        jsonArray.append(timestamp.toJson());
    }
    
    m_wordTimestamps = jsonArray;
}

bool Transcription::operator==(const Transcription& other) const {
    return m_id == other.m_id;
}

bool Transcription::operator!=(const Transcription& other) const {
    return !(*this == other);
}

bool Transcription::validateConfidence() const {
    return m_confidence >= 0.0 && m_confidence <= 1.0;
}

bool Transcription::validateProcessingTime() const {
    // Processing time should be positive for completed transcriptions
    if (m_status == TranscriptionStatus::Completed) {
        return m_processingTime > 0;
    }
    
    return m_processingTime >= 0;
}

bool Transcription::validateProvider() const {
    if (m_provider.isEmpty()) {
        return false;
    }
    
    // Check if provider is in the list of supported providers
    return getSupportedProviders().contains(m_provider);
}

bool Transcription::validateWordTimestamps() const {
    // Word timestamps are optional, so empty array is valid
    if (m_wordTimestamps.isEmpty()) {
        return true;
    }
    
    // If present, each timestamp should be a valid object
    for (const QJsonValue& value : m_wordTimestamps) {
        if (!value.isObject()) {
            return false;
        }
        
        QJsonObject obj = value.toObject();
        if (!obj.contains("word") || !obj.contains("startTime") || !obj.contains("endTime")) {
            return false;
        }
        
        double startTime = obj["startTime"].toDouble();
        double endTime = obj["endTime"].toDouble();
        
        if (startTime < 0 || endTime < startTime) {
            return false;
        }
    }
    
    return true;
}
