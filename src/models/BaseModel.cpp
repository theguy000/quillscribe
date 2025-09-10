#include "BaseModel.h"
#include <QUuid>
#include <QRegularExpression>

QString BaseModel::generateUuid() {
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

bool BaseModel::isValidUuid(const QString& uuid) {
    if (uuid.isEmpty()) {
        return false;
    }
    
    // UUID pattern: 8-4-4-4-12 hexadecimal digits
    QRegularExpression uuidPattern("^[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}$");
    return uuidPattern.match(uuid).hasMatch();
}

bool BaseModel::isValidLanguageCode(const QString& languageCode) {
    if (languageCode.isEmpty()) {
        return false;
    }
    
    // ISO 639-1 language code pattern (e.g., "en", "en-US", "fr-CA")
    QRegularExpression langPattern("^[a-z]{2}(-[A-Z]{2})?$");
    return langPattern.match(languageCode).hasMatch();
}

// RecordingStatus conversion functions
QString recordingStatusToString(RecordingStatus status) {
    switch (status) {
        case RecordingStatus::Recording:
            return "Recording";
        case RecordingStatus::Completed:
            return "Completed";
        case RecordingStatus::Processing:
            return "Processing";
        case RecordingStatus::Error:
            return "Error";
        case RecordingStatus::Cancelled:
            return "Cancelled";
        default:
            return "Unknown";
    }
}

RecordingStatus recordingStatusFromString(const QString& status) {
    if (status == "Recording") return RecordingStatus::Recording;
    if (status == "Completed") return RecordingStatus::Completed;
    if (status == "Processing") return RecordingStatus::Processing;
    if (status == "Error") return RecordingStatus::Error;
    if (status == "Cancelled") return RecordingStatus::Cancelled;
    return RecordingStatus::Error; // Default for unknown values
}

// TranscriptionStatus conversion functions
QString transcriptionStatusToString(TranscriptionStatus status) {
    switch (status) {
        case TranscriptionStatus::Pending:
            return "Pending";
        case TranscriptionStatus::Processing:
            return "Processing";
        case TranscriptionStatus::Completed:
            return "Completed";
        case TranscriptionStatus::Failed:
            return "Failed";
        case TranscriptionStatus::Cancelled:
            return "Cancelled";
        default:
            return "Unknown";
    }
}

TranscriptionStatus transcriptionStatusFromString(const QString& status) {
    if (status == "Pending") return TranscriptionStatus::Pending;
    if (status == "Processing") return TranscriptionStatus::Processing;
    if (status == "Completed") return TranscriptionStatus::Completed;
    if (status == "Failed") return TranscriptionStatus::Failed;
    if (status == "Cancelled") return TranscriptionStatus::Cancelled;
    return TranscriptionStatus::Failed; // Default for unknown values
}

// SessionStatus conversion functions
QString sessionStatusToString(SessionStatus status) {
    switch (status) {
        case SessionStatus::Active:
            return "Active";
        case SessionStatus::Completed:
            return "Completed";
        case SessionStatus::Archived:
            return "Archived";
        default:
            return "Unknown";
    }
}

SessionStatus sessionStatusFromString(const QString& status) {
    if (status == "Active") return SessionStatus::Active;
    if (status == "Completed") return SessionStatus::Completed;
    if (status == "Archived") return SessionStatus::Archived;
    return SessionStatus::Completed; // Default for unknown values
}

// EnhancementMode conversion functions
QString enhancementModeToString(EnhancementMode mode) {
    switch (mode) {
        case EnhancementMode::GrammarOnly:
            return "GrammarOnly";
        case EnhancementMode::StyleImprovement:
            return "StyleImprovement";
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

EnhancementMode enhancementModeFromString(const QString& mode) {
    if (mode == "GrammarOnly") return EnhancementMode::GrammarOnly;
    if (mode == "StyleImprovement") return EnhancementMode::StyleImprovement;
    if (mode == "Summarization") return EnhancementMode::Summarization;
    if (mode == "Formalization") return EnhancementMode::Formalization;
    if (mode == "Custom") return EnhancementMode::Custom;
    return EnhancementMode::GrammarOnly; // Default for unknown values
}
