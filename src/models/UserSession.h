#pragma once

#include "BaseModel.h"
#include <QDateTime>
#include <QStringList>

/**
 * @brief UserSession model class
 * 
 * Groups related recordings and tracks user activity.
 * Provides session management functionality and statistics.
 */
class UserSession : public BaseModel {
public:
    // Constructors
    UserSession();
    explicit UserSession(const QString& name);
    explicit UserSession(const QJsonObject& json);
    
    // Copy constructor and assignment operator
    UserSession(const UserSession& other);
    UserSession& operator=(const UserSession& other);
    
    // BaseModel interface implementation
    QString getId() const override;
    QJsonObject toJson() const override;
    bool fromJson(const QJsonObject& json) override;
    bool isValid() const override;
    
    // Getters
    QString getName() const { return m_name; }
    QDateTime getStartTime() const { return m_startTime; }
    QDateTime getEndTime() const { return m_endTime; }
    int getRecordingCount() const { return m_recordingCount; }
    qint64 getTotalDuration() const { return m_totalDuration; }
    QStringList getTags() const { return m_tags; }
    QString getNotes() const { return m_notes; }
    SessionStatus getStatus() const { return m_status; }
    
    // Setters
    void setName(const QString& name);
    void setStartTime(const QDateTime& startTime);
    void setEndTime(const QDateTime& endTime);
    void setRecordingCount(int count);
    void setTotalDuration(qint64 duration);
    void setTags(const QStringList& tags);
    void setNotes(const QString& notes);
    void setStatus(SessionStatus status);
    
    // Session management
    void startSession();
    void endSession();
    void pauseSession();
    void resumeSession();
    void archiveSession();
    
    // Recording management
    void addRecording(qint64 duration);
    void removeRecording(qint64 duration);
    void updateRecordingCount(int newCount, qint64 newTotalDuration);
    
    // Tag management
    void addTag(const QString& tag);
    void removeTag(const QString& tag);
    bool hasTag(const QString& tag) const;
    void clearTags();
    
    // Statistics and calculations
    qint64 getSessionDuration() const; // Total session time (end - start)
    qint64 getActualRecordingTime() const; // Sum of individual recording durations
    double getRecordingEfficiency() const; // recording time / session time
    QString getFormattedSessionDuration() const;
    QString getFormattedRecordingDuration() const;
    double getAverageRecordingDuration() const;
    
    // Display and formatting
    QString getDisplayName() const;
    QString getStatusDisplayText() const;
    QString getSessionSummary() const;
    QString getTagsAsString(const QString& separator = ", ") const;
    QString getFormattedDateRange() const;
    
    // State checking
    bool isActive() const;
    bool isCompleted() const;
    bool isArchived() const;
    bool hasRecordings() const;
    bool hasEndTime() const;
    bool hasNotes() const;
    bool hasTags() const;
    
    // Session operations
    bool canAddRecordings() const;
    bool canEdit() const;
    bool canDelete() const;
    bool canArchive() const;
    bool canReactivate() const;
    
    // Validation helpers
    bool isNameUnique() const; // Will need external validation
    bool hasValidDateRange() const;
    bool hasReasonableStatistics() const;
    
    // Export functionality
    QString exportToText() const;
    QJsonObject exportStatistics() const;
    
    // Search and filtering
    bool matchesSearchTerm(const QString& searchTerm) const;
    bool isWithinDateRange(const QDateTime& startDate, const QDateTime& endDate) const;
    
    // Equality operators
    bool operator==(const UserSession& other) const;
    bool operator!=(const UserSession& other) const;

private:
    QString m_id;
    QString m_name;                 // User-assigned session name
    QDateTime m_startTime;          // Session start timestamp
    QDateTime m_endTime;            // Session end timestamp (nullable)
    int m_recordingCount;           // Number of recordings in session
    qint64 m_totalDuration;         // Combined duration of all recordings (ms)
    QStringList m_tags;             // User-assigned tags
    QString m_notes;                // User notes about the session
    SessionStatus m_status;         // Active, Completed, Archived
    
    // Validation helpers
    bool validateName() const;
    bool validateDateTimes() const;
    bool validateCounts() const;
    bool validateStatus() const;
    
    // Utility helpers
    QString formatDuration(qint64 durationMs) const;
    QStringList tokenizeForSearch() const;
};
