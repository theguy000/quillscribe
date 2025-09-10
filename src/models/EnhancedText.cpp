#include "EnhancedText.h"
#include <QJsonObject>
#include <QRegularExpression>
#include <QStringList>
#include <QtMath>

EnhancedText::EnhancedText()
    : m_id(generateUuid())
    , m_transcriptionId("")
    , m_originalText("")
    , m_enhancedText("")
    , m_enhancementMode(EnhancementMode::GrammarOnly)
    , m_provider("")
    , m_promptTemplate("")
    , m_processingTime(0)
    , m_settings(QJsonObject())
    , m_createdAt(QDateTime::currentDateTime())
    , m_userRating(0)
{
}

EnhancedText::EnhancedText(const QString& transcriptionId, const QString& originalText, const QString& enhancedText)
    : m_id(generateUuid())
    , m_transcriptionId(transcriptionId)
    , m_originalText(originalText)
    , m_enhancedText(enhancedText)
    , m_enhancementMode(EnhancementMode::GrammarOnly)
    , m_provider("")
    , m_promptTemplate("")
    , m_processingTime(0)
    , m_settings(QJsonObject())
    , m_createdAt(QDateTime::currentDateTime())
    , m_userRating(0)
{
}

EnhancedText::EnhancedText(const QJsonObject& json)
    : EnhancedText()
{
    fromJson(json);
}

EnhancedText::EnhancedText(const EnhancedText& other)
    : m_id(other.m_id)
    , m_transcriptionId(other.m_transcriptionId)
    , m_originalText(other.m_originalText)
    , m_enhancedText(other.m_enhancedText)
    , m_enhancementMode(other.m_enhancementMode)
    , m_provider(other.m_provider)
    , m_promptTemplate(other.m_promptTemplate)
    , m_processingTime(other.m_processingTime)
    , m_settings(other.m_settings)
    , m_createdAt(other.m_createdAt)
    , m_userRating(other.m_userRating)
{
}

EnhancedText& EnhancedText::operator=(const EnhancedText& other) {
    if (this != &other) {
        m_id = other.m_id;
        m_transcriptionId = other.m_transcriptionId;
        m_originalText = other.m_originalText;
        m_enhancedText = other.m_enhancedText;
        m_enhancementMode = other.m_enhancementMode;
        m_provider = other.m_provider;
        m_promptTemplate = other.m_promptTemplate;
        m_processingTime = other.m_processingTime;
        m_settings = other.m_settings;
        m_createdAt = other.m_createdAt;
        m_userRating = other.m_userRating;
    }
    return *this;
}

QString EnhancedText::getId() const {
    return m_id;
}

QJsonObject EnhancedText::toJson() const {
    QJsonObject json;
    json["id"] = m_id;
    json["transcriptionId"] = m_transcriptionId;
    json["originalText"] = m_originalText;
    json["enhancedText"] = m_enhancedText;
    json["enhancementMode"] = enhancementModeToString(m_enhancementMode);
    json["provider"] = m_provider;
    json["promptTemplate"] = m_promptTemplate;
    json["processingTime"] = static_cast<qint64>(m_processingTime);
    json["settings"] = m_settings;
    json["createdAt"] = m_createdAt.toString(Qt::ISODate);
    json["userRating"] = m_userRating;
    return json;
}

bool EnhancedText::fromJson(const QJsonObject& json) {
    if (!json.contains("id") || !isValidUuid(json["id"].toString())) {
        return false;
    }
    
    m_id = json["id"].toString();
    m_transcriptionId = json.value("transcriptionId").toString();
    m_originalText = json.value("originalText").toString();
    m_enhancedText = json.value("enhancedText").toString();
    
    QString enhancementModeStr = json.value("enhancementMode").toString("GrammarOnly");
    m_enhancementMode = enhancementModeFromString(enhancementModeStr);
    
    m_provider = json.value("provider").toString();
    m_promptTemplate = json.value("promptTemplate").toString();
    m_processingTime = json.value("processingTime").toVariant().toLongLong();
    m_settings = json.value("settings").toObject();
    
    QString createdAtStr = json.value("createdAt").toString();
    m_createdAt = QDateTime::fromString(createdAtStr, Qt::ISODate);
    if (!m_createdAt.isValid()) {
        m_createdAt = QDateTime::currentDateTime();
    }
    
    m_userRating = json.value("userRating").toInt(0);
    
    return true;
}

bool EnhancedText::isValid() const {
    // ID must be valid UUID
    if (!isValidUuid(m_id)) {
        return false;
    }
    
    // Transcription ID must be valid UUID
    if (!isValidUuid(m_transcriptionId)) {
        return false;
    }
    
    // Enhanced text must not be empty
    if (m_enhancedText.isEmpty()) {
        return false;
    }
    
    // Enhancement mode validation
    if (!validateEnhancementMode()) {
        return false;
    }
    
    // Processing time validation
    if (!validateProcessingTime()) {
        return false;
    }
    
    // User rating validation
    if (!validateUserRating()) {
        return false;
    }
    
    // Created at must be valid
    if (!m_createdAt.isValid()) {
        return false;
    }
    
    return true;
}

void EnhancedText::setTranscriptionId(const QString& transcriptionId) {
    m_transcriptionId = transcriptionId;
}

void EnhancedText::setOriginalText(const QString& originalText) {
    m_originalText = originalText;
}

void EnhancedText::setEnhancedText(const QString& enhancedText) {
    m_enhancedText = enhancedText;
}

void EnhancedText::setEnhancementMode(EnhancementMode mode) {
    m_enhancementMode = mode;
}

void EnhancedText::setProvider(const QString& provider) {
    m_provider = provider;
}

void EnhancedText::setPromptTemplate(const QString& promptTemplate) {
    m_promptTemplate = promptTemplate;
}

void EnhancedText::setProcessingTime(qint64 processingTime) {
    m_processingTime = processingTime;
}

void EnhancedText::setSettings(const QJsonObject& settings) {
    m_settings = settings;
}

void EnhancedText::setCreatedAt(const QDateTime& createdAt) {
    m_createdAt = createdAt;
}

void EnhancedText::setUserRating(int rating) {
    m_userRating = qBound(0, rating, 5);
}

int EnhancedText::getOriginalWordCount() const {
    if (m_originalText.isEmpty()) {
        return 0;
    }
    return m_originalText.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts).size();
}

int EnhancedText::getEnhancedWordCount() const {
    if (m_enhancedText.isEmpty()) {
        return 0;
    }
    return m_enhancedText.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts).size();
}

int EnhancedText::getOriginalCharacterCount() const {
    return m_originalText.length();
}

int EnhancedText::getEnhancedCharacterCount() const {
    return m_enhancedText.length();
}

double EnhancedText::getCompressionRatio() const {
    int originalLength = getOriginalCharacterCount();
    if (originalLength == 0) {
        return 0.0;
    }
    
    int enhancedLength = getEnhancedCharacterCount();
    return static_cast<double>(enhancedLength) / static_cast<double>(originalLength);
}

QString EnhancedText::getFormattedProcessingTime() const {
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

QString EnhancedText::getEnhancementModeDisplayName() const {
    switch (m_enhancementMode) {
        case EnhancementMode::GrammarOnly:
            return "Grammar Correction";
        case EnhancementMode::StyleImprovement:
            return "Style Enhancement";
        case EnhancementMode::Summarization:
            return "Summarization";
        case EnhancementMode::Formalization:
            return "Formalization";
        case EnhancementMode::Custom:
            return "Custom Enhancement";
        default:
            return "Unknown";
    }
}

QString EnhancedText::getProviderDisplayName() const {
    if (m_provider.startsWith("gemini-pro")) {
        return "Gemini Pro";
    } else if (m_provider.startsWith("gemini-flash")) {
        return "Gemini Flash";
    } else if (m_provider.startsWith("gemini")) {
        return "Google Gemini";
    }
    
    return m_provider.isEmpty() ? "Unknown Provider" : m_provider;
}

QStringList EnhancedText::getSupportedProviders() const {
    return QStringList{
        "gemini-pro",
        "gemini-flash",
        "gemini-pro-vision"
    };
}

bool EnhancedText::hasSignificantChanges() const {
    if (m_originalText.isEmpty() || m_enhancedText.isEmpty()) {
        return false;
    }
    
    // Consider changes significant if similarity is less than 80%
    return calculateSimilarity() < 0.8;
}

double EnhancedText::calculateSimilarity() const {
    if (m_originalText.isEmpty() && m_enhancedText.isEmpty()) {
        return 1.0;
    }
    
    if (m_originalText.isEmpty() || m_enhancedText.isEmpty()) {
        return 0.0;
    }
    
    // Simple similarity based on Levenshtein distance
    double distance = calculateLevenshteinDistance(m_originalText, m_enhancedText);
    int maxLength = qMax(m_originalText.length(), m_enhancedText.length());
    
    return 1.0 - (distance / maxLength);
}

QStringList EnhancedText::getAddedWords() const {
    QStringList originalWords = tokenizeText(m_originalText);
    QStringList enhancedWords = tokenizeText(m_enhancedText);
    QStringList added;
    
    for (const QString& word : enhancedWords) {
        if (!originalWords.contains(word, Qt::CaseInsensitive)) {
            added.append(word);
        }
    }
    
    return added;
}

QStringList EnhancedText::getRemovedWords() const {
    QStringList originalWords = tokenizeText(m_originalText);
    QStringList enhancedWords = tokenizeText(m_enhancedText);
    QStringList removed;
    
    for (const QString& word : originalWords) {
        if (!enhancedWords.contains(word, Qt::CaseInsensitive)) {
            removed.append(word);
        }
    }
    
    return removed;
}

QString EnhancedText::getDiffSummary() const {
    QStringList added = getAddedWords();
    QStringList removed = getRemovedWords();
    
    QStringList summary;
    
    if (!added.isEmpty()) {
        summary.append(QString("Added %1 words").arg(added.size()));
    }
    
    if (!removed.isEmpty()) {
        summary.append(QString("Removed %1 words").arg(removed.size()));
    }
    
    if (summary.isEmpty()) {
        return "Minor text improvements";
    }
    
    return summary.join(", ");
}

QString EnhancedText::getDisplayPreview(int maxLength) const {
    if (maxLength <= 0 || m_enhancedText.length() <= maxLength) {
        return m_enhancedText;
    }
    
    return m_enhancedText.left(maxLength - 3) + "...";
}

void EnhancedText::setMaxWordCount(int maxWords) {
    QJsonObject settings = m_settings;
    settings["maxWordCount"] = maxWords;
    m_settings = settings;
}

void EnhancedText::setPreserveFormatting(bool preserve) {
    QJsonObject settings = m_settings;
    settings["preserveFormatting"] = preserve;
    m_settings = settings;
}

void EnhancedText::setCustomPrompt(const QString& prompt) {
    QJsonObject settings = m_settings;
    settings["customPrompt"] = prompt;
    m_settings = settings;
}

void EnhancedText::setTemperature(double temperature) {
    QJsonObject settings = m_settings;
    settings["temperature"] = temperature;
    m_settings = settings;
}

int EnhancedText::getMaxWordCount() const {
    return m_settings.value("maxWordCount").toInt(500);
}

bool EnhancedText::getPreserveFormatting() const {
    return m_settings.value("preserveFormatting").toBool(true);
}

QString EnhancedText::getCustomPrompt() const {
    return m_settings.value("customPrompt").toString();
}

double EnhancedText::getTemperature() const {
    return m_settings.value("temperature").toDouble(0.7);
}

bool EnhancedText::hasUserRating() const {
    return m_userRating > 0;
}

QString EnhancedText::getRatingText() const {
    switch (m_userRating) {
        case 5: return "Excellent";
        case 4: return "Good";
        case 3: return "Average";
        case 2: return "Poor";
        case 1: return "Very Poor";
        default: return "Not Rated";
    }
}

void EnhancedText::clearUserRating() {
    m_userRating = 0;
}

bool EnhancedText::isEnhancementValid() const {
    return !m_enhancedText.isEmpty() && m_enhancedText != m_originalText;
}

bool EnhancedText::exceedsWordLimit(int maxWords) const {
    return getEnhancedWordCount() > maxWords;
}

bool EnhancedText::hasProperFormatting() const {
    // Basic formatting checks
    if (m_enhancedText.isEmpty()) {
        return false;
    }
    
    // Check if text starts with capital letter
    if (!m_enhancedText.at(0).isUpper()) {
        return false;
    }
    
    // Check if text ends with proper punctuation
    QChar lastChar = m_enhancedText.at(m_enhancedText.length() - 1);
    if (!QString(".!?").contains(lastChar)) {
        return false;
    }
    
    return true;
}

QString EnhancedText::getPlainText() const {
    return m_enhancedText;
}

QString EnhancedText::getMarkdownText() const {
    return QString("## Enhanced Text\n\n%1").arg(m_enhancedText);
}

QString EnhancedText::getComparisonText() const {
    return QString("**Original:**\n%1\n\n**Enhanced:**\n%2").arg(m_originalText, m_enhancedText);
}

bool EnhancedText::operator==(const EnhancedText& other) const {
    return m_id == other.m_id;
}

bool EnhancedText::operator!=(const EnhancedText& other) const {
    return !(*this == other);
}

bool EnhancedText::validateProcessingTime() const {
    return m_processingTime >= 0;
}

bool EnhancedText::validateProvider() const {
    if (m_provider.isEmpty()) {
        return true; // Provider can be empty during creation
    }
    
    // Check if provider is supported
    return getSupportedProviders().contains(m_provider) || 
           m_provider.startsWith("gemini");
}

bool EnhancedText::validateUserRating() const {
    return m_userRating >= 0 && m_userRating <= 5;
}

bool EnhancedText::validateEnhancementMode() const {
    // All enum values are valid
    return true;
}

QStringList EnhancedText::tokenizeText(const QString& text) const {
    if (text.isEmpty()) {
        return QStringList();
    }
    
    // Remove punctuation and split by whitespace
    QString cleanText = text;
    cleanText.remove(QRegularExpression("[^\\w\\s]"));
    
    return cleanText.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
}

double EnhancedText::calculateLevenshteinDistance(const QString& text1, const QString& text2) const {
    const int len1 = text1.length();
    const int len2 = text2.length();
    
    if (len1 == 0) return len2;
    if (len2 == 0) return len1;
    
    // Create matrix
    QVector<QVector<int>> matrix(len1 + 1, QVector<int>(len2 + 1));
    
    // Initialize first row and column
    for (int i = 0; i <= len1; ++i) {
        matrix[i][0] = i;
    }
    for (int j = 0; j <= len2; ++j) {
        matrix[0][j] = j;
    }
    
    // Calculate distances
    for (int i = 1; i <= len1; ++i) {
        for (int j = 1; j <= len2; ++j) {
            int cost = (text1[i-1] == text2[j-1]) ? 0 : 1;
            
            matrix[i][j] = qMin(qMin(matrix[i-1][j] + 1, matrix[i][j-1] + 1), matrix[i-1][j-1] + cost);
        }
    }
    
    return matrix[len1][len2];
}
