#include "ErrorHandler.h"
#include <QDebug>
#include <QDateTime>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QApplication>

ErrorHandler::ErrorHandler(QObject* parent)
    : QObject(parent)
    , m_parentWidget(nullptr)
    , m_systemTrayIcon(nullptr)
    , m_loggingEnabled(true)
    , m_errorTimer(new QTimer(this))
{
    // Setup default log path
    QString appDataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(appDataDir);
    m_logFilePath = QDir(appDataDir).absoluteFilePath("quillscribe.log");
    
    m_errorTimer->setSingleShot(true);
    connect(m_errorTimer, &QTimer::timeout, this, &ErrorHandler::onErrorTimeout);
}

void ErrorHandler::reportError(ErrorSeverity severity, const QString& title, const QString& message, NotificationType notification) {
    // Log the error
    if (m_loggingEnabled) {
        logError(severity, title, message);
    }
    
    // Emit signal for other components
    emit errorOccurred(severity, title, message);
    
    // Handle notifications based on type
    switch (notification) {
        case NotificationType::MessageBox:
            showMessageBox(severity, title, message);
            break;
        case NotificationType::SystemTray:
            showSystemTrayNotification(title, message);
            break;
        case NotificationType::StatusBar:
            emit statusMessageRequested(message, 5000);
            break;
        case NotificationType::All:
            if (severity >= ErrorSeverity::Critical) {
                showMessageBox(severity, title, message);
            } else if (severity == ErrorSeverity::Warning) {
                showSystemTrayNotification(title, message);
            }
            emit statusMessageRequested(message, 3000);
            break;
    }
}

void ErrorHandler::reportInfo(const QString& message, int timeout) {
    reportError(ErrorSeverity::Info, "Information", message, NotificationType::StatusBar);
    emit statusMessageRequested(message, timeout);
}

void ErrorHandler::reportWarning(const QString& title, const QString& message) {
    reportError(ErrorSeverity::Warning, title, message, NotificationType::All);
}

void ErrorHandler::reportCriticalError(const QString& title, const QString& message) {
    reportError(ErrorSeverity::Critical, title, message, NotificationType::All);
}

void ErrorHandler::reportFatalError(const QString& title, const QString& message) {
    reportError(ErrorSeverity::Fatal, title, message, NotificationType::All);
    
    // For fatal errors, we might want to exit the application
    QTimer::singleShot(3000, qApp, &QApplication::quit);
}

void ErrorHandler::handleAudioError(AudioError audioError, const QString& message) {
    QString title;
    ErrorSeverity severity = ErrorSeverity::Warning;
    
    // Map audio error codes to user-friendly messages
    switch (audioError) {
        case AudioError::DeviceNotFound:
            title = "Audio Device Error";
            severity = ErrorSeverity::Critical;
            break;
        case AudioError::DeviceAccessDenied:
            title = "Audio Access Denied";
            severity = ErrorSeverity::Critical;
            break;
        case AudioError::FormatNotSupported:
            title = "Audio Format Error";
            severity = ErrorSeverity::Warning;
            break;
        case AudioError::IoError:
            title = "Audio I/O Error";
            severity = ErrorSeverity::Critical;
            break;
        case AudioError::InsufficientMemory:
            title = "Insufficient Memory";
            severity = ErrorSeverity::Critical;
            break;
        case AudioError::UnknownError:
            title = "Unknown Audio Error";
            severity = ErrorSeverity::Warning;
            break;
        default:
            title = "Audio Error";
            severity = ErrorSeverity::Warning;
            break;
    }
    
    reportError(severity, title, message);
}

void ErrorHandler::handleTranscriptionError(TranscriptionError transcriptionError, const QString& message) {
    QString title;
    ErrorSeverity severity = ErrorSeverity::Warning;
    
    switch (transcriptionError) {
        case TranscriptionError::ModelNotFound:
            title = "Transcription Model Error";
            severity = ErrorSeverity::Critical;
            break;
        case TranscriptionError::ModelLoadError:
            title = "Model Loading Error";
            severity = ErrorSeverity::Critical;
            break;
        case TranscriptionError::InvalidAudioFile:
            title = "Invalid Audio File";
            severity = ErrorSeverity::Warning;
            break;
        case TranscriptionError::AudioFormatError:
            title = "Audio Format Error";
            severity = ErrorSeverity::Warning;
            break;
        case TranscriptionError::TimeoutError:
            title = "Transcription Timeout";
            severity = ErrorSeverity::Warning;
            break;
        case TranscriptionError::ProcessingError:
            title = "Processing Error";
            severity = ErrorSeverity::Warning;
            break;
        case TranscriptionError::FileTooLarge:
            title = "File Too Large";
            severity = ErrorSeverity::Warning;
            break;
        case TranscriptionError::InsufficientMemory:
            title = "Insufficient Memory";
            severity = ErrorSeverity::Critical;
            break;
        default:
            title = "Transcription Error";
            severity = ErrorSeverity::Warning;
            break;
    }
    
    reportError(severity, title, message);
}

void ErrorHandler::handleEnhancementError(EnhancementError enhancementError, const QString& message) {
    QString title;
    ErrorSeverity severity = ErrorSeverity::Warning;
    
    switch (enhancementError) {
        case EnhancementError::InvalidApiKey:
            title = "API Key Error";
            severity = ErrorSeverity::Critical;
            break;
        case EnhancementError::NetworkError:
            title = "Network Error";
            severity = ErrorSeverity::Warning;
            break;
        case EnhancementError::ServiceUnavailable:
            title = "Service Unavailable";
            severity = ErrorSeverity::Warning;
            break;
        case EnhancementError::QuotaExceeded:
            title = "Quota Exceeded";
            severity = ErrorSeverity::Warning;
            break;
        case EnhancementError::TextTooLong:
            title = "Text Too Long";
            severity = ErrorSeverity::Info;
            break;
        case EnhancementError::AuthenticationError:
            title = "Authentication Error";
            severity = ErrorSeverity::Critical;
            break;
        case EnhancementError::InvalidPrompt:
            title = "Invalid Prompt";
            severity = ErrorSeverity::Warning;
            break;
        case EnhancementError::TimeoutError:
            title = "Request Timeout";
            severity = ErrorSeverity::Warning;
            break;
        case EnhancementError::ContentFiltered:
            title = "Content Filtered";
            severity = ErrorSeverity::Warning;
            break;
        default:
            title = "Enhancement Error";
            severity = ErrorSeverity::Warning;
            break;
    }
    
    reportError(severity, title, message);
}

void ErrorHandler::handleStorageError(StorageError storageError, const QString& message) {
    QString title;
    ErrorSeverity severity = ErrorSeverity::Warning;
    
    switch (storageError) {
        case StorageError::DatabaseConnectionFailed:
            title = "Database Connection Error";
            severity = ErrorSeverity::Critical;
            break;
        case StorageError::TableCreationFailed:
            title = "Database Setup Error";
            severity = ErrorSeverity::Critical;
            break;
        case StorageError::QueryFailed:
            title = "Database Query Error";
            severity = ErrorSeverity::Warning;
            break;
        case StorageError::InsertFailed:
            title = "Database Insert Error";
            severity = ErrorSeverity::Warning;
            break;
        case StorageError::UpdateFailed:
            title = "Database Update Error";
            severity = ErrorSeverity::Warning;
            break;
        case StorageError::DeleteFailed:
            title = "Database Delete Error";
            severity = ErrorSeverity::Warning;
            break;
        case StorageError::RecordNotFound:
            title = "Record Not Found";
            severity = ErrorSeverity::Info;
            break;
        case StorageError::ConstraintViolation:
            title = "Data Constraint Error";
            severity = ErrorSeverity::Warning;
            break;
        case StorageError::DiskSpaceInsufficient:
            title = "Insufficient Disk Space";
            severity = ErrorSeverity::Critical;
            break;
        default:
            title = "Storage Error";
            severity = ErrorSeverity::Warning;
            break;
    }
    
    reportError(severity, title, message);
}

void ErrorHandler::setParentWidget(QWidget* parent) {
    m_parentWidget = parent;
}

void ErrorHandler::setSystemTrayIcon(QSystemTrayIcon* trayIcon) {
    m_systemTrayIcon = trayIcon;
}

void ErrorHandler::enableLogging(bool enabled) {
    m_loggingEnabled = enabled;
}

void ErrorHandler::setLogFilePath(const QString& logPath) {
    m_logFilePath = logPath;
}

void ErrorHandler::onErrorTimeout() {
    // Handle error timeouts if needed
}

void ErrorHandler::showMessageBox(ErrorSeverity severity, const QString& title, const QString& message) {
    if (!m_parentWidget) {
        return;
    }
    
    QMessageBox::Icon icon = severityToIcon(severity);
    QMessageBox msgBox(icon, title, message, QMessageBox::Ok, m_parentWidget);
    
    // For fatal errors, add additional buttons
    if (severity == ErrorSeverity::Fatal) {
        msgBox.addButton("Restart", QMessageBox::AcceptRole);
        msgBox.addButton("Exit", QMessageBox::RejectRole);
    }
    
    int result = msgBox.exec();
    
    if (severity == ErrorSeverity::Fatal && result == 0) {
        // Restart requested
        QApplication::quit();
        // In a real implementation, this would restart the application
    }
}

void ErrorHandler::showSystemTrayNotification(const QString& title, const QString& message) {
    if (!m_systemTrayIcon || !m_systemTrayIcon->isVisible()) {
        return;
    }
    
    m_systemTrayIcon->showMessage(title, message, QSystemTrayIcon::Warning, 5000);
}

void ErrorHandler::logError(ErrorSeverity severity, const QString& title, const QString& message) {
    QFile logFile(m_logFilePath);
    if (!logFile.open(QIODevice::WriteOnly | QIODevice::Append)) {
        return;
    }
    
    QTextStream stream(&logFile);
    QString timestamp = QDateTime::currentDateTime().toString(Qt::ISODate);
    QString severityStr = severityToString(severity);
    
    stream << QString("[%1] %2: %3 - %4")
              .arg(timestamp, severityStr, title, message) << Qt::endl;
}

QString ErrorHandler::severityToString(ErrorSeverity severity) const {
    switch (severity) {
        case ErrorSeverity::Info: return "INFO";
        case ErrorSeverity::Warning: return "WARNING";
        case ErrorSeverity::Critical: return "CRITICAL";
        case ErrorSeverity::Fatal: return "FATAL";
        default: return "UNKNOWN";
    }
}

QMessageBox::Icon ErrorHandler::severityToIcon(ErrorSeverity severity) const {
    switch (severity) {
        case ErrorSeverity::Info: return QMessageBox::Information;
        case ErrorSeverity::Warning: return QMessageBox::Warning;
        case ErrorSeverity::Critical: return QMessageBox::Critical;
        case ErrorSeverity::Fatal: return QMessageBox::Critical;
        default: return QMessageBox::NoIcon;
    }
}
