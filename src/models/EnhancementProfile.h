#pragma once

#include "BaseModel.h"
#include <QDateTime>

/**
 * @brief EnhancementProfile model class
 * 
 * User preferences for AI text enhancement.
 * Provides templates and settings for different enhancement scenarios.
 */
class EnhancementProfile : public BaseModel {
public:
    // Constructors
    EnhancementProfile();
    explicit EnhancementProfile(const QString& name);
    explicit EnhancementProfile(const QJsonObject& json);
    
    // Copy constructor and assignment operator
    EnhancementProfile(const EnhancementProfile& other);
    EnhancementProfile& operator=(const EnhancementProfile& other);
    
    // BaseModel interface implementation
    QString getId() const override;
    QJsonObject toJson() const override;
    bool fromJson(const QJsonObject& json) override;
    bool isValid() const override;
    
    // Getters
    QString getName() const { return m_name; }
    EnhancementMode getDefaultMode() const { return m_defaultMode; }
    QString getCustomPrompt() const { return m_customPrompt; }
    QString getProvider() const { return m_provider; }
    int getMaxWordCount() const { return m_maxWordCount; }
    bool getAutoEnhance() const { return m_autoEnhance; }
    bool getPreserveFormatting() const { return m_preserveFormatting; }
    bool getIsDefault() const { return m_isDefault; }
    QDateTime getCreatedAt() const { return m_createdAt; }
    QDateTime getLastUsed() const { return m_lastUsed; }
    
    // Setters
    void setName(const QString& name);
    void setDefaultMode(EnhancementMode mode);
    void setCustomPrompt(const QString& prompt);
    void setProvider(const QString& provider);
    void setMaxWordCount(int maxWords);
    void setAutoEnhance(bool autoEnhance);
    void setPreserveFormatting(bool preserve);
    void setIsDefault(bool isDefault);
    void setCreatedAt(const QDateTime& createdAt);
    void setLastUsed(const QDateTime& lastUsed);
    
    // Profile management
    void markAsUsed();
    void makeDefault();
    void resetToDefaults();
    
    // Validation and constraints
    bool isNameValid() const;
    bool requiresCustomPrompt() const;
    bool hasValidWordLimit() const;
    
    // Display and formatting
    QString getDisplayName() const;
    QString getModeDisplayName() const;
    QString getProviderDisplayName() const;
    QString getSettingsSummary() const;
    QString getUsageInfo() const;
    
    // Profile templates and presets
    static EnhancementProfile createDefaultProfile();
    static EnhancementProfile createAcademicProfile();
    static EnhancementProfile createBusinessProfile();
    static EnhancementProfile createCasualProfile();
    static EnhancementProfile createSummaryProfile();
    
    // Utility methods
    QStringList getSupportedProviders() const;
    QStringList getAvailableModes() const;
    bool canBeDeleted() const;
    
    // Equality operators
    bool operator==(const EnhancementProfile& other) const;
    bool operator!=(const EnhancementProfile& other) const;

private:
    QString m_id;
    QString m_name;                     // Profile name
    EnhancementMode m_defaultMode;      // Default enhancement type
    QString m_customPrompt;             // Custom enhancement prompt
    QString m_provider;                 // Preferred AI provider
    int m_maxWordCount;                 // Maximum words to process
    bool m_autoEnhance;                 // Automatically enhance after transcription
    bool m_preserveFormatting;          // Keep original formatting/structure
    bool m_isDefault;                   // Whether this is the default profile
    QDateTime m_createdAt;              // Profile creation time
    QDateTime m_lastUsed;               // Last time profile was used
    
    // Validation helpers
    bool validateName() const;
    bool validateWordCount() const;
    bool validateProvider() const;
    bool validateCustomPrompt() const;
};
