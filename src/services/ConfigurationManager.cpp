#include "ConfigurationManager.h"
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFileInfo>
#include <QApplication>
#include <QCoreApplication>

// Default value constants
const int ConfigurationManager::DEFAULT_INPUT_GAIN = 100;
const int ConfigurationManager::DEFAULT_TRANSCRIPTION_PROVIDER = 1; // Base model
const int ConfigurationManager::DEFAULT_ENHANCEMENT_MODE = 1; // Style improvement
const QString ConfigurationManager::DEFAULT_LANGUAGE = "en";
const QString ConfigurationManager::DEFAULT_AUDIO_FORMAT = "wav";

ConfigurationManager::ConfigurationManager(QObject* parent)
    : QObject(parent)
    , m_settings(nullptr)
    , m_autoSave(true)
{
    // Set application properties for QSettings
    QCoreApplication::setOrganizationName("QuillScribe");
    QCoreApplication::setOrganizationDomain("quillscribe.app");
    QCoreApplication::setApplicationName("QuillScribe");
    
    // Initialize QSettings
    m_settings = new QSettings(this);
    
    // Initialize defaults and categories
    initializeDefaults();
    
    // Load current settings
    loadSettings();
    
    qDebug() << "ConfigurationManager initialized with settings file:" << m_settings->fileName();
}

ConfigurationManager::~ConfigurationManager() {
    if (m_autoSave) {
        saveSettings();
    }
}

bool ConfigurationManager::contains(const QString& key) const {
    return m_settings->contains(key);
}

void ConfigurationManager::remove(const QString& key) {
    m_settings->remove(key);
    if (m_autoSave) {
        m_settings->sync();
    }
}

QStringList ConfigurationManager::keys() const {
    return m_settings->allKeys();
}

// Category-specific getters
QVariant ConfigurationManager::getApplicationSetting(const QString& key, const QVariant& defaultValue) const {
    return m_settings->value("Application/" + key, defaultValue);
}

QVariant ConfigurationManager::getAudioSetting(const QString& key, const QVariant& defaultValue) const {
    return m_settings->value("Audio/" + key, defaultValue);
}

QVariant ConfigurationManager::getTranscriptionSetting(const QString& key, const QVariant& defaultValue) const {
    return m_settings->value("Transcription/" + key, defaultValue);
}

QVariant ConfigurationManager::getEnhancementSetting(const QString& key, const QVariant& defaultValue) const {
    return m_settings->value("Enhancement/" + key, defaultValue);
}

QVariant ConfigurationManager::getStorageSetting(const QString& key, const QVariant& defaultValue) const {
    return m_settings->value("Storage/" + key, defaultValue);
}

QVariant ConfigurationManager::getUISetting(const QString& key, const QVariant& defaultValue) const {
    return m_settings->value("UI/" + key, defaultValue);
}

// Category-specific setters
void ConfigurationManager::setApplicationSetting(const QString& key, const QVariant& value) {
    setValue("Application/" + key, value);
}

void ConfigurationManager::setAudioSetting(const QString& key, const QVariant& value) {
    setValue("Audio/" + key, value);
}

void ConfigurationManager::setTranscriptionSetting(const QString& key, const QVariant& value) {
    setValue("Transcription/" + key, value);
}

void ConfigurationManager::setEnhancementSetting(const QString& key, const QVariant& value) {
    setValue("Enhancement/" + key, value);
}

void ConfigurationManager::setStorageSetting(const QString& key, const QVariant& value) {
    setValue("Storage/" + key, value);
}

void ConfigurationManager::setUISetting(const QString& key, const QVariant& value) {
    setValue("UI/" + key, value);
}

// Convenience methods for common settings
QString ConfigurationManager::getApiKey(const QString& provider) const {
    return getEnhancementSetting("ApiKey_" + provider, QString()).toString();
}

void ConfigurationManager::setApiKey(const QString& provider, const QString& apiKey) {
    setEnhancementSetting("ApiKey_" + provider, apiKey);
}

QString ConfigurationManager::getCurrentSessionId() const {
    return getApplicationSetting("CurrentSessionId", QString()).toString();
}

void ConfigurationManager::setCurrentSessionId(const QString& sessionId) {
    setApplicationSetting("CurrentSessionId", sessionId);
}

int ConfigurationManager::getTranscriptionProvider() const {
    return getTranscriptionSetting("Provider", DEFAULT_TRANSCRIPTION_PROVIDER).toInt();
}

void ConfigurationManager::setTranscriptionProvider(int provider) {
    setTranscriptionSetting("Provider", provider);
}

int ConfigurationManager::getEnhancementMode() const {
    return getEnhancementSetting("Mode", DEFAULT_ENHANCEMENT_MODE).toInt();
}

void ConfigurationManager::setEnhancementMode(int mode) {
    setEnhancementSetting("Mode", mode);
}

int ConfigurationManager::getInputGain() const {
    return getAudioSetting("InputGain", DEFAULT_INPUT_GAIN).toInt();
}

void ConfigurationManager::setInputGain(int gain) {
    setAudioSetting("InputGain", gain);
}

// Window and UI state
QByteArray ConfigurationManager::getWindowGeometry() const {
    return getUISetting("WindowGeometry", QByteArray()).toByteArray();
}

void ConfigurationManager::setWindowGeometry(const QByteArray& geometry) {
    setUISetting("WindowGeometry", geometry);
}

QByteArray ConfigurationManager::getWindowState() const {
    return getUISetting("WindowState", QByteArray()).toByteArray();
}

void ConfigurationManager::setWindowState(const QByteArray& state) {
    setUISetting("WindowState", state);
}

QByteArray ConfigurationManager::getSplitterState() const {
    return getUISetting("SplitterState", QByteArray()).toByteArray();
}

void ConfigurationManager::setSplitterState(const QByteArray& state) {
    setUISetting("SplitterState", state);
}

// Export/Import
bool ConfigurationManager::exportSettings(const QString& filePath) const {
    QJsonObject settingsObj;
    
    // Export all settings by category
    QStringList allKeys = m_settings->allKeys();
    for (const QString& key : allKeys) {
        QVariant value = m_settings->value(key);
        
        // Only export serializable values
        if (value.canConvert<QString>()) {
            settingsObj[key] = value.toString();
        } else if (value.canConvert<int>()) {
            settingsObj[key] = value.toInt();
        } else if (value.canConvert<bool>()) {
            settingsObj[key] = value.toBool();
        } else if (value.canConvert<double>()) {
            settingsObj[key] = value.toDouble();
        }
    }
    
    QJsonDocument doc(settingsObj);
    
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Failed to open export file:" << filePath;
        return false;
    }
    
    file.write(doc.toJson());
    return true;
}

bool ConfigurationManager::importSettings(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open import file:" << filePath;
        return false;
    }
    
    QByteArray data = file.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    
    if (!doc.isObject()) {
        qWarning() << "Invalid settings file format";
        return false;
    }
    
    QJsonObject settingsObj = doc.object();
    for (auto it = settingsObj.begin(); it != settingsObj.end(); ++it) {
        QString key = it.key();
        QJsonValue value = it.value();
        
        // Import the value based on its type
        if (value.isBool()) {
            setValue(key, value.toBool());
        } else if (value.isDouble()) {
            setValue(key, value.toDouble());
        } else if (value.isString()) {
            setValue(key, value.toString());
        }
    }
    
    emit configurationLoaded();
    return true;
}

// Reset to defaults
void ConfigurationManager::resetCategory(SettingsCategory category) {
    QString categoryStr = categoryToString(category);
    
    // Remove all keys in the category
    m_settings->beginGroup(categoryStr);
    m_settings->remove("");
    m_settings->endGroup();
    
    // Restore defaults for this category
    for (auto it = m_defaults.begin(); it != m_defaults.end(); ++it) {
        if (it.key().startsWith(categoryStr + "/")) {
            m_settings->setValue(it.key(), it.value());
        }
    }
    
    if (m_autoSave) {
        m_settings->sync();
    }
    
    emit settingsReset(category);
    emit categoryChanged(category);
}

void ConfigurationManager::resetAll() {
    m_settings->clear();
    
    // Restore all defaults
    for (auto it = m_defaults.begin(); it != m_defaults.end(); ++it) {
        m_settings->setValue(it.key(), it.value());
    }
    
    if (m_autoSave) {
        m_settings->sync();
    }
    
    emit settingsReset(SettingsCategory::Application);
}

// Validation
bool ConfigurationManager::isValidApiKey(const QString& provider, const QString& apiKey) const {
    Q_UNUSED(provider)
    // Basic validation - check if not empty and has reasonable length
    return !apiKey.isEmpty() && apiKey.length() >= 10;
}

bool ConfigurationManager::isValidPath(const QString& path) const {
    if (path.isEmpty()) return false;
    
    QFileInfo fileInfo(path);
    QDir parentDir = fileInfo.absoluteDir();
    
    return parentDir.exists() || parentDir.mkpath(parentDir.absolutePath());
}

// Configuration file management
QString ConfigurationManager::getConfigFilePath() const {
    return m_settings->fileName();
}

bool ConfigurationManager::backupSettings(const QString& backupPath) const {
    QString configPath = getConfigFilePath();
    return QFile::copy(configPath, backupPath);
}

bool ConfigurationManager::restoreSettings(const QString& backupPath) {
    if (!QFile::exists(backupPath)) {
        return false;
    }
    
    QString configPath = getConfigFilePath();
    
    // Remove current config
    if (QFile::exists(configPath)) {
        QFile::remove(configPath);
    }
    
    // Copy backup to config location
    if (QFile::copy(backupPath, configPath)) {
        // Reload settings
        delete m_settings;
        m_settings = new QSettings(this);
        loadSettings();
        return true;
    }
    
    return false;
}

// Private methods
void ConfigurationManager::initializeDefaults() {
    // Application defaults
    m_defaults["Application/Language"] = DEFAULT_LANGUAGE;
    m_defaults["Application/CurrentSessionId"] = QString();
    m_defaults["Application/AutoSave"] = true;
    m_defaults["Application/CheckUpdates"] = true;
    
    // Audio defaults
    m_defaults["Audio/InputGain"] = DEFAULT_INPUT_GAIN;
    m_defaults["Audio/Format"] = DEFAULT_AUDIO_FORMAT;
    m_defaults["Audio/SampleRate"] = 16000;
    m_defaults["Audio/AutoGainControl"] = true;
    m_defaults["Audio/NoiseReduction"] = true;
    m_defaults["Audio/DeviceName"] = QString();
    
    // Transcription defaults
    m_defaults["Transcription/Provider"] = DEFAULT_TRANSCRIPTION_PROVIDER;
    m_defaults["Transcription/Language"] = DEFAULT_LANGUAGE;
    m_defaults["Transcription/ModelPath"] = QString();
    m_defaults["Transcription/MaxConcurrent"] = 2;
    m_defaults["Transcription/Timeout"] = 30000;
    
    // Enhancement defaults
    m_defaults["Enhancement/Mode"] = DEFAULT_ENHANCEMENT_MODE;
    m_defaults["Enhancement/ApiKey_Gemini"] = QString();
    m_defaults["Enhancement/MaxTextLength"] = 2000;
    m_defaults["Enhancement/Creativity"] = 0.3;
    m_defaults["Enhancement/TargetAudience"] = "general";
    m_defaults["Enhancement/Tone"] = "professional";
    
    // Storage defaults
    m_defaults["Storage/DatabasePath"] = QString();
    m_defaults["Storage/BackupEnabled"] = true;
    m_defaults["Storage/BackupInterval"] = 24; // hours
    m_defaults["Storage/MaxBackups"] = 5;
    
    // UI defaults
    m_defaults["UI/WindowGeometry"] = QByteArray();
    m_defaults["UI/WindowState"] = QByteArray();
    m_defaults["UI/SplitterState"] = QByteArray();
    m_defaults["UI/Theme"] = "default";
    m_defaults["UI/ShowStatusBar"] = true;
    m_defaults["UI/ShowToolBar"] = true;
    
    // Initialize key categories
    for (auto it = m_defaults.begin(); it != m_defaults.end(); ++it) {
        QString key = it.key();
        QString categoryStr = key.split("/").first();
        m_keyCategories[key] = categoryFromString(categoryStr);
    }
}

void ConfigurationManager::loadSettings() {
    // Set defaults for missing keys
    for (auto it = m_defaults.begin(); it != m_defaults.end(); ++it) {
        if (!m_settings->contains(it.key())) {
            m_settings->setValue(it.key(), it.value());
        }
    }
    
    if (m_autoSave) {
        m_settings->sync();
    }
    
    emit configurationLoaded();
}

void ConfigurationManager::saveSettings() {
    m_settings->sync();
    emit configurationSaved();
}

QString ConfigurationManager::categoryToString(SettingsCategory category) const {
    switch (category) {
        case SettingsCategory::Application: return "Application";
        case SettingsCategory::Audio: return "Audio";
        case SettingsCategory::Transcription: return "Transcription";
        case SettingsCategory::Enhancement: return "Enhancement";
        case SettingsCategory::Storage: return "Storage";
        case SettingsCategory::UI: return "UI";
        default: return "Unknown";
    }
}

ConfigurationManager::SettingsCategory ConfigurationManager::categoryFromString(const QString& categoryStr) const {
    if (categoryStr == "Application") return SettingsCategory::Application;
    if (categoryStr == "Audio") return SettingsCategory::Audio;
    if (categoryStr == "Transcription") return SettingsCategory::Transcription;
    if (categoryStr == "Enhancement") return SettingsCategory::Enhancement;
    if (categoryStr == "Storage") return SettingsCategory::Storage;
    if (categoryStr == "UI") return SettingsCategory::UI;
    return SettingsCategory::Application;
}

QVariant ConfigurationManager::getDefaultValue(const QString& key) const {
    return m_defaults.value(key, QVariant());
}

bool ConfigurationManager::validateSetting(const QString& key, const QVariant& value) const {
    // Basic validation based on key patterns
    if (key.contains("ApiKey") && value.toString().length() < 10) {
        return false;
    }
    
    if (key.contains("Gain") && (value.toInt() < 0 || value.toInt() > 200)) {
        return false;
    }
    
    if (key.contains("SampleRate") && value.toInt() < 8000) {
        return false;
    }
    
    if (key.contains("Path") && !value.toString().isEmpty() && !isValidPath(value.toString())) {
        return false;
    }
    
    return true;
}
