#include "MainWindow.h"
#include "services/AudioRecorderService.h"
#include "services/TranscriptionService.h"
#include "services/TextEnhancementService.h"
#include "services/StorageManager.h"
#include "services/ErrorHandler.h"
#include "services/ConfigurationManager.h"
#include "models/Recording.h"
#include "models/Transcription.h"
#include "models/EnhancedText.h"
#include "models/UserSession.h"

#include <QApplication>
#include <QCloseEvent>
#include <QResizeEvent>
#include <QSettings>
#include <QStandardPaths>
#include <QMessageBox>
#include <QFileDialog>
#include <QSplitter>
#include <QGroupBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QTextEdit>
#include <QComboBox>
#include <QProgressBar>
#include <QSlider>
#include <QListWidget>
#include <QMenuBar>
#include <QStatusBar>
#include <QTimer>
#include <QDebug>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_centralWidget(nullptr)
    , m_mainSplitter(nullptr)
    , m_isRecording(false)
    , m_isPaused(false)
    , m_uiUpdateTimer(new QTimer(this))
    , m_errorHandler(new ErrorHandler(this))
    , m_configManager(new ConfigurationManager(this))
{
    setWindowTitle("QuillScribe - Voice-to-Text with AI Enhancement");
    setMinimumSize(MIN_WINDOW_WIDTH, MIN_WINDOW_HEIGHT);
    resize(getOptimalWindowSize());

    // Setup error handler
    m_errorHandler->setParentWidget(this);
    connect(m_errorHandler, &ErrorHandler::statusMessageRequested, this, &MainWindow::showStatusMessage);
    
    // Setup configuration manager
    connect(m_configManager, &ConfigurationManager::settingChanged, this, &MainWindow::onSettingChanged);
    connect(m_configManager, &ConfigurationManager::configurationLoaded, this, &MainWindow::onConfigurationLoaded);
    
    // Initialize services first
    initializeServices();
    
    // Setup UI
    setupUI();
    setupMenuBar();
    setupStatusBar();
    setupConnections();
    
    // Initialize settings and load data
    initializeSettings();
    loadSettings();
    
    // Start UI update timer
    m_uiUpdateTimer->setInterval(UI_UPDATE_INTERVAL_MS);
    connect(m_uiUpdateTimer, &QTimer::timeout, this, &MainWindow::updateUI);
    m_uiUpdateTimer->start();
    
    // Create initial session if none exists
    if (m_currentSessionId.isEmpty()) {
        createNewSession();
    }
    
    // Apply styling
    setupApplicationStyle();
    
    showStatusMessage("QuillScribe ready - Click Record to start", 3000);
}

MainWindow::~MainWindow() {
    saveSettings();
    
    // Stop any ongoing operations
    if (m_isRecording) {
        stopRecording();
    }
}

void MainWindow::closeEvent(QCloseEvent* event) {
    // Save current session if recording exists
    if (m_isRecording) {
        QMessageBox::StandardButton reply = QMessageBox::question(
            this, "Recording in Progress",
            "A recording is in progress. Stop recording and exit?",
            QMessageBox::Yes | QMessageBox::No);
        
        if (reply == QMessageBox::Yes) {
            stopRecording();
        } else {
            event->ignore();
            return;
        }
    }
    
    saveCurrentSession();
    saveSettings();
    event->accept();
}

void MainWindow::resizeEvent(QResizeEvent* event) {
    QMainWindow::resizeEvent(event);
    // Could handle responsive layout changes here
}

void MainWindow::setupUI() {
    m_centralWidget = new QWidget;
    setCentralWidget(m_centralWidget);
    
    // Create main layout
    QHBoxLayout* mainLayout = new QHBoxLayout(m_centralWidget);
    
    // Create main splitter
    m_mainSplitter = new QSplitter(Qt::Horizontal);
    mainLayout->addWidget(m_mainSplitter);
    
    // Setup panels
    setupRecordingPanel();
    setupTranscriptionPanel();
    setupEnhancementPanel();
    setupSessionPanel();
    setupControlPanel();
    
    // Create left panel (recording + control)
    QWidget* leftPanel = new QWidget;
    QVBoxLayout* leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->addWidget(m_recordingGroup);
    leftLayout->addWidget(m_controlGroup);
    leftLayout->addStretch();
    
    // Create center panel (transcription + enhancement)
    QWidget* centerPanel = new QWidget;
    QVBoxLayout* centerLayout = new QVBoxLayout(centerPanel);
    centerLayout->addWidget(m_transcriptionGroup);
    centerLayout->addWidget(m_enhancementGroup);
    
    // Add panels to splitter
    m_mainSplitter->addWidget(leftPanel);
    m_mainSplitter->addWidget(centerPanel);
    m_mainSplitter->addWidget(m_sessionGroup);
    
    // Set splitter proportions
    m_mainSplitter->setSizes({300, 600, 300});
    m_mainSplitter->setCollapsible(0, false);
    m_mainSplitter->setCollapsible(1, false);
    m_mainSplitter->setCollapsible(2, true);
}

void MainWindow::setupRecordingPanel() {
    m_recordingGroup = new QGroupBox("Voice Recording");
    QVBoxLayout* layout = new QVBoxLayout(m_recordingGroup);
    
    // Recording controls
    QHBoxLayout* buttonLayout = new QHBoxLayout;
    m_recordButton = new QPushButton("🎙️ Record");
    m_recordButton->setStyleSheet("QPushButton { background-color: #4CAF50; color: white; font-weight: bold; padding: 10px; }");
    m_pauseButton = new QPushButton("⏸️ Pause");
    m_pauseButton->setEnabled(false);
    m_stopButton = new QPushButton("⏹️ Stop");
    m_stopButton->setEnabled(false);
    
    buttonLayout->addWidget(m_recordButton);
    buttonLayout->addWidget(m_pauseButton);
    buttonLayout->addWidget(m_stopButton);
    
    // Recording time display
    m_recordingTimeLabel = new QLabel("00:00:00");
    m_recordingTimeLabel->setAlignment(Qt::AlignCenter);
    m_recordingTimeLabel->setStyleSheet("QLabel { font-size: 18px; font-weight: bold; color: #333; }");
    
    // Input level monitoring
    QLabel* levelLabel = new QLabel("Input Level:");
    m_inputLevelBar = new QProgressBar;
    m_inputLevelBar->setRange(0, 100);
    m_inputLevelBar->setValue(0);
    m_inputLevelBar->setStyleSheet("QProgressBar::chunk { background-color: #4CAF50; }");
    
    // Device selection
    QLabel* deviceLabel = new QLabel("Recording Device:");
    m_deviceComboBox = new QComboBox;
    updateDeviceList();
    
    // Status
    m_recordingStatusLabel = new QLabel("Ready");
    m_recordingStatusLabel->setStyleSheet("QLabel { color: #666; font-style: italic; }");
    
    // Add to layout
    layout->addLayout(buttonLayout);
    layout->addWidget(m_recordingTimeLabel);
    layout->addWidget(levelLabel);
    layout->addWidget(m_inputLevelBar);
    layout->addWidget(deviceLabel);
    layout->addWidget(m_deviceComboBox);
    layout->addWidget(m_recordingStatusLabel);
}

void MainWindow::setupTranscriptionPanel() {
    m_transcriptionGroup = new QGroupBox("Speech-to-Text Transcription");
    QVBoxLayout* layout = new QVBoxLayout(m_transcriptionGroup);
    
    // Provider selection
    QHBoxLayout* providerLayout = new QHBoxLayout;
    QLabel* providerLabel = new QLabel("Transcription Provider:");
    m_transcriptionProviderCombo = new QComboBox;
    m_transcriptionProviderCombo->addItem("Whisper Tiny (Fast)", static_cast<int>(TranscriptionProvider::WhisperCppTiny));
    m_transcriptionProviderCombo->addItem("Whisper Base (Balanced)", static_cast<int>(TranscriptionProvider::WhisperCppBase));
    m_transcriptionProviderCombo->addItem("Whisper Small (Accurate)", static_cast<int>(TranscriptionProvider::WhisperCppSmall));
    m_transcriptionProviderCombo->setCurrentIndex(1); // Base as default
    
    providerLayout->addWidget(providerLabel);
    providerLayout->addWidget(m_transcriptionProviderCombo);
    providerLayout->addStretch();
    
    // Progress bar
    m_transcriptionProgressBar = new QProgressBar;
    m_transcriptionProgressBar->setVisible(false);
    
    // Status label
    m_transcriptionStatusLabel = new QLabel("No transcription");
    m_transcriptionStatusLabel->setStyleSheet("QLabel { color: #666; font-style: italic; }");
    
    // Text display
    m_transcriptionTextEdit = new QTextEdit;
    m_transcriptionTextEdit->setPlaceholderText("Transcribed text will appear here after recording...");
    m_transcriptionTextEdit->setMinimumHeight(150);
    
    // Re-transcribe button
    m_retranscribeButton = new QPushButton("🔄 Re-transcribe");
    m_retranscribeButton->setEnabled(false);
    
    // Add to layout
    layout->addLayout(providerLayout);
    layout->addWidget(m_transcriptionProgressBar);
    layout->addWidget(m_transcriptionStatusLabel);
    layout->addWidget(m_transcriptionTextEdit);
    layout->addWidget(m_retranscribeButton);
}

void MainWindow::setupEnhancementPanel() {
    m_enhancementGroup = new QGroupBox("AI Text Enhancement");
    QVBoxLayout* layout = new QVBoxLayout(m_enhancementGroup);
    
    // Mode selection
    QHBoxLayout* modeLayout = new QHBoxLayout;
    QLabel* modeLabel = new QLabel("Enhancement Mode:");
    m_enhancementModeCombo = new QComboBox;
    m_enhancementModeCombo->addItem("Grammar Only", static_cast<int>(EnhancementMode::GrammarOnly));
    m_enhancementModeCombo->addItem("Style Improvement", static_cast<int>(EnhancementMode::StyleImprovement));
    m_enhancementModeCombo->addItem("Summarization", static_cast<int>(EnhancementMode::Summarization));
    m_enhancementModeCombo->addItem("Formalization", static_cast<int>(EnhancementMode::Formalization));
    m_enhancementModeCombo->addItem("Custom", static_cast<int>(EnhancementMode::Custom));
    m_enhancementModeCombo->setCurrentIndex(1); // Style improvement as default
    
    modeLayout->addWidget(modeLabel);
    modeLayout->addWidget(m_enhancementModeCombo);
    modeLayout->addStretch();
    
    // Enhance button
    m_enhanceButton = new QPushButton("✨ Enhance Text");
    m_enhanceButton->setStyleSheet("QPushButton { background-color: #2196F3; color: white; font-weight: bold; padding: 8px; }");
    m_enhanceButton->setEnabled(false);
    
    // Progress bar
    m_enhancementProgressBar = new QProgressBar;
    m_enhancementProgressBar->setVisible(false);
    
    // Status label
    m_enhancementStatusLabel = new QLabel("No enhancement");
    m_enhancementStatusLabel->setStyleSheet("QLabel { color: #666; font-style: italic; }");
    
    // Text display
    m_enhancedTextEdit = new QTextEdit;
    m_enhancedTextEdit->setPlaceholderText("AI-enhanced text will appear here after enhancement...");
    m_enhancedTextEdit->setMinimumHeight(150);
    
    // Add to layout
    layout->addLayout(modeLayout);
    layout->addWidget(m_enhanceButton);
    layout->addWidget(m_enhancementProgressBar);
    layout->addWidget(m_enhancementStatusLabel);
    layout->addWidget(m_enhancedTextEdit);
}

void MainWindow::setupSessionPanel() {
    m_sessionGroup = new QGroupBox("Session & History");
    QVBoxLayout* layout = new QVBoxLayout(m_sessionGroup);
    
    // Session selection
    QHBoxLayout* sessionLayout = new QHBoxLayout;
    QLabel* sessionLabel = new QLabel("Current Session:");
    m_sessionComboBox = new QComboBox;
    m_newSessionButton = new QPushButton("📋 New");
    
    sessionLayout->addWidget(sessionLabel);
    sessionLayout->addWidget(m_sessionComboBox);
    sessionLayout->addWidget(m_newSessionButton);
    
    // Recording history
    QLabel* historyLabel = new QLabel("Recording History:");
    m_recordingHistoryList = new QListWidget;
    m_recordingHistoryList->setMaximumHeight(200);
    
    // Action buttons
    QHBoxLayout* actionLayout = new QHBoxLayout;
    m_clearButton = new QPushButton("🗑️ Clear");
    m_saveButton = new QPushButton("💾 Save");
    
    actionLayout->addWidget(m_clearButton);
    actionLayout->addWidget(m_saveButton);
    actionLayout->addStretch();
    
    // Add to layout
    layout->addLayout(sessionLayout);
    layout->addWidget(historyLabel);
    layout->addWidget(m_recordingHistoryList);
    layout->addLayout(actionLayout);
    layout->addStretch();
}

void MainWindow::setupControlPanel() {
    m_controlGroup = new QGroupBox("Controls");
    QVBoxLayout* layout = new QVBoxLayout(m_controlGroup);
    
    // Settings button
    m_settingsButton = new QPushButton("⚙️ Settings");
    
    // Input gain control
    QLabel* gainLabel = new QLabel("Input Gain:");
    m_inputGainSlider = new QSlider(Qt::Horizontal);
    m_inputGainSlider->setRange(0, 200);
    m_inputGainSlider->setValue(100);
    m_inputGainLabel = new QLabel("100%");
    
    QHBoxLayout* gainLayout = new QHBoxLayout;
    gainLayout->addWidget(m_inputGainSlider);
    gainLayout->addWidget(m_inputGainLabel);
    
    // Add to layout
    layout->addWidget(m_settingsButton);
    layout->addWidget(gainLabel);
    layout->addLayout(gainLayout);
}

void MainWindow::setupMenuBar() {
    m_menuBar = menuBar();
    
    // File menu
    m_fileMenu = m_menuBar->addMenu("&File");
    m_newSessionAction = m_fileMenu->addAction("&New Session", this, &MainWindow::onNewSessionAction);
    m_newSessionAction->setShortcut(QKeySequence::New);
    
    m_openSessionAction = m_fileMenu->addAction("&Open Session...", this, &MainWindow::onOpenSessionAction);
    m_openSessionAction->setShortcut(QKeySequence::Open);
    
    m_saveSessionAction = m_fileMenu->addAction("&Save Session", this, &MainWindow::onSaveSessionAction);
    m_saveSessionAction->setShortcut(QKeySequence::Save);
    
    m_fileMenu->addSeparator();
    
    m_exportAction = m_fileMenu->addAction("&Export...", this, &MainWindow::onExportAction);
    m_exportAction->setShortcut(QKeySequence("Ctrl+E"));
    
    m_fileMenu->addSeparator();
    
    m_quitAction = m_fileMenu->addAction("&Quit", this, &QWidget::close);
    m_quitAction->setShortcut(QKeySequence::Quit);
    
    // Edit menu
    m_editMenu = m_menuBar->addMenu("&Edit");
    m_preferencesAction = m_editMenu->addAction("&Preferences...", this, &MainWindow::onPreferencesAction);
    m_preferencesAction->setShortcut(QKeySequence::Preferences);
    
    // View menu
    m_viewMenu = m_menuBar->addMenu("&View");
    // Add view options later
    
    // Tools menu
    m_toolsMenu = m_menuBar->addMenu("&Tools");
    // Add tool options later
    
    // Help menu
    m_helpMenu = m_menuBar->addMenu("&Help");
    m_aboutAction = m_helpMenu->addAction("&About QuillScribe", this, &MainWindow::onAboutAction);
}

void MainWindow::setupStatusBar() {
    QStatusBar* status = statusBar();
    
    // Add global progress bar to status bar
    m_globalProgressBar = new QProgressBar;
    m_globalProgressBar->setMaximumWidth(200);
    m_globalProgressBar->setVisible(false);
    status->addPermanentWidget(m_globalProgressBar);
    
    showStatusMessage("Ready");
}

void MainWindow::setupConnections() {
    // Recording control connections
    connect(m_recordButton, &QPushButton::clicked, this, &MainWindow::onRecordButtonClicked);
    connect(m_pauseButton, &QPushButton::clicked, this, &MainWindow::onPauseButtonClicked);
    connect(m_stopButton, &QPushButton::clicked, this, &MainWindow::onStopButtonClicked);
    
    // UI control connections
    connect(m_deviceComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onDeviceSelectionChanged);
    connect(m_transcriptionProviderCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onTranscriptionProviderChanged);
    connect(m_enhancementModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onEnhancementModeChanged);
    connect(m_sessionComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onSessionSelectionChanged);
    connect(m_recordingHistoryList, &QListWidget::currentItemChanged, this, &MainWindow::onRecordingSelectionChanged);
    
    connect(m_enhanceButton, &QPushButton::clicked, this, &MainWindow::onEnhanceButtonClicked);
    connect(m_retranscribeButton, &QPushButton::clicked, this, &MainWindow::retranscribe);
    connect(m_clearButton, &QPushButton::clicked, this, &MainWindow::onClearButtonClicked);
    connect(m_saveButton, &QPushButton::clicked, this, &MainWindow::onSaveButtonClicked);
    connect(m_settingsButton, &QPushButton::clicked, this, &MainWindow::onSettingsButtonClicked);
    connect(m_newSessionButton, &QPushButton::clicked, this, &MainWindow::onNewSessionAction);
    
    // Input gain slider
    connect(m_inputGainSlider, &QSlider::valueChanged, this, [this](int value) {
        double gain = value / 100.0;
        m_inputGainLabel->setText(QString("%1%").arg(value));
        if (m_audioRecorderService) {
            m_audioRecorderService->setInputGain(gain);
        }
        // Save to configuration
        m_configManager->setInputGain(value);
    });
    
    // Service connections will be setup in initializeServices()
}

void MainWindow::initializeServices() {
    // Initialize storage manager first
    m_storageManager = std::make_unique<StorageManager>();
    QString dbPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/quillscribe.db";
    if (!m_storageManager->initialize(dbPath)) {
        m_errorHandler->reportCriticalError("Database Initialization Failed", 
                                           "Failed to initialize database: " + m_storageManager->getErrorString());
        return;
    }
    
    // Initialize audio recorder service
    m_audioRecorderService = std::make_unique<AudioRecorderService>(m_storageManager.get());
    
    // Connect audio recorder signals
    connect(m_audioRecorderService.get(), &IAudioRecorder::recordingStarted, this, &MainWindow::onRecordingStarted);
    connect(m_audioRecorderService.get(), &IAudioRecorder::recordingStopped, this, &MainWindow::onRecordingStopped);
    connect(m_audioRecorderService.get(), &IAudioRecorder::recordingPaused, this, &MainWindow::onRecordingPaused);
    connect(m_audioRecorderService.get(), &IAudioRecorder::recordingResumed, this, &MainWindow::onRecordingResumed);
    connect(m_audioRecorderService.get(), &IAudioRecorder::errorOccurred, this, &MainWindow::onRecordingError);
    connect(m_audioRecorderService.get(), &IAudioRecorder::inputLevelChanged, this, &MainWindow::onInputLevelChanged);
    connect(m_audioRecorderService.get(), &IAudioRecorder::durationChanged, this, &MainWindow::onRecordingDurationChanged);
    
    // Initialize transcription service
    m_transcriptionService = std::make_unique<TranscriptionService>(m_storageManager.get());
    
    // Connect transcription signals
    connect(m_transcriptionService.get(), &ITranscriptionService::transcriptionCompleted, this, &MainWindow::onTranscriptionCompleted);
    connect(m_transcriptionService.get(), &ITranscriptionService::transcriptionFailed, this, &MainWindow::onTranscriptionFailed);
    connect(m_transcriptionService.get(), &ITranscriptionService::transcriptionProgress, this, &MainWindow::onTranscriptionProgress);
    
    // Initialize text enhancement service  
    m_textEnhancementService = std::make_unique<TextEnhancementService>();
    // Note: TextEnhancementService doesn't need StorageManager constructor as it uses QSettings
    
    // Connect enhancement signals
    connect(m_textEnhancementService.get(), &ITextEnhancementService::enhancementCompleted, this, &MainWindow::onEnhancementCompleted);
    connect(m_textEnhancementService.get(), &ITextEnhancementService::enhancementFailed, this, &MainWindow::onEnhancementFailed);
    connect(m_textEnhancementService.get(), &ITextEnhancementService::enhancementProgress, this, &MainWindow::onEnhancementProgress);
    
    // Connect storage manager signals for real-time UI updates
    connect(m_storageManager.get(), &IStorageManager::databaseConnected, this, &MainWindow::onDatabaseConnected);
    connect(m_storageManager.get(), &IStorageManager::databaseDisconnected, this, &MainWindow::onDatabaseDisconnected);
    connect(m_storageManager.get(), &IStorageManager::errorOccurred, this, &MainWindow::onStorageError);
    
    // Connect recording storage signals
    if (auto* recordingStorage = m_storageManager->getRecordingStorage()) {
        connect(recordingStorage, &IRecordingStorage::recordingCreated, this, &MainWindow::onRecordingCreated);
        connect(recordingStorage, &IRecordingStorage::recordingUpdated, this, &MainWindow::onRecordingUpdated);
        connect(recordingStorage, &IRecordingStorage::recordingDeleted, this, &MainWindow::onRecordingDeleted);
    }
    
    // Connect session storage signals  
    if (auto* sessionStorage = m_storageManager->getUserSessionStorage()) {
        connect(sessionStorage, &IUserSessionStorage::sessionCreated, this, &MainWindow::onSessionCreated);
        connect(sessionStorage, &IUserSessionStorage::sessionStarted, this, &MainWindow::onSessionStarted);
        connect(sessionStorage, &IUserSessionStorage::sessionEnded, this, &MainWindow::onSessionEnded);
    }
    
    qDebug() << "All services initialized successfully";
}

void MainWindow::initializeSettings() {
    // Set up application settings
    QCoreApplication::setOrganizationName("QuillScribe");
    QCoreApplication::setOrganizationDomain("quillscribe.app");
    QCoreApplication::setApplicationName("QuillScribe");
}

void MainWindow::loadSettings() {
    // Window geometry
    QByteArray geometry = m_configManager->getWindowGeometry();
    if (!geometry.isEmpty()) {
        restoreGeometry(geometry);
    }
    
    QByteArray windowState = m_configManager->getWindowState();
    if (!windowState.isEmpty()) {
        restoreState(windowState);
    }
    
    // Splitter state
    if (m_mainSplitter) {
        QByteArray splitterState = m_configManager->getSplitterState();
        if (!splitterState.isEmpty()) {
            m_mainSplitter->restoreState(splitterState);
        }
    }
    
    // Service settings
    QString lastSessionId = m_configManager->getCurrentSessionId();
    if (!lastSessionId.isEmpty()) {
        m_currentSessionId = lastSessionId;
    }
    
    // Input gain
    int inputGain = m_configManager->getInputGain();
    if (m_inputGainSlider) {
        m_inputGainSlider->setValue(inputGain);
    }
    
    // Provider selections
    int transcriptionProvider = m_configManager->getTranscriptionProvider();
    if (m_transcriptionProviderCombo) {
        m_transcriptionProviderCombo->setCurrentIndex(transcriptionProvider);
    }
    
    int enhancementMode = m_configManager->getEnhancementMode();
    if (m_enhancementModeCombo) {
        m_enhancementModeCombo->setCurrentIndex(enhancementMode);
    }
}

void MainWindow::saveSettings() {
    // Window geometry
    m_configManager->setWindowGeometry(saveGeometry());
    m_configManager->setWindowState(saveState());
    
    // Splitter state
    if (m_mainSplitter) {
        m_configManager->setSplitterState(m_mainSplitter->saveState());
    }
    
    // Service settings
    m_configManager->setCurrentSessionId(m_currentSessionId);
    
    if (m_inputGainSlider) {
        m_configManager->setInputGain(m_inputGainSlider->value());
    }
    
    if (m_transcriptionProviderCombo) {
        m_configManager->setTranscriptionProvider(m_transcriptionProviderCombo->currentIndex());
    }
    
    if (m_enhancementModeCombo) {
        m_configManager->setEnhancementMode(m_enhancementModeCombo->currentIndex());
    }
}

// Recording control slots
void MainWindow::onRecordButtonClicked() {
    if (m_isRecording) {
        if (m_isPaused) {
            resumeRecording();
        } else {
            pauseRecording();
        }
    } else {
        startRecording();
    }
}

void MainWindow::onStopButtonClicked() {
    stopRecording();
}

void MainWindow::onPauseButtonClicked() {
    if (m_isRecording && !m_isPaused) {
        pauseRecording();
    }
}

void MainWindow::onRecordingStarted() {
    m_isRecording = true;
    m_isPaused = false;
    m_recordingTimer.start();
    updateRecordingControls();
    showStatusMessage("Recording started");
}

void MainWindow::onRecordingStopped(const QString& filePath, qint64 duration) {
    Q_UNUSED(filePath)
    Q_UNUSED(duration)
    
    m_isRecording = false;
    m_isPaused = false;
    updateRecordingControls();
    showStatusMessage("Recording completed - Starting transcription...");
    
    // Start transcription automatically
    if (!m_currentRecordingId.isEmpty()) {
        startTranscription(m_currentRecordingId);
    }
}

void MainWindow::onRecordingPaused() {
    m_isPaused = true;
    updateRecordingControls();
    showStatusMessage("Recording paused");
}

void MainWindow::onRecordingResumed() {
    m_isPaused = false;
    updateRecordingControls();
    showStatusMessage("Recording resumed");
}

void MainWindow::onRecordingError(AudioError error, const QString& errorMessage) {
    m_isRecording = false;
    m_isPaused = false;
    updateRecordingControls();
    
    // Use centralized error handler
    m_errorHandler->handleAudioError(error, errorMessage);
}

void MainWindow::onInputLevelChanged(double level) {
    int levelPercent = static_cast<int>(level * 100);
    m_inputLevelBar->setValue(levelPercent);
    
    // Change color based on level
    if (level > 0.9) {
        m_inputLevelBar->setStyleSheet("QProgressBar::chunk { background-color: #f44336; }"); // Red for clipping
    } else if (level > 0.7) {
        m_inputLevelBar->setStyleSheet("QProgressBar::chunk { background-color: #ff9800; }"); // Orange for high
    } else {
        m_inputLevelBar->setStyleSheet("QProgressBar::chunk { background-color: #4CAF50; }"); // Green for normal
    }
}

void MainWindow::onRecordingDurationChanged(qint64 duration) {
    m_recordingTimeLabel->setText(formatDuration(duration));
}

void MainWindow::onTranscriptionCompleted(const QString& requestId, const TranscriptionResult& result) {
    Q_UNUSED(requestId)
    
    m_transcriptionProgressBar->setVisible(false);
    m_transcriptionStatusLabel->setText(QString("Transcribed (Confidence: %1%)").arg(static_cast<int>(result.confidence * 100)));
    m_transcriptionTextEdit->setText(result.text);
    m_enhanceButton->setEnabled(true);
    m_retranscribeButton->setEnabled(true);
    
    showStatusMessage("Transcription completed successfully");
}

void MainWindow::onTranscriptionFailed(const QString& requestId, TranscriptionError error, const QString& errorMessage) {
    Q_UNUSED(requestId)
    
    m_transcriptionProgressBar->setVisible(false);
    m_transcriptionStatusLabel->setText("Transcription failed");
    
    // Use centralized error handler
    m_errorHandler->handleTranscriptionError(error, errorMessage);
}

void MainWindow::onTranscriptionProgress(const QString& requestId, int progressPercent) {
    Q_UNUSED(requestId)
    
    m_transcriptionProgressBar->setVisible(true);
    m_transcriptionProgressBar->setValue(progressPercent);
    m_transcriptionStatusLabel->setText(QString("Transcribing... %1%").arg(progressPercent));
}

void MainWindow::onEnhancementCompleted(const QString& requestId, const EnhancementResult& result) {
    Q_UNUSED(requestId)
    
    m_enhancementProgressBar->setVisible(false);
    m_enhancementStatusLabel->setText("Enhanced successfully");
    m_enhancedTextEdit->setText(result.enhancedText);
    
    showStatusMessage("Text enhancement completed");
}

void MainWindow::onEnhancementFailed(const QString& requestId, EnhancementError error, const QString& errorMessage) {
    Q_UNUSED(requestId)
    
    m_enhancementProgressBar->setVisible(false);
    m_enhancementStatusLabel->setText("Enhancement failed");
    
    // Use centralized error handler
    m_errorHandler->handleEnhancementError(error, errorMessage);
}

void MainWindow::onEnhancementProgress(const QString& requestId, int progressPercent) {
    Q_UNUSED(requestId)
    
    m_enhancementProgressBar->setVisible(true);
    m_enhancementProgressBar->setValue(progressPercent);
    m_enhancementStatusLabel->setText(QString("Enhancing... %1%").arg(progressPercent));
}

// UI interaction slots (simplified implementations)
void MainWindow::onDeviceSelectionChanged() {
    // Update audio device selection
    updateDeviceList();
}

void MainWindow::onTranscriptionProviderChanged() {
    // Update transcription provider and save to configuration
    if (m_transcriptionProviderCombo) {
        int provider = m_transcriptionProviderCombo->currentIndex();
        m_configManager->setTranscriptionProvider(provider);
    }
}

void MainWindow::onEnhancementModeChanged() {
    // Update enhancement mode and save to configuration
    if (m_enhancementModeCombo) {
        int mode = m_enhancementModeCombo->currentIndex();
        m_configManager->setEnhancementMode(mode);
    }
}

void MainWindow::onSessionSelectionChanged() {
    // Load selected session
}

void MainWindow::onRecordingSelectionChanged() {
    // Load selected recording
}

void MainWindow::onEnhanceButtonClicked() {
    if (!m_currentTranscriptionId.isEmpty()) {
        startEnhancement(m_currentTranscriptionId);
    }
}

void MainWindow::onClearButtonClicked() {
    clearCurrentDisplay();
}

void MainWindow::onSaveButtonClicked() {
    saveCurrentSession();
}

void MainWindow::onSettingsButtonClicked() {
    // Open settings dialog
}

// Menu action slots (simplified implementations)
void MainWindow::onNewSessionAction() {
    createNewSession();
}

void MainWindow::onOpenSessionAction() {
    // Open session dialog
}

void MainWindow::onSaveSessionAction() {
    saveCurrentSession();
}

void MainWindow::onExportAction() {
    // Export data dialog
}

void MainWindow::onPreferencesAction() {
    // Preferences dialog
}

void MainWindow::onAboutAction() {
    QMessageBox::about(this, "About QuillScribe", 
        "QuillScribe v1.0\n\n"
        "Voice-to-Text Application with AI Enhancement\n\n"
        "Built with Qt 6, whisper.cpp, and Google Gemini API");
}

// Storage and data update slots
void MainWindow::onDatabaseConnected() {
    showStatusMessage("Database connected successfully");
    updateSessionList();
    updateRecordingHistory();
}

void MainWindow::onDatabaseDisconnected() {
    showStatusMessage("Database disconnected");
}

void MainWindow::onStorageError(StorageError error, const QString& errorMessage) {
    // Use centralized error handler
    m_errorHandler->handleStorageError(error, errorMessage);
}

void MainWindow::onRecordingCreated(const QString& recordingId) {
    qDebug() << "Recording created in database:" << recordingId;
    updateRecordingHistory();
    updateStatusBar();
}

void MainWindow::onRecordingUpdated(const QString& recordingId) {
    qDebug() << "Recording updated in database:" << recordingId;
    updateRecordingHistory();
    updateStatusBar();
}

void MainWindow::onRecordingDeleted(const QString& recordingId) {
    qDebug() << "Recording deleted from database:" << recordingId;
    updateRecordingHistory();
    updateStatusBar();
}

void MainWindow::onSessionCreated(const QString& sessionId) {
    qDebug() << "Session created in database:" << sessionId;
    updateSessionList();
    
    // Set as current session if we don't have one
    if (m_currentSessionId.isEmpty()) {
        m_currentSessionId = sessionId;
    }
}

void MainWindow::onSessionStarted(const QString& sessionId) {
    qDebug() << "Session started:" << sessionId;
    showStatusMessage("Session started: " + sessionId);
}

void MainWindow::onSessionEnded(const QString& sessionId) {
    qDebug() << "Session ended:" << sessionId;
    showStatusMessage("Session ended: " + sessionId);
    updateSessionList();
}

// Configuration management slots
void MainWindow::onSettingChanged(const QString& key, const QVariant& value) {
    qDebug() << "Setting changed:" << key << "=" << value;
    
    // Handle specific setting changes that require immediate UI updates
    if (key == "Audio/InputGain" && m_inputGainSlider) {
        m_inputGainSlider->setValue(value.toInt());
    } else if (key == "Transcription/Provider" && m_transcriptionProviderCombo) {
        m_transcriptionProviderCombo->setCurrentIndex(value.toInt());
    } else if (key == "Enhancement/Mode" && m_enhancementModeCombo) {
        m_enhancementModeCombo->setCurrentIndex(value.toInt());
    } else if (key == "Application/CurrentSessionId") {
        m_currentSessionId = value.toString();
        updateSessionList();
    }
}

void MainWindow::onConfigurationLoaded() {
    qDebug() << "Configuration loaded from:" << m_configManager->getConfigFilePath();
    showStatusMessage("Configuration loaded successfully", 2000);
    
    // Update UI with loaded settings
    loadSettings();
}

// Utility methods
void MainWindow::updateUI() {
    updateRecordingControls();
    updateStatusBar();
}

void MainWindow::updateRecordingControls() {
    if (m_isRecording) {
        if (m_isPaused) {
            m_recordButton->setText("▶️ Resume");
            m_recordButton->setStyleSheet("QPushButton { background-color: #4CAF50; color: white; font-weight: bold; padding: 10px; }");
            m_pauseButton->setEnabled(false);
        } else {
            m_recordButton->setText("⏸️ Pause");
            m_recordButton->setStyleSheet("QPushButton { background-color: #ff9800; color: white; font-weight: bold; padding: 10px; }");
            m_pauseButton->setEnabled(true);
        }
        m_stopButton->setEnabled(true);
        m_recordingStatusLabel->setText(m_isPaused ? "Paused" : "Recording...");
    } else {
        m_recordButton->setText("🎙️ Record");
        m_recordButton->setStyleSheet("QPushButton { background-color: #4CAF50; color: white; font-weight: bold; padding: 10px; }");
        m_pauseButton->setEnabled(false);
        m_stopButton->setEnabled(false);
        m_recordingStatusLabel->setText("Ready");
    }
}

void MainWindow::updateTranscriptionControls() {
    // Update transcription UI state
}

void MainWindow::updateEnhancementControls() {
    // Update enhancement UI state
}

void MainWindow::updateDeviceList() {
    if (!m_audioRecorderService) return;
    
    m_deviceComboBox->clear();
    auto devices = m_audioRecorderService->getAvailableDevices();
    
    for (const auto& device : devices) {
        m_deviceComboBox->addItem(device.description(), QVariant::fromValue(device));
    }
    
    if (devices.isEmpty()) {
        m_deviceComboBox->addItem("No recording devices found");
        m_deviceComboBox->setEnabled(false);
    } else {
        m_deviceComboBox->setEnabled(true);
    }
}

void MainWindow::updateSessionList() {
    // Update session list
}

void MainWindow::updateRecordingHistory() {
    // Update recording history
}

void MainWindow::updateStatusBar() {
    // Update status bar with current information
}

void MainWindow::startRecording() {
    if (!validateRecordingSettings()) {
        return;
    }
    
    // Generate recording file path
    QString recordingPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + 
                           "/recordings/" + QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm-ss") + ".wav";
    
    // Create recording directory if it doesn't exist
    QDir().mkpath(QFileInfo(recordingPath).absolutePath());
    
    // Set current session ID in audio recorder service
    m_audioRecorderService->setCurrentSessionId(m_currentSessionId);
    
    if (m_audioRecorderService->startRecording(recordingPath)) {
        // Recording will be automatically saved to storage by the AudioRecorderService
        // Get the recording ID from the service
        m_currentRecordingId = m_audioRecorderService->getCurrentRecordingId();
    }
}

void MainWindow::stopRecording() {
    if (m_audioRecorderService) {
        m_audioRecorderService->stopRecording();
    }
}

void MainWindow::pauseRecording() {
    if (m_audioRecorderService) {
        m_audioRecorderService->pauseRecording();
    }
}

void MainWindow::resumeRecording() {
    if (m_audioRecorderService) {
        m_audioRecorderService->resumeRecording();
    }
}

void MainWindow::startTranscription(const QString& recordingId) {
    Q_UNUSED(recordingId)
    
    if (!validateTranscriptionSettings()) {
        return;
    }
    
    m_transcriptionStatusLabel->setText("Starting transcription...");
    m_transcriptionProgressBar->setVisible(true);
    m_transcriptionProgressBar->setValue(0);
    
    // Implementation would create transcription request and submit to service
}

void MainWindow::retranscribe() {
    if (!m_currentRecordingId.isEmpty()) {
        startTranscription(m_currentRecordingId);
    }
}

void MainWindow::startEnhancement(const QString& transcriptionId) {
    Q_UNUSED(transcriptionId)
    
    if (!validateEnhancementSettings()) {
        return;
    }
    
    QString textToEnhance = m_transcriptionTextEdit->toPlainText();
    if (textToEnhance.isEmpty()) {
        m_errorHandler->reportWarning("Enhancement Error", "No text available for enhancement. Please complete a transcription first.");
        return;
    }
    
    m_enhancementStatusLabel->setText("Starting enhancement...");
    m_enhancementProgressBar->setVisible(true);
    m_enhancementProgressBar->setValue(0);
    
    // Implementation would create enhancement request and submit to service
}

void MainWindow::createNewSession() {
    if (m_storageManager && m_storageManager->getUserSessionStorage()) {
        QString sessionName = QString("Session %1").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm"));
        m_currentSessionId = m_storageManager->getUserSessionStorage()->createNewSession(sessionName);
        updateSessionList();
        showStatusMessage("New session created: " + sessionName);
    }
}

void MainWindow::loadSession(const QString& sessionId) {
    m_currentSessionId = sessionId;
    updateRecordingHistory();
}

void MainWindow::saveCurrentSession() {
    if (m_storageManager && !m_currentSessionId.isEmpty()) {
        // Save session data
        showStatusMessage("Session saved successfully");
    }
}

void MainWindow::clearCurrentDisplay() {
    m_transcriptionTextEdit->clear();
    m_enhancedTextEdit->clear();
    m_transcriptionStatusLabel->setText("No transcription");
    m_enhancementStatusLabel->setText("No enhancement");
    m_enhanceButton->setEnabled(false);
    m_retranscribeButton->setEnabled(false);
}

bool MainWindow::validateRecordingSettings() {
    if (!m_audioRecorderService || !m_audioRecorderService->isDeviceAvailable()) {
        m_errorHandler->reportCriticalError("Recording Device Error", 
                                           "No recording device available. Please check your microphone connection.");
        return false;
    }
    
    return true;
}

bool MainWindow::validateTranscriptionSettings() {
    return m_transcriptionService != nullptr;
}

bool MainWindow::validateEnhancementSettings() {
    return m_textEnhancementService != nullptr;
}

QString MainWindow::formatDuration(qint64 milliseconds) const {
    int totalSeconds = static_cast<int>(milliseconds / 1000);
    int hours = totalSeconds / 3600;
    int minutes = (totalSeconds % 3600) / 60;
    int seconds = totalSeconds % 60;
    
    return QString("%1:%2:%3")
           .arg(hours, 2, 10, QChar('0'))
           .arg(minutes, 2, 10, QChar('0'))
           .arg(seconds, 2, 10, QChar('0'));
}

QString MainWindow::formatFileSize(qint64 bytes) const {
    const qint64 kb = 1024;
    const qint64 mb = kb * 1024;
    const qint64 gb = mb * 1024;
    
    if (bytes >= gb) {
        return QString::number(bytes / gb, 'f', 1) + " GB";
    } else if (bytes >= mb) {
        return QString::number(bytes / mb, 'f', 1) + " MB";
    } else if (bytes >= kb) {
        return QString::number(bytes / kb, 'f', 1) + " KB";
    } else {
        return QString::number(bytes) + " bytes";
    }
}

void MainWindow::showStatusMessage(const QString& message, int timeout) {
    statusBar()->showMessage(message, timeout);
}

void MainWindow::showErrorMessage(const QString& title, const QString& message) {
    QMessageBox::critical(this, title, message);
}

void MainWindow::showSuccessMessage(const QString& message) {
    QMessageBox::information(this, "Success", message);
}

void MainWindow::setupApplicationStyle() {
    // Set application style
    setStyleSheet(R"(
        QMainWindow {
            background-color: #f5f5f5;
        }
        
        QGroupBox {
            font-weight: bold;
            border: 2px solid #cccccc;
            border-radius: 8px;
            margin-top: 1ex;
            padding: 10px;
            background-color: white;
        }
        
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 10px;
            padding: 0 5px 0 5px;
            color: #333333;
        }
        
        QPushButton {
            border: 1px solid #cccccc;
            border-radius: 6px;
            padding: 8px 16px;
            min-width: 80px;
            background-color: white;
        }
        
        QPushButton:hover {
            background-color: #e1e1e1;
            border-color: #999999;
        }
        
        QPushButton:pressed {
            background-color: #d4edda;
        }
        
        QPushButton:disabled {
            background-color: #f8f9fa;
            color: #6c757d;
        }
        
        QComboBox {
            border: 1px solid #cccccc;
            border-radius: 4px;
            padding: 5px;
            min-width: 6em;
            background-color: white;
        }
        
        QTextEdit {
            border: 1px solid #cccccc;
            border-radius: 4px;
            padding: 8px;
            background-color: white;
            selection-background-color: #4CAF50;
        }
        
        QProgressBar {
            border: 1px solid #cccccc;
            border-radius: 4px;
            text-align: center;
        }
        
        QProgressBar::chunk {
            background-color: #4CAF50;
            border-radius: 3px;
        }
    )");
}

QSize MainWindow::getOptimalWindowSize() const {
    return QSize(DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT);
}

void MainWindow::updateRecordingTimer() {
    if (m_isRecording && !m_isPaused) {
        qint64 elapsed = m_recordingTimer.elapsed();
        m_recordingTimeLabel->setText(formatDuration(elapsed));
    }
}