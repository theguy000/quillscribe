#pragma once

#include <QMainWindow>
#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QTextEdit>
#include <QComboBox>
#include <QProgressBar>
#include <QSlider>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QListWidget>
#include <QSplitter>
#include <QMenuBar>
#include <QStatusBar>
#include <QTimer>
#include <QElapsedTimer>
#include <QAudioDevice>
#include <QMediaDevices>
#include <memory>

// Include contract interfaces for result types
#include "../specs/001-voice-to-text/contracts/audio-recording-interface.h"
#include "../specs/001-voice-to-text/contracts/transcription-service-interface.h"
#include "../specs/001-voice-to-text/contracts/ai-enhancement-interface.h"
#include "../specs/001-voice-to-text/contracts/storage-interface.h"

// Forward declarations for services
class AudioRecorderService;
class TranscriptionService;
class TextEnhancementService;
class StorageManager;

// Forward declarations for models
class Recording;
class Transcription;
class EnhancedText;
class UserSession;

/**
 * @brief Main application window for QuillScribe
 * 
 * Provides the primary user interface for voice recording, transcription,
 * and AI enhancement functionality. Features a modern, intuitive design
 * with real-time feedback and session management.
 */
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private slots:
    // Recording control slots
    void onRecordButtonClicked();
    void onStopButtonClicked();
    void onPauseButtonClicked();
    
    // Service response slots
    void onRecordingStarted();
    void onRecordingStopped(const QString& filePath, qint64 duration);
    void onRecordingPaused();
    void onRecordingResumed();
    void onRecordingError(AudioError error, const QString& errorMessage);
    
    void onInputLevelChanged(double level);
    void onRecordingDurationChanged(qint64 duration);
    
    // Transcription slots
    void onTranscriptionCompleted(const QString& requestId, const TranscriptionResult& result);
    void onTranscriptionFailed(const QString& requestId, TranscriptionError error, const QString& errorMessage);
    void onTranscriptionProgress(const QString& requestId, int progressPercent);
    
    // Enhancement slots
    void onEnhancementCompleted(const QString& requestId, const EnhancementResult& result);
    void onEnhancementFailed(const QString& requestId, EnhancementError error, const QString& errorMessage);
    void onEnhancementProgress(const QString& requestId, int progressPercent);
    
    // UI interaction slots
    void onDeviceSelectionChanged();
    void onTranscriptionProviderChanged();
    void onEnhancementModeChanged();
    void onSessionSelectionChanged();
    void onRecordingSelectionChanged();
    void onEnhanceButtonClicked();
    void onClearButtonClicked();
    void onSaveButtonClicked();
    void onSettingsButtonClicked();
    
    // Menu action slots
    void onNewSessionAction();
    void onOpenSessionAction();
    void onSaveSessionAction();
    void onExportAction();
    void onPreferencesAction();
    void onAboutAction();
    
    // Storage and data update slots
    void onDatabaseConnected();
    void onDatabaseDisconnected();
    void onStorageError(StorageError error, const QString& errorMessage);
    void onRecordingCreated(const QString& recordingId);
    void onRecordingUpdated(const QString& recordingId);
    void onRecordingDeleted(const QString& recordingId);
    void onSessionCreated(const QString& sessionId);
    void onSessionStarted(const QString& sessionId);
    void onSessionEnded(const QString& sessionId);
    
    // Configuration management slots
    void onSettingChanged(const QString& key, const QVariant& value);
    void onConfigurationLoaded();
    
    // Update slots
    void updateRecordingTimer();
    void updateUI();
    void updateStatusBar();

private:
    // Core services
    std::unique_ptr<AudioRecorderService> m_audioRecorderService;
    std::unique_ptr<TranscriptionService> m_transcriptionService;
    std::unique_ptr<TextEnhancementService> m_textEnhancementService;
    std::unique_ptr<StorageManager> m_storageManager;
    
    // Error handling
    class ErrorHandler* m_errorHandler;
    
    // Configuration management
    class ConfigurationManager* m_configManager;
    
    // Current state
    QString m_currentSessionId;
    QString m_currentRecordingId;
    QString m_currentTranscriptionId;
    bool m_isRecording;
    bool m_isPaused;
    QElapsedTimer m_recordingTimer;
    
    // UI Components - Main Layout
    QWidget* m_centralWidget;
    QSplitter* m_mainSplitter;
    
    // Recording Panel
    QGroupBox* m_recordingGroup;
    QPushButton* m_recordButton;
    QPushButton* m_pauseButton;
    QPushButton* m_stopButton;
    QLabel* m_recordingTimeLabel;
    QLabel* m_inputLevelLabel;
    QProgressBar* m_inputLevelBar;
    QComboBox* m_deviceComboBox;
    QLabel* m_recordingStatusLabel;
    
    // Transcription Panel
    QGroupBox* m_transcriptionGroup;
    QTextEdit* m_transcriptionTextEdit;
    QComboBox* m_transcriptionProviderCombo;
    QProgressBar* m_transcriptionProgressBar;
    QLabel* m_transcriptionStatusLabel;
    QPushButton* m_retranscribeButton;
    
    // Enhancement Panel
    QGroupBox* m_enhancementGroup;
    QTextEdit* m_enhancedTextEdit;
    QComboBox* m_enhancementModeCombo;
    QPushButton* m_enhanceButton;
    QProgressBar* m_enhancementProgressBar;
    QLabel* m_enhancementStatusLabel;
    
    // Session and History Panel
    QGroupBox* m_sessionGroup;
    QComboBox* m_sessionComboBox;
    QPushButton* m_newSessionButton;
    QListWidget* m_recordingHistoryList;
    QPushButton* m_clearButton;
    QPushButton* m_saveButton;
    
    // Control Panel
    QGroupBox* m_controlGroup;
    QPushButton* m_settingsButton;
    QSlider* m_inputGainSlider;
    QLabel* m_inputGainLabel;
    
    // Status and Progress
    QProgressBar* m_globalProgressBar;
    QTimer* m_uiUpdateTimer;
    
    // Menu and toolbar
    QMenuBar* m_menuBar;
    QMenu* m_fileMenu;
    QMenu* m_editMenu;
    QMenu* m_viewMenu;
    QMenu* m_toolsMenu;
    QMenu* m_helpMenu;
    
    QAction* m_newSessionAction;
    QAction* m_openSessionAction;
    QAction* m_saveSessionAction;
    QAction* m_exportAction;
    QAction* m_quitAction;
    QAction* m_preferencesAction;
    QAction* m_aboutAction;
    
    // Setup methods
    void setupUI();
    void setupMenuBar();
    void setupStatusBar();
    void setupRecordingPanel();
    void setupTranscriptionPanel();
    void setupEnhancementPanel();
    void setupSessionPanel();
    void setupControlPanel();
    void setupConnections();
    
    // Initialization methods
    void initializeServices();
    void initializeSettings();
    void loadSettings();
    void saveSettings();
    
    // UI update methods
    void updateRecordingControls();
    void updateTranscriptionControls();
    void updateEnhancementControls();
    void updateDeviceList();
    void updateSessionList();
    void updateRecordingHistory();
    
    // Recording management
    void startRecording();
    void stopRecording();
    void pauseRecording();
    void resumeRecording();
    bool validateRecordingSettings();
    
    // Transcription management
    void startTranscription(const QString& recordingId);
    void retranscribe();
    bool validateTranscriptionSettings();
    
    // Enhancement management
    void startEnhancement(const QString& transcriptionId);
    bool validateEnhancementSettings();
    
    // Session management
    void createNewSession();
    void loadSession(const QString& sessionId);
    void saveCurrentSession();
    QString getCurrentSessionName() const;
    
    // Data management
    void loadRecording(const QString& recordingId);
    void displayTranscription(const Transcription& transcription);
    void displayEnhancement(const EnhancedText& enhancement);
    void clearCurrentDisplay();
    
    // Utility methods
    QString formatDuration(qint64 milliseconds) const;
    QString formatFileSize(qint64 bytes) const;
    void showStatusMessage(const QString& message, int timeout = 5000);
    void showErrorMessage(const QString& title, const QString& message);
    void showSuccessMessage(const QString& message);
    
    // Settings management
    void applyTheme();
    void setupApplicationStyle();
    QSize getOptimalWindowSize() const;
    
    // Constants
    static constexpr int UI_UPDATE_INTERVAL_MS = 100;
    static constexpr int DEFAULT_WINDOW_WIDTH = 1200;
    static constexpr int DEFAULT_WINDOW_HEIGHT = 800;
    static constexpr int MIN_WINDOW_WIDTH = 800;
    static constexpr int MIN_WINDOW_HEIGHT = 600;
    static constexpr int STATUS_MESSAGE_TIMEOUT = 5000;
};
