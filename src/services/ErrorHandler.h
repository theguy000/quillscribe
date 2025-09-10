#pragma once

#include <QObject>
#include <QString>
#include <QTimer>
#include <QMessageBox>
#include <QSystemTrayIcon>

#include "../../specs/001-voice-to-text/contracts/audio-recording-interface.h"
#include "../../specs/001-voice-to-text/contracts/transcription-service-interface.h"
#include "../../specs/001-voice-to-text/contracts/ai-enhancement-interface.h"
#include "../../specs/001-voice-to-text/contracts/storage-interface.h"

/**
 * @brief Centralized error handling and notification service
 * 
 * Provides consistent error handling, logging, and user notification
 * across the entire application.
 */
class ErrorHandler : public QObject {
    Q_OBJECT

public:
    enum class ErrorSeverity {
        Info,
        Warning,
        Critical,
        Fatal
    };

    enum class NotificationType {
        StatusBar,
        MessageBox,
        SystemTray,
        All
    };

    explicit ErrorHandler(QObject* parent = nullptr);

    // Error reporting methods
    void reportError(ErrorSeverity severity, const QString& title, const QString& message, 
                    NotificationType notification = NotificationType::All);
    void reportInfo(const QString& message, int timeout = 3000);
    void reportWarning(const QString& title, const QString& message);
    void reportCriticalError(const QString& title, const QString& message);
    void reportFatalError(const QString& title, const QString& message);

    // Service-specific error handlers
    void handleAudioError(AudioError audioError, const QString& message);
    void handleTranscriptionError(TranscriptionError transcriptionError, const QString& message);
    void handleEnhancementError(EnhancementError enhancementError, const QString& message);
    void handleStorageError(StorageError storageError, const QString& message);

    // Configuration
    void setParentWidget(QWidget* parent);
    void setSystemTrayIcon(QSystemTrayIcon* trayIcon);
    void enableLogging(bool enabled);
    void setLogFilePath(const QString& logPath);

signals:
    void errorOccurred(ErrorSeverity severity, const QString& title, const QString& message);
    void statusMessageRequested(const QString& message, int timeout);

private slots:
    void onErrorTimeout();

private:
    void showMessageBox(ErrorSeverity severity, const QString& title, const QString& message);
    void showSystemTrayNotification(const QString& title, const QString& message);
    void logError(ErrorSeverity severity, const QString& title, const QString& message);
    QString severityToString(ErrorSeverity severity) const;
    QMessageBox::Icon severityToIcon(ErrorSeverity severity) const;

    QWidget* m_parentWidget;
    QSystemTrayIcon* m_systemTrayIcon;
    bool m_loggingEnabled;
    QString m_logFilePath;
    QTimer* m_errorTimer;
};
