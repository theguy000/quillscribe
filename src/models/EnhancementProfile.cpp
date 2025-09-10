#include "EnhancementProfile.h"
#include <QJsonObject>

EnhancementProfile::EnhancementProfile()
    : m_id(generateUuid())
    , m_name("Default Profile")
    , m_defaultMode(EnhancementMode::GrammarOnly)
    , m_customPrompt("")
    , m_provider("gemini-flash")
    , m_maxWordCount(500)
    , m_autoEnhance(false)
    , m_preserveFormatting(true)
    , m_isDefault(false)
    , m_createdAt(QDateTime::currentDateTime())
    , m_lastUsed(QDateTime())
{
}

EnhancementProfile::EnhancementProfile(const QString& name)
    : m_id(generateUuid())
    , m_name(name)
    , m_defaultMode(EnhancementMode::GrammarOnly)
    , m_customPrompt("")
    , m_provider("gemini-flash")
    , m_maxWordCount(500)
    , m_autoEnhance(false)
    , m_preserveFormatting(true)
    , m_isDefault(false)
    , m_createdAt(QDateTime::currentDateTime())
    , m_lastUsed(QDateTime())
{
}

EnhancementProfile::EnhancementProfile(const QJsonObject& json)
    : EnhancementProfile()
{
    fromJson(json);
}

EnhancementProfile::EnhancementProfile(const EnhancementProfile& other)
    : m_id(other.m_id)
    , m_name(other.m_name)
    , m_defaultMode(other.m_defaultMode)
    , m_customPrompt(other.m_customPrompt)
    , m_provider(other.m_provider)
    , m_maxWordCount(other.m_maxWordCount)
    , m_autoEnhance(other.m_autoEnhance)
    , m_preserveFormatting(other.m_preserveFormatting)
    , m_isDefault(other.m_isDefault)
    , m_createdAt(other.m_createdAt)
    , m_lastUsed(other.m_lastUsed)
{
}

EnhancementProfile& EnhancementProfile::operator=(const EnhancementProfile& other) {
    if (this != &other) {
        m_id = other.m_id;
        m_name = other.m_name;
        m_defaultMode = other.m_defaultMode;
        m_customPrompt = other.m_customPrompt;
        m_provider = other.m_provider;
        m_maxWordCount = other.m_maxWordCount;
        m_autoEnhance = other.m_autoEnhance;
        m_preserveFormatting = other.m_preserveFormatting;
        m_isDefault = other.m_isDefault;
        m_createdAt = other.m_createdAt;
        m_lastUsed = other.m_lastUsed;
    }
    return *this;
}

QString EnhancementProfile::getId() const {
    return m_id;
}

QJsonObject EnhancementProfile::toJson() const {
    QJsonObject json;
    json["id"] = m_id;
    json["name"] = m_name;
    json["defaultMode"] = enhancementModeToString(m_defaultMode);
    json["customPrompt"] = m_customPrompt;
    json["provider"] = m_provider;
    json["maxWordCount"] = m_maxWordCount;
    json["autoEnhance"] = m_autoEnhance;
    json["preserveFormatting"] = m_preserveFormatting;
    json["isDefault"] = m_isDefault;
    json["createdAt"] = m_createdAt.toString(Qt::ISODate);
    json["lastUsed"] = m_lastUsed.isValid() ? m_lastUsed.toString(Qt::ISODate) : QString();
    return json;
}

bool EnhancementProfile::fromJson(const QJsonObject& json) {
    if (!json.contains("id") || !isValidUuid(json["id"].toString())) {
        return false;
    }
    
    m_id = json["id"].toString();
    m_name = json.value("name").toString("Default Profile");
    
    QString modeStr = json.value("defaultMode").toString("GrammarOnly");
    m_defaultMode = enhancementModeFromString(modeStr);
    
    m_customPrompt = json.value("customPrompt").toString();
    m_provider = json.value("provider").toString("gemini-flash");
    m_maxWordCount = json.value("maxWordCount").toInt(500);
    m_autoEnhance = json.value("autoEnhance").toBool(false);
    m_preserveFormatting = json.value("preserveFormatting").toBool(true);
    m_isDefault = json.value("isDefault").toBool(false);
    
    QString createdAtStr = json.value("createdAt").toString();
    m_createdAt = QDateTime::fromString(createdAtStr, Qt::ISODate);
    if (!m_createdAt.isValid()) {
        m_createdAt = QDateTime::currentDateTime();
    }
    
    QString lastUsedStr = json.value("lastUsed").toString();
    if (!lastUsedStr.isEmpty()) {
        m_lastUsed = QDateTime::fromString(lastUsedStr, Qt::ISODate);
    }
    
    return true;
}

bool EnhancementProfile::isValid() const {
    if (!isValidUuid(m_id)) {
        return false;
    }
    
    if (!validateName()) {
        return false;
    }
    
    if (!validateWordCount()) {
        return false;
    }
    
    if (!validateCustomPrompt()) {
        return false;
    }
    
    if (!m_createdAt.isValid()) {
        return false;
    }
    
    return true;
}

void EnhancementProfile::setName(const QString& name) {
    m_name = name.trimmed();
}

void EnhancementProfile::setDefaultMode(EnhancementMode mode) {
    m_defaultMode = mode;
}

void EnhancementProfile::setCustomPrompt(const QString& prompt) {
    m_customPrompt = prompt;
}

void EnhancementProfile::setProvider(const QString& provider) {
    m_provider = provider;
}

void EnhancementProfile::setMaxWordCount(int maxWords) {
    m_maxWordCount = qBound(1, maxWords, 10000);
}

void EnhancementProfile::setAutoEnhance(bool autoEnhance) {
    m_autoEnhance = autoEnhance;
}

void EnhancementProfile::setPreserveFormatting(bool preserve) {
    m_preserveFormatting = preserve;
}

void EnhancementProfile::setIsDefault(bool isDefault) {
    m_isDefault = isDefault;
}

void EnhancementProfile::setCreatedAt(const QDateTime& createdAt) {
    m_createdAt = createdAt;
}

void EnhancementProfile::setLastUsed(const QDateTime& lastUsed) {
    m_lastUsed = lastUsed;
}

void EnhancementProfile::markAsUsed() {
    m_lastUsed = QDateTime::currentDateTime();
}

void EnhancementProfile::makeDefault() {
    m_isDefault = true;
}

void EnhancementProfile::resetToDefaults() {
    m_defaultMode = EnhancementMode::GrammarOnly;
    m_customPrompt = "";
    m_provider = "gemini-flash";
    m_maxWordCount = 500;
    m_autoEnhance = false;
    m_preserveFormatting = true;
}

bool EnhancementProfile::isNameValid() const {
    return validateName();
}

bool EnhancementProfile::requiresCustomPrompt() const {
    return m_defaultMode == EnhancementMode::Custom;
}

bool EnhancementProfile::hasValidWordLimit() const {
    return validateWordCount();
}

QString EnhancementProfile::getDisplayName() const {
    return m_name.isEmpty() ? "Unnamed Profile" : m_name;
}

QString EnhancementProfile::getModeDisplayName() const {
    switch (m_defaultMode) {
        case EnhancementMode::GrammarOnly:
            return "Grammar Only";
        case EnhancementMode::StyleImprovement:
            return "Style Enhancement";
        case EnhancementMode::Summarization:
            return "Summarization";
        case EnhancementMode::Formalization:
            return "Formalization";
        case EnhancementMode::Custom:
            return "Custom";
        default:
            return "Unknown";
    }
}

QString EnhancementProfile::getProviderDisplayName() const {
    if (m_provider.contains("gemini-pro")) {
        return "Gemini Pro";
    } else if (m_provider.contains("gemini-flash")) {
        return "Gemini Flash";
    } else if (m_provider.contains("gemini")) {
        return "Google Gemini";
    }
    
    return m_provider.isEmpty() ? "Default Provider" : m_provider;
}

QString EnhancementProfile::getSettingsSummary() const {
    QStringList settings;
    settings << QString("Mode: %1").arg(getModeDisplayName());
    settings << QString("Max words: %1").arg(m_maxWordCount);
    settings << QString("Provider: %1").arg(getProviderDisplayName());
    
    if (m_autoEnhance) {
        settings << "Auto-enhance enabled";
    }
    
    if (m_preserveFormatting) {
        settings << "Preserve formatting";
    }
    
    return settings.join(", ");
}

QString EnhancementProfile::getUsageInfo() const {
    QString info;
    
    if (m_isDefault) {
        info += "[Default] ";
    }
    
    info += getDisplayName();
    
    if (m_lastUsed.isValid()) {
        info += QString(" (Last used: %1)").arg(m_lastUsed.toString("yyyy-MM-dd"));
    } else {
        info += " (Never used)";
    }
    
    return info;
}

EnhancementProfile EnhancementProfile::createDefaultProfile() {
    EnhancementProfile profile("Quick Grammar Fix");
    profile.setDefaultMode(EnhancementMode::GrammarOnly);
    profile.setProvider("gemini-flash");
    profile.setMaxWordCount(500);
    profile.setAutoEnhance(false);
    profile.setIsDefault(true);
    return profile;
}

EnhancementProfile EnhancementProfile::createAcademicProfile() {
    EnhancementProfile profile("Academic Writing");
    profile.setDefaultMode(EnhancementMode::Formalization);
    profile.setProvider("gemini-pro");
    profile.setMaxWordCount(1000);
    profile.setAutoEnhance(false);
    profile.setPreserveFormatting(true);
    return profile;
}

EnhancementProfile EnhancementProfile::createBusinessProfile() {
    EnhancementProfile profile("Business Communication");
    profile.setDefaultMode(EnhancementMode::StyleImprovement);
    profile.setProvider("gemini-pro");
    profile.setMaxWordCount(800);
    profile.setAutoEnhance(true);
    profile.setPreserveFormatting(true);
    return profile;
}

EnhancementProfile EnhancementProfile::createCasualProfile() {
    EnhancementProfile profile("Casual Notes");
    profile.setDefaultMode(EnhancementMode::GrammarOnly);
    profile.setProvider("gemini-flash");
    profile.setMaxWordCount(300);
    profile.setAutoEnhance(true);
    profile.setPreserveFormatting(false);
    return profile;
}

EnhancementProfile EnhancementProfile::createSummaryProfile() {
    EnhancementProfile profile("Meeting Summary");
    profile.setDefaultMode(EnhancementMode::Summarization);
    profile.setProvider("gemini-pro");
    profile.setMaxWordCount(2000);
    profile.setAutoEnhance(false);
    profile.setPreserveFormatting(false);
    return profile;
}

QStringList EnhancementProfile::getSupportedProviders() const {
    return QStringList{
        "gemini-flash",
        "gemini-pro",
        "gemini-pro-vision"
    };
}

QStringList EnhancementProfile::getAvailableModes() const {
    return QStringList{
        "Grammar Only",
        "Style Enhancement", 
        "Summarization",
        "Formalization",
        "Custom"
    };
}

bool EnhancementProfile::canBeDeleted() const {
    return !m_isDefault; // Can't delete the default profile
}

bool EnhancementProfile::operator==(const EnhancementProfile& other) const {
    return m_id == other.m_id;
}

bool EnhancementProfile::operator!=(const EnhancementProfile& other) const {
    return !(*this == other);
}

bool EnhancementProfile::validateName() const {
    QString trimmedName = m_name.trimmed();
    return !trimmedName.isEmpty() && trimmedName.length() <= 100;
}

bool EnhancementProfile::validateWordCount() const {
    return m_maxWordCount >= 1 && m_maxWordCount <= 10000;
}

bool EnhancementProfile::validateProvider() const {
    if (m_provider.isEmpty()) {
        return true; // Empty provider is allowed, will use default
    }
    
    return getSupportedProviders().contains(m_provider) || 
           m_provider.startsWith("gemini");
}

bool EnhancementProfile::validateCustomPrompt() const {
    if (requiresCustomPrompt()) {
        return !m_customPrompt.trimmed().isEmpty();
    }
    
    return true; // Custom prompt not required for other modes
}
