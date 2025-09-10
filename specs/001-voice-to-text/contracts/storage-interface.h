// Storage Interface Contract
// Contract for data persistence functionality in voice-to-text application

#pragma once

#include <QObject>
#include <QString>
#include <QDateTime>
#include <QJsonObject>
#include <QList>

// Forward declarations for data models
class Recording;
class Transcription;
class EnhancedText;
class UserSession;
class EnhancementProfile;

enum class StorageError {
    NoError,
    DatabaseConnectionFailed,
    TableCreationFailed,
    InsertFailed,
    UpdateFailed,
    DeleteFailed,
    QueryFailed,
    RecordNotFound,
    ConstraintViolation,
    DiskSpaceInsufficient,
    PermissionDenied,
    CorruptedData,
    BackupFailed,
    UnknownError
};

enum class SortOrder {
    Ascending,
    Descending
};

struct QueryFilter {
    QString field;
    QString operation;      // "=", "!=", ">", "<", ">=", "<=", "LIKE", "IN"
    QVariant value;
    QString logicalOperator = "AND"; // "AND", "OR"
};

struct QueryOptions {
    QList<QueryFilter> filters;
    QString orderBy;
    SortOrder sortOrder = SortOrder::Descending;
    int limit = -1;         // -1 means no limit
    int offset = 0;
    bool includeDeleted = false;
};

/**
 * @brief Interface for recording data persistence
 * 
 * Contract Requirements:
 * - FR-007: Must save transcription history with timestamps for later retrieval
 * - FR-015: Must protect user privacy and voice data
 */
class IRecordingStorage : public QObject {
    Q_OBJECT

public:
    explicit IRecordingStorage(QObject* parent = nullptr) : QObject(parent) {}
    virtual ~IRecordingStorage() = default;

    // CRUD Operations
    virtual QString saveRecording(const Recording& recording) = 0;
    virtual Recording getRecording(const QString& id) const = 0;
    virtual bool updateRecording(const Recording& recording) = 0;
    virtual bool deleteRecording(const QString& id) = 0;
    virtual bool recordingExists(const QString& id) const = 0;

    // Query Operations
    virtual QList<Recording> getAllRecordings(const QueryOptions& options = {}) const = 0;
    virtual QList<Recording> getRecordingsBySession(const QString& sessionId, const QueryOptions& options = {}) const = 0;
    virtual QList<Recording> getRecordingsByDateRange(const QDateTime& start, const QDateTime& end) const = 0;
    virtual QList<Recording> searchRecordings(const QString& searchTerm, const QueryOptions& options = {}) const = 0;

    // Statistics
    virtual int getRecordingCount() const = 0;
    virtual qint64 getTotalRecordingDuration() const = 0;
    virtual qint64 getTotalStorageUsed() const = 0;
    virtual QDateTime getOldestRecordingDate() const = 0;
    virtual QDateTime getNewestRecordingDate() const = 0;

    // Maintenance
    virtual bool cleanup() = 0;
    virtual bool vacuum() = 0;
    virtual QStringList getOrphanedAudioFiles() const = 0;

signals:
    void recordingCreated(const QString& id);
    void recordingUpdated(const QString& id);
    void recordingDeleted(const QString& id);
};

/**
 * @brief Interface for transcription data persistence
 */
class ITranscriptionStorage : public QObject {
    Q_OBJECT

public:
    explicit ITranscriptionStorage(QObject* parent = nullptr) : QObject(parent) {}
    virtual ~ITranscriptionStorage() = default;

    // CRUD Operations
    virtual QString saveTranscription(const Transcription& transcription) = 0;
    virtual Transcription getTranscription(const QString& id) const = 0;
    virtual bool updateTranscription(const Transcription& transcription) = 0;
    virtual bool deleteTranscription(const QString& id) = 0;
    virtual bool transcriptionExists(const QString& id) const = 0;

    // Query Operations
    virtual QList<Transcription> getAllTranscriptions(const QueryOptions& options = {}) const = 0;
    virtual Transcription getTranscriptionByRecording(const QString& recordingId) const = 0;
    virtual QList<Transcription> searchTranscriptions(const QString& searchTerm, const QueryOptions& options = {}) const = 0;
    virtual QList<Transcription> getTranscriptionsByProvider(const QString& provider) const = 0;

    // Statistics
    virtual int getTranscriptionCount() const = 0;
    virtual double getAverageConfidence() const = 0;
    virtual qint64 getAverageProcessingTime() const = 0;

signals:
    void transcriptionCreated(const QString& id);
    void transcriptionUpdated(const QString& id);
    void transcriptionDeleted(const QString& id);
};

/**
 * @brief Interface for enhanced text data persistence
 */
class IEnhancedTextStorage : public QObject {
    Q_OBJECT

public:
    explicit IEnhancedTextStorage(QObject* parent = nullptr) : QObject(parent) {}
    virtual ~IEnhancedTextStorage() = default;

    // CRUD Operations
    virtual QString saveEnhancedText(const EnhancedText& enhancedText) = 0;
    virtual EnhancedText getEnhancedText(const QString& id) const = 0;
    virtual bool updateEnhancedText(const EnhancedText& enhancedText) = 0;
    virtual bool deleteEnhancedText(const QString& id) = 0;
    virtual bool enhancedTextExists(const QString& id) const = 0;

    // Query Operations
    virtual QList<EnhancedText> getAllEnhancedTexts(const QueryOptions& options = {}) const = 0;
    virtual QList<EnhancedText> getEnhancedTextsByTranscription(const QString& transcriptionId) const = 0;
    virtual QList<EnhancedText> getEnhancedTextsByMode(int enhancementMode) const = 0;
    virtual QList<EnhancedText> getEnhancedTextsByProvider(const QString& provider) const = 0;

    // Statistics
    virtual int getEnhancedTextCount() const = 0;
    virtual qint64 getAverageProcessingTime() const = 0;
    virtual double getAverageUserRating() const = 0;

signals:
    void enhancedTextCreated(const QString& id);
    void enhancedTextUpdated(const QString& id);
    void enhancedTextDeleted(const QString& id);
};

/**
 * @brief Interface for user session data persistence
 */
class IUserSessionStorage : public QObject {
    Q_OBJECT

public:
    explicit IUserSessionStorage(QObject* parent = nullptr) : QObject(parent) {}
    virtual ~IUserSessionStorage() = default;

    // CRUD Operations
    virtual QString saveUserSession(const UserSession& session) = 0;
    virtual UserSession getUserSession(const QString& id) const = 0;
    virtual bool updateUserSession(const UserSession& session) = 0;
    virtual bool deleteUserSession(const QString& id) = 0;
    virtual bool userSessionExists(const QString& id) const = 0;

    // Session Management
    virtual QString createNewSession(const QString& name) = 0;
    virtual bool endSession(const QString& id, const QDateTime& endTime = QDateTime::currentDateTime()) = 0;
    virtual UserSession getCurrentActiveSession() const = 0;
    virtual QList<UserSession> getActiveSessions() const = 0;

    // Query Operations
    virtual QList<UserSession> getAllSessions(const QueryOptions& options = {}) const = 0;
    virtual QList<UserSession> getSessionsByDateRange(const QDateTime& start, const QDateTime& end) const = 0;
    virtual QList<UserSession> searchSessions(const QString& searchTerm, const QueryOptions& options = {}) const = 0;

    // Statistics
    virtual int getSessionCount() const = 0;
    virtual qint64 getAverageSessionDuration() const = 0;
    virtual int getAverageRecordingsPerSession() const = 0;

signals:
    void sessionCreated(const QString& id);
    void sessionUpdated(const QString& id);
    void sessionDeleted(const QString& id);
    void sessionStarted(const QString& id);
    void sessionEnded(const QString& id);
};

/**
 * @brief Interface for enhancement profile data persistence
 */
class IEnhancementProfileStorage : public QObject {
    Q_OBJECT

public:
    explicit IEnhancementProfileStorage(QObject* parent = nullptr) : QObject(parent) {}
    virtual ~IEnhancementProfileStorage() = default;

    // CRUD Operations
    virtual QString saveProfile(const EnhancementProfile& profile) = 0;
    virtual EnhancementProfile getProfile(const QString& id) const = 0;
    virtual bool updateProfile(const EnhancementProfile& profile) = 0;
    virtual bool deleteProfile(const QString& id) = 0;
    virtual bool profileExists(const QString& id) const = 0;

    // Profile Management
    virtual EnhancementProfile getDefaultProfile() const = 0;
    virtual bool setDefaultProfile(const QString& id) = 0;
    virtual QList<EnhancementProfile> getAllProfiles(const QueryOptions& options = {}) const = 0;
    virtual bool updateLastUsed(const QString& id, const QDateTime& timestamp = QDateTime::currentDateTime()) = 0;

    // Statistics
    virtual int getProfileCount() const = 0;
    virtual QString getMostUsedProfile() const = 0;

signals:
    void profileCreated(const QString& id);
    void profileUpdated(const QString& id);
    void profileDeleted(const QString& id);
    void defaultProfileChanged(const QString& id);
};

/**
 * @brief Main storage interface that combines all storage operations
 */
class IStorageManager : public QObject {
    Q_OBJECT

public:
    explicit IStorageManager(QObject* parent = nullptr) : QObject(parent) {}
    virtual ~IStorageManager() = default;

    // Storage component access
    virtual IRecordingStorage* getRecordingStorage() = 0;
    virtual ITranscriptionStorage* getTranscriptionStorage() = 0;
    virtual IEnhancedTextStorage* getEnhancedTextStorage() = 0;
    virtual IUserSessionStorage* getUserSessionStorage() = 0;
    virtual IEnhancementProfileStorage* getProfileStorage() = 0;

    // Database Management
    virtual bool initialize(const QString& databasePath) = 0;
    virtual bool close() = 0;
    virtual bool isConnected() const = 0;
    virtual QString getDatabasePath() const = 0;

    // Transaction Management
    virtual bool beginTransaction() = 0;
    virtual bool commitTransaction() = 0;
    virtual bool rollbackTransaction() = 0;

    // Backup & Restore
    virtual bool backupDatabase(const QString& backupPath) = 0;
    virtual bool restoreDatabase(const QString& backupPath) = 0;
    virtual QStringList getAvailableBackups(const QString& backupDir) const = 0;

    // Database Maintenance
    virtual bool vacuum() = 0;
    virtual bool analyze() = 0;
    virtual qint64 getDatabaseSize() const = 0;
    virtual bool checkIntegrity() const = 0;

    // Migration
    virtual int getCurrentSchemaVersion() const = 0;
    virtual bool migrateToVersion(int version) = 0;
    virtual QStringList getPendingMigrations() const = 0;

    // Error Handling
    virtual StorageError getLastError() const = 0;
    virtual QString getErrorString() const = 0;
    virtual void clearErrorState() = 0;

    // Privacy & Security
    virtual bool enableEncryption(const QString& password) = 0;
    virtual bool changeEncryptionPassword(const QString& oldPassword, const QString& newPassword) = 0;
    virtual bool isEncrypted() const = 0;

signals:
    void databaseConnected();
    void databaseDisconnected();
    void errorOccurred(StorageError error, const QString& errorMessage);
    void backupCompleted(const QString& backupPath);
    void migrationProgress(int currentVersion, int targetVersion);
};

/**
 * @brief Factory interface for creating storage managers
 */
class IStorageManagerFactory {
public:
    virtual ~IStorageManagerFactory() = default;
    virtual IStorageManager* createStorageManager() = 0;
    virtual QStringList getSupportedDatabaseTypes() const = 0;
    virtual bool isDatabaseTypeSupported(const QString& type) const = 0;
};

// Contract Test Requirements:
// 1. Test CRUD operations for all entities
// 2. Test query performance with large datasets
// 3. Test transaction rollback on failures
// 4. Test database backup and restore functionality
// 5. Test encryption and privacy features (FR-015)
// 6. Test data integrity constraints
// 7. Test concurrent access from multiple threads
// 8. Test database migration scenarios
// 9. Test error handling for storage failures
// 10. Test search functionality with various filters
// 11. Test storage cleanup and maintenance
// 12. Test database size and performance monitoring

