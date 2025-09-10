#pragma once

#include "../models/BaseModel.h"
#include "../models/Recording.h"
#include "../models/Transcription.h"
#include "../models/EnhancedText.h"
#include "../models/UserSession.h"
#include "../models/EnhancementProfile.h"
#include "../../specs/001-voice-to-text/contracts/storage-interface.h"
#include <QObject>
#include <QString>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QMutex>
#include <QMutexLocker>
#include <QTimer>
#include <QDir>
#include <QFileInfo>

/**
 * @brief Recording storage implementation using SQLite
 */
class RecordingStorage : public IRecordingStorage {
    Q_OBJECT

public:
    explicit RecordingStorage(QSqlDatabase* database, QObject* parent = nullptr);

    // CRUD Operations
    QString saveRecording(const Recording& recording) override;
    Recording getRecording(const QString& id) const override;
    bool updateRecording(const Recording& recording) override;
    bool deleteRecording(const QString& id) override;
    bool recordingExists(const QString& id) const override;

    // Query Operations
    QList<Recording> getAllRecordings(const QueryOptions& options = {}) const override;
    QList<Recording> getRecordingsBySession(const QString& sessionId, const QueryOptions& options = {}) const override;
    QList<Recording> getRecordingsByDateRange(const QDateTime& start, const QDateTime& end) const override;
    QList<Recording> searchRecordings(const QString& searchTerm, const QueryOptions& options = {}) const override;

    // Statistics
    int getRecordingCount() const override;
    qint64 getTotalRecordingDuration() const override;
    qint64 getTotalStorageUsed() const override;
    QDateTime getOldestRecordingDate() const override;
    QDateTime getNewestRecordingDate() const override;

    // Maintenance
    bool cleanup() override;
    bool vacuum() override;
    QStringList getOrphanedAudioFiles() const override;

private:
    QSqlDatabase* m_database;
    mutable QMutex m_mutex;

    QString buildWhereClause(const QueryOptions& options) const;
    QString buildOrderClause(const QueryOptions& options) const;
    Recording recordingFromQuery(const QSqlQuery& query) const;
    bool executeQuery(QSqlQuery& query) const;
};

/**
 * @brief Transcription storage implementation using SQLite
 */
class TranscriptionStorage : public ITranscriptionStorage {
    Q_OBJECT

public:
    explicit TranscriptionStorage(QSqlDatabase* database, QObject* parent = nullptr);

    // CRUD Operations
    QString saveTranscription(const Transcription& transcription) override;
    Transcription getTranscription(const QString& id) const override;
    bool updateTranscription(const Transcription& transcription) override;
    bool deleteTranscription(const QString& id) override;
    bool transcriptionExists(const QString& id) const override;

    // Query Operations
    QList<Transcription> getAllTranscriptions(const QueryOptions& options = {}) const override;
    Transcription getTranscriptionByRecording(const QString& recordingId) const override;
    QList<Transcription> searchTranscriptions(const QString& searchTerm, const QueryOptions& options = {}) const override;
    QList<Transcription> getTranscriptionsByProvider(const QString& provider) const override;

    // Statistics
    int getTranscriptionCount() const override;
    double getAverageConfidence() const override;
    qint64 getAverageProcessingTime() const override;

private:
    QSqlDatabase* m_database;
    mutable QMutex m_mutex;

    Transcription transcriptionFromQuery(const QSqlQuery& query) const;
    bool executeQuery(QSqlQuery& query) const;
};

/**
 * @brief Enhanced text storage implementation using SQLite
 */
class EnhancedTextStorage : public IEnhancedTextStorage {
    Q_OBJECT

public:
    explicit EnhancedTextStorage(QSqlDatabase* database, QObject* parent = nullptr);

    // CRUD Operations
    QString saveEnhancedText(const EnhancedText& enhancedText) override;
    EnhancedText getEnhancedText(const QString& id) const override;
    bool updateEnhancedText(const EnhancedText& enhancedText) override;
    bool deleteEnhancedText(const QString& id) override;
    bool enhancedTextExists(const QString& id) const override;

    // Query Operations
    QList<EnhancedText> getAllEnhancedTexts(const QueryOptions& options = {}) const override;
    QList<EnhancedText> getEnhancedTextsByTranscription(const QString& transcriptionId) const override;
    QList<EnhancedText> getEnhancedTextsByMode(int enhancementMode) const override;
    QList<EnhancedText> getEnhancedTextsByProvider(const QString& provider) const override;

    // Statistics
    int getEnhancedTextCount() const override;
    qint64 getAverageProcessingTime() const override;
    double getAverageUserRating() const override;

private:
    QSqlDatabase* m_database;
    mutable QMutex m_mutex;

    EnhancedText enhancedTextFromQuery(const QSqlQuery& query) const;
    bool executeQuery(QSqlQuery& query) const;
};

/**
 * @brief User session storage implementation using SQLite
 */
class UserSessionStorage : public IUserSessionStorage {
    Q_OBJECT

public:
    explicit UserSessionStorage(QSqlDatabase* database, QObject* parent = nullptr);

    // CRUD Operations
    QString saveUserSession(const UserSession& session) override;
    UserSession getUserSession(const QString& id) const override;
    bool updateUserSession(const UserSession& session) override;
    bool deleteUserSession(const QString& id) override;
    bool userSessionExists(const QString& id) const override;

    // Session Management
    QString createNewSession(const QString& name) override;
    bool endSession(const QString& id, const QDateTime& endTime = QDateTime::currentDateTime()) override;
    UserSession getCurrentActiveSession() const override;
    QList<UserSession> getActiveSessions() const override;

    // Query Operations
    QList<UserSession> getAllSessions(const QueryOptions& options = {}) const override;
    QList<UserSession> getSessionsByDateRange(const QDateTime& start, const QDateTime& end) const override;
    QList<UserSession> searchSessions(const QString& searchTerm, const QueryOptions& options = {}) const override;

    // Statistics
    int getSessionCount() const override;
    qint64 getAverageSessionDuration() const override;
    int getAverageRecordingsPerSession() const override;

private:
    QSqlDatabase* m_database;
    mutable QMutex m_mutex;

    UserSession userSessionFromQuery(const QSqlQuery& query) const;
    bool executeQuery(QSqlQuery& query) const;
};

/**
 * @brief Enhancement profile storage implementation using SQLite
 */
class EnhancementProfileStorage : public IEnhancementProfileStorage {
    Q_OBJECT

public:
    explicit EnhancementProfileStorage(QSqlDatabase* database, QObject* parent = nullptr);

    // CRUD Operations
    QString saveProfile(const EnhancementProfile& profile) override;
    EnhancementProfile getProfile(const QString& id) const override;
    bool updateProfile(const EnhancementProfile& profile) override;
    bool deleteProfile(const QString& id) override;
    bool profileExists(const QString& id) const override;

    // Profile Management
    EnhancementProfile getDefaultProfile() const override;
    bool setDefaultProfile(const QString& id) override;
    QList<EnhancementProfile> getAllProfiles(const QueryOptions& options = {}) const override;
    bool updateLastUsed(const QString& id, const QDateTime& timestamp = QDateTime::currentDateTime()) override;

    // Statistics
    int getProfileCount() const override;
    QString getMostUsedProfile() const override;

private:
    QSqlDatabase* m_database;
    mutable QMutex m_mutex;

    EnhancementProfile enhancementProfileFromQuery(const QSqlQuery& query) const;
    bool executeQuery(QSqlQuery& query) const;
};

/**
 * @brief Main storage manager implementation using SQLite
 * 
 * Provides comprehensive data persistence functionality with SQLite backend.
 * Manages database connections, transactions, and coordinates all storage operations.
 */
class StorageManager : public IStorageManager {
    Q_OBJECT

public:
    explicit StorageManager(QObject* parent = nullptr);
    ~StorageManager() override;

    // Storage component access
    IRecordingStorage* getRecordingStorage() override;
    ITranscriptionStorage* getTranscriptionStorage() override;
    IEnhancedTextStorage* getEnhancedTextStorage() override;
    IUserSessionStorage* getUserSessionStorage() override;
    IEnhancementProfileStorage* getProfileStorage() override;

    // Database Management
    bool initialize(const QString& databasePath) override;
    bool close() override;
    bool isConnected() const override;
    QString getDatabasePath() const override;

    // Transaction Management
    bool beginTransaction() override;
    bool commitTransaction() override;
    bool rollbackTransaction() override;

    // Backup & Restore
    bool backupDatabase(const QString& backupPath) override;
    bool restoreDatabase(const QString& backupPath) override;
    QStringList getAvailableBackups(const QString& backupDir) const override;

    // Database Maintenance
    bool vacuum() override;
    bool analyze() override;
    qint64 getDatabaseSize() const override;
    bool checkIntegrity() const override;

    // Migration
    int getCurrentSchemaVersion() const override;
    bool migrateToVersion(int version) override;
    QStringList getPendingMigrations() const override;

    // Error Handling
    StorageError getLastError() const override;
    QString getErrorString() const override;
    void clearErrorState() override;

    // Privacy & Security
    bool enableEncryption(const QString& password) override;
    bool changeEncryptionPassword(const QString& oldPassword, const QString& newPassword) override;
    bool isEncrypted() const override;

private slots:
    void performMaintenance();

private:
    // Database components
    QSqlDatabase m_database;
    QString m_databasePath;
    bool m_isEncrypted;

    // Storage implementations
    RecordingStorage* m_recordingStorage;
    TranscriptionStorage* m_transcriptionStorage;
    EnhancedTextStorage* m_enhancedTextStorage;
    UserSessionStorage* m_userSessionStorage;
    EnhancementProfileStorage* m_profileStorage;

    // State management
    StorageError m_lastError;
    QString m_errorString;
    mutable QMutex m_transactionMutex;
    int m_transactionLevel;

    // Maintenance
    QTimer* m_maintenanceTimer;

    // Database setup and management
    bool createTables();
    bool createRecordingsTable();
    bool createTranscriptionsTable();
    bool createEnhancedTextsTable();
    bool createUserSessionsTable();
    bool createEnhancementProfilesTable();
    bool createMetadataTable();

    // Schema management
    bool createIndexes();
    bool createTriggers();
    bool updateSchemaVersion(int version);
    QStringList getSchemaUpdateQueries(int fromVersion, int toVersion);

    // Error handling
    void setError(StorageError error, const QString& errorMessage);
    void clearError();
    StorageError mapSqlError(const QSqlError& sqlError) const;

    // Utility methods
    bool executeSqlFile(const QString& filePath);
    bool executeSqlQuery(const QString& query);
    QString getConnectionName() const;
    bool validateDatabasePath(const QString& path) const;
    bool ensureDirectoryExists(const QString& path) const;

    // Backup helpers
    bool copyDatabaseFile(const QString& source, const QString& destination);
    bool validateBackupFile(const QString& backupPath) const;

    // Constants
    static constexpr int CURRENT_SCHEMA_VERSION = 1;
    static constexpr int MAINTENANCE_INTERVAL_MS = 3600000; // 1 hour
    static constexpr const char* DATABASE_CONNECTION_NAME = "QuillScribeMain";
    static constexpr const char* BACKUP_FILE_EXTENSION = ".backup";
    static constexpr const char* METADATA_TABLE_NAME = "metadata";
};
