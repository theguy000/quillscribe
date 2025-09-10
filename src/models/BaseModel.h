#pragma once

#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QDateTime>
#include <QStringList>
#include <QJsonArray>

// Include contract interfaces for shared enums
#include "../../specs/001-voice-to-text/contracts/transcription-service-interface.h"
#include "../../specs/001-voice-to-text/contracts/ai-enhancement-interface.h"

/**
 * @brief Base class for all data models
 * 
 * Provides common interface for serialization, validation, and identification
 * All model classes inherit from this to ensure consistent behavior
 */
class BaseModel {
public:
    virtual ~BaseModel() = default;
    
    // Core interface methods
    virtual QString getId() const = 0;
    virtual QJsonObject toJson() const = 0;
    virtual bool fromJson(const QJsonObject& json) = 0;
    virtual bool isValid() const = 0;
    
public:
    // Utility method for generating UUIDs
    static QString generateUuid();
    
protected:
    
    // Validation helpers
    static bool isValidUuid(const QString& uuid);
    static bool isValidLanguageCode(const QString& languageCode);
};

// Enumerations for model states
enum class RecordingStatus {
    Recording,      // Currently recording
    Completed,      // Recording finished successfully  
    Processing,     // Being transcribed
    Error,          // Recording or processing failed
    Cancelled       // User cancelled recording
};

enum class SessionStatus {
    Active,         // Currently in use
    Completed,      // Finished session
    Archived        // Archived for history
};

// Utility functions for enum conversion
QString recordingStatusToString(RecordingStatus status);
RecordingStatus recordingStatusFromString(const QString& status);

QString transcriptionStatusToString(TranscriptionStatus status);
TranscriptionStatus transcriptionStatusFromString(const QString& status);

QString sessionStatusToString(SessionStatus status);
SessionStatus sessionStatusFromString(const QString& status);

QString enhancementModeToString(EnhancementMode mode);
EnhancementMode enhancementModeFromString(const QString& mode);
