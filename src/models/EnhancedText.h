#pragma once

#include "BaseModel.h"
#include <QDateTime>
#include <QJsonObject>

/**
 * @brief EnhancedText model class
 * 
 * Represents AI-improved version of transcribed text.
 * Includes enhancement settings, provider information, and user rating.
 */
class EnhancedText : public BaseModel {
public:
    // Constructors
    EnhancedText();
    EnhancedText(const QString& transcriptionId, const QString& originalText, const QString& enhancedText);
    explicit EnhancedText(const QJsonObject& json);
    
    // Copy constructor and assignment operator
    EnhancedText(const EnhancedText& other);
    EnhancedText& operator=(const EnhancedText& other);
    
    // BaseModel interface implementation
    QString getId() const override;
    QJsonObject toJson() const override;
    bool fromJson(const QJsonObject& json) override;
    bool isValid() const override;
    
    // Getters
    QString getTranscriptionId() const { return m_transcriptionId; }
    QString getOriginalText() const { return m_originalText; }
    QString getEnhancedText() const { return m_enhancedText; }
    EnhancementMode getEnhancementMode() const { return m_enhancementMode; }
    QString getProvider() const { return m_provider; }
    QString getPromptTemplate() const { return m_promptTemplate; }
    qint64 getProcessingTime() const { return m_processingTime; }
    QJsonObject getSettings() const { return m_settings; }
    QDateTime getCreatedAt() const { return m_createdAt; }
    int getUserRating() const { return m_userRating; }
    
    // Setters
    void setTranscriptionId(const QString& transcriptionId);
    void setOriginalText(const QString& originalText);
    void setEnhancedText(const QString& enhancedText);
    void setEnhancementMode(EnhancementMode mode);
    void setProvider(const QString& provider);
    void setPromptTemplate(const QString& promptTemplate);
    void setProcessingTime(qint64 processingTime);
    void setSettings(const QJsonObject& settings);
    void setCreatedAt(const QDateTime& createdAt);
    void setUserRating(int rating);
    
    // Utility methods
    int getOriginalWordCount() const;
    int getEnhancedWordCount() const;
    int getOriginalCharacterCount() const;
    int getEnhancedCharacterCount() const;
    double getCompressionRatio() const; // enhanced/original length ratio
    QString getFormattedProcessingTime() const;
    QString getEnhancementModeDisplayName() const;
    QString getProviderDisplayName() const;
    QStringList getSupportedProviders() const;
    
    // Text comparison and analysis
    bool hasSignificantChanges() const;
    double calculateSimilarity() const; // 0.0-1.0, simple text similarity
    QStringList getAddedWords() const;
    QStringList getRemovedWords() const;
    QString getDiffSummary() const;
    QString getDisplayPreview(int maxLength = 100) const;
    
    // Enhancement settings helpers
    void setMaxWordCount(int maxWords);
    void setPreserveFormatting(bool preserve);
    void setCustomPrompt(const QString& prompt);
    void setTemperature(double temperature); // For AI creativity control
    
    int getMaxWordCount() const;
    bool getPreserveFormatting() const;
    QString getCustomPrompt() const;
    double getTemperature() const;
    
    // Rating and feedback
    bool hasUserRating() const;
    QString getRatingText() const; // "Excellent", "Good", etc.
    void clearUserRating();
    
    // Enhancement validation
    bool isEnhancementValid() const;
    bool exceedsWordLimit(int maxWords) const;
    bool hasProperFormatting() const;
    
    // Export and sharing
    QString getPlainText() const;
    QString getMarkdownText() const;
    QString getComparisonText() const; // Side-by-side comparison
    
    // Equality operators
    bool operator==(const EnhancedText& other) const;
    bool operator!=(const EnhancedText& other) const;

private:
    QString m_id;
    QString m_transcriptionId;      // Foreign key to Transcription
    QString m_originalText;         // Source text before enhancement
    QString m_enhancedText;         // AI-improved text
    EnhancementMode m_enhancementMode;  // Type of enhancement applied
    QString m_provider;             // AI provider used
    QString m_promptTemplate;       // Enhancement prompt used
    qint64 m_processingTime;        // Time taken for enhancement (ms)
    QJsonObject m_settings;         // Enhancement settings used
    QDateTime m_createdAt;          // When enhancement was completed
    int m_userRating;               // User rating (1-5, 0 = no rating)
    
    // Validation helpers
    bool validateProcessingTime() const;
    bool validateProvider() const;
    bool validateUserRating() const;
    bool validateEnhancementMode() const;
    
    // Text analysis helpers
    QStringList tokenizeText(const QString& text) const;
    double calculateLevenshteinDistance(const QString& text1, const QString& text2) const;
};
