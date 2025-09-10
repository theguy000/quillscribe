#pragma once

#include <QObject>
#include <QSettings>
#include <QString>
#include <QVariant>
#include <QMap>
#include <QStringList>

/**
 * @brief Centralized configuration and settings management
 * 
 * Manages application settings, user preferences, and configuration
 * with validation, defaults, and change notifications.
 */
class ConfigurationManager : public QObject {
    Q_OBJECT

public:
    enum class SettingsCategory {
        Application,
        Audio,
        Transcription,
        Enhancement,
        Storage,
        UI
    };

    explicit ConfigurationManager(QObject* parent = nullptr);
    ~ConfigurationManager();

    // Settings management
    template<typename T>
    T getValue(const QString& key, const T& defaultValue = T{}) const;
    
    template<typename T>
    void setValue(const QString& key, const T& value);
    
    bool contains(const QString& key) const;
    void remove(const QString& key);
    QStringList keys() const;

    // Category-specific getters
    QVariant getApplicationSetting(const QString& key, const QVariant& defaultValue = QVariant()) const;
    QVariant getAudioSetting(const QString& key, const QVariant& defaultValue = QVariant()) const;
    QVariant getTranscriptionSetting(const QString& key, const QVariant& defaultValue = QVariant()) const;
    QVariant getEnhancementSetting(const QString& key, const QVariant& defaultValue = QVariant()) const;
    QVariant getStorageSetting(const QString& key, const QVariant& defaultValue = QVariant()) const;
    QVariant getUISetting(const QString& key, const QVariant& defaultValue = QVariant()) const;

    // Category-specific setters
    void setApplicationSetting(const QString& key, const QVariant& value);
    void setAudioSetting(const QString& key, const QVariant& value);
    void setTranscriptionSetting(const QString& key, const QVariant& value);
    void setEnhancementSetting(const QString& key, const QVariant& value);
    void setStorageSetting(const QString& key, const QVariant& value);
    void setUISetting(const QString& key, const QVariant& value);

    // Convenience methods for common settings
    QString getApiKey(const QString& provider) const;
    void setApiKey(const QString& provider, const QString& apiKey);
    
    QString getCurrentSessionId() const;
    void setCurrentSessionId(const QString& sessionId);
    
    int getTranscriptionProvider() const;
    void setTranscriptionProvider(int provider);
    
    int getEnhancementMode() const;
    void setEnhancementMode(int mode);
    
    int getInputGain() const;
    void setInputGain(int gain);

    // Window and UI state
    QByteArray getWindowGeometry() const;
    void setWindowGeometry(const QByteArray& geometry);
    
    QByteArray getWindowState() const;
    void setWindowState(const QByteArray& state);
    
    QByteArray getSplitterState() const;
    void setSplitterState(const QByteArray& state);

    // Export/Import
    bool exportSettings(const QString& filePath) const;
    bool importSettings(const QString& filePath);
    
    // Reset to defaults
    void resetCategory(SettingsCategory category);
    void resetAll();
    
    // Validation
    bool isValidApiKey(const QString& provider, const QString& apiKey) const;
    bool isValidPath(const QString& path) const;

    // Configuration file management
    QString getConfigFilePath() const;
    bool backupSettings(const QString& backupPath) const;
    bool restoreSettings(const QString& backupPath);

signals:
    void settingChanged(const QString& key, const QVariant& value);
    void categoryChanged(SettingsCategory category);
    void settingsReset(SettingsCategory category);
    void configurationLoaded();
    void configurationSaved();

private:
    void initializeDefaults();
    void loadSettings();
    void saveSettings();
    QString categoryToString(SettingsCategory category) const;
    SettingsCategory categoryFromString(const QString& categoryStr) const;
    QVariant getDefaultValue(const QString& key) const;
    bool validateSetting(const QString& key, const QVariant& value) const;

    QSettings* m_settings;
    QMap<QString, QVariant> m_defaults;
    QMap<QString, SettingsCategory> m_keyCategories;
    bool m_autoSave;
    
    // Default values constants
    static const int DEFAULT_INPUT_GAIN;
    static const int DEFAULT_TRANSCRIPTION_PROVIDER;
    static const int DEFAULT_ENHANCEMENT_MODE;
    static const QString DEFAULT_LANGUAGE;
    static const QString DEFAULT_AUDIO_FORMAT;
};

// Template implementations
template<typename T>
T ConfigurationManager::getValue(const QString& key, const T& defaultValue) const {
    return m_settings->value(key, QVariant::fromValue(defaultValue)).template value<T>();
}

template<typename T>
void ConfigurationManager::setValue(const QString& key, const T& value) {
    QVariant oldValue = m_settings->value(key);
    QVariant newValue = QVariant::fromValue(value);
    
    if (oldValue != newValue && validateSetting(key, newValue)) {
        m_settings->setValue(key, newValue);
        
        if (m_autoSave) {
            m_settings->sync();
        }
        
        emit settingChanged(key, newValue);
        
        // Check if this affects a category
        if (m_keyCategories.contains(key)) {
            emit categoryChanged(m_keyCategories.value(key));
        }
    }
}
