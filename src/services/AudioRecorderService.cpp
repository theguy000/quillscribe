#include "AudioRecorderService.h"
#include "AudioLevelIODevice.h"
#include "StorageManager.h"
#include "../models/Recording.h"
#include <QAudioSource>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QDateTime>
#include <QtMath>
#include <algorithm>

AudioRecorderService::AudioRecorderService(QObject* parent)
    : IAudioRecorder(parent)
    , m_audioSource(nullptr)
    , m_outputFile(nullptr)
    , m_levelIODevice(nullptr)
    , m_state(AudioRecordingState::Stopped)
    , m_lastError(AudioError::NoError)
    , m_recordingDuration(0)
    , m_recordedBytes(0)
    , m_currentInputLevel(0.0)
    , m_autoGainControl(true)
    , m_noiseReduction(true)
    , m_inputGain(1.0)
    , m_storageManager(nullptr)
{
    // Setup monitoring timers
    m_levelTimer = new QTimer(this);
    m_levelTimer->setInterval(LEVEL_UPDATE_INTERVAL_MS);
    connect(m_levelTimer, &QTimer::timeout, this, &AudioRecorderService::handleInputLevelChanged);
    
    m_durationTimer = new QTimer(this);
    m_durationTimer->setInterval(DURATION_UPDATE_INTERVAL_MS);
    connect(m_durationTimer, &QTimer::timeout, this, &AudioRecorderService::updateRecordingDuration);
    
    // Set default audio format
    m_audioFormat = getRecommendedFormat();
    
    // Set default device
    auto devices = getAvailableDevices();
    if (!devices.isEmpty()) {
        m_currentDevice = devices.first();
    }
}

AudioRecorderService::AudioRecorderService(IStorageManager* storageManager, QObject* parent)
    : AudioRecorderService(parent)
{
    m_storageManager = storageManager;
}

AudioRecorderService::~AudioRecorderService() {
    if (m_state != AudioRecordingState::Stopped) {
        stopRecording();
    }
    cleanupAudioInput();
}

QList<QAudioDevice> AudioRecorderService::getAvailableDevices() const {
    return QMediaDevices::audioInputs();
}

bool AudioRecorderService::setRecordingDevice(const QAudioDevice& device) {
    QMutexLocker locker(&m_stateMutex);
    
    if (!isDeviceValid(device)) {
        AudioError error = AudioError::DeviceNotFound;
        QString errorMsg = "Invalid audio device";
        setError(error, errorMsg);
        emit errorOccurred(error, errorMsg);
        return false;
    }
    
    if (m_state == AudioRecordingState::Recording) {
        AudioError error = AudioError::DeviceAccessDenied;
        QString errorMsg = "Cannot change device while recording";
        setError(error, errorMsg);
        emit errorOccurred(error, errorMsg);
        return false;
    }
    
    // Stop any ongoing level monitoring during device switching
    bool wasLevelTimerRunning = m_levelTimer->isActive();
    if (wasLevelTimerRunning) {
        m_levelTimer->stop();
    }
    
    m_currentDevice = device;
    clearError();
    
    // Reinitialize audio input with new device
    cleanupAudioInput();
    
    // Restart level timer if it was running (for real-time level monitoring while not recording)
    if (wasLevelTimerRunning) {
        m_levelTimer->start();
    }
    
    qDebug() << "Successfully switched to audio device:" << device.description();
    return true;
}

QAudioDevice AudioRecorderService::getCurrentDevice() const {
    return m_currentDevice;
}

bool AudioRecorderService::isDeviceAvailable() const {
    return isDeviceValid(m_currentDevice);
}

void AudioRecorderService::setAudioFormat(const QAudioFormat& format) {
    QMutexLocker locker(&m_stateMutex);
    
    if (m_state == AudioRecordingState::Recording) {
        qWarning() << "Cannot change audio format while recording";
        return;
    }
    
    if (isFormatSupported(format)) {
        m_audioFormat = format;
        cleanupAudioInput(); // Force reinit with new format
        clearError();
    } else {
        setError(AudioError::FormatNotSupported, "Audio format not supported");
    }
}

QAudioFormat AudioRecorderService::getAudioFormat() const {
    return m_audioFormat;
}

QAudioFormat AudioRecorderService::getRecommendedFormat() const {
    QAudioFormat format;
    format.setSampleRate(16000);        // 16kHz for whisper.cpp
    format.setChannelCount(1);          // Mono
    format.setSampleFormat(QAudioFormat::Int16);  // 16-bit signed integer
    return format;
}

bool AudioRecorderService::startRecording(const QString& outputPath) {
    QMutexLocker locker(&m_stateMutex);
    
    if (m_state == AudioRecordingState::Recording) {
        setError(AudioError::IoError, "Already recording");
        return false;
    }
    
    if (!validateOutputPath(outputPath)) {
        setError(AudioError::IoError, "Invalid output path: " + outputPath);
        return false;
    }
    
    if (!isDeviceAvailable()) {
        setError(AudioError::DeviceNotFound, "Recording device not available");
        return false;
    }
    
    // Clean up any existing audio input
    cleanupAudioInput();
    
    // Create output file
    m_outputFile = new QFile(outputPath);
    
    // Create level monitoring device that wraps the file
    m_levelIODevice = new AudioLevelIODevice(m_outputFile, this);
    
    // Set audio format parameters for level calculation
    m_levelIODevice->setAudioFormat(
        m_audioFormat.sampleRate(),
        m_audioFormat.channelCount(),
        m_audioFormat.sampleFormat() == QAudioFormat::Int16 ? 2 : 
        m_audioFormat.sampleFormat() == QAudioFormat::Int32 ? 4 : 1
    );
    
    // Connect level monitoring signals
    connect(m_levelIODevice, &AudioLevelIODevice::levelChanged, this, [this](double level) {
        m_currentInputLevel = level * m_inputGain;
        emit inputLevelChanged(m_currentInputLevel);
    });
    
    connect(m_levelIODevice, &AudioLevelIODevice::audioDataReady, this, [this](const QByteArray& data) {
        m_currentAudioData = data;
        emit audioDataReady(data);
    });
    
    if (!m_levelIODevice->open(QIODevice::WriteOnly)) {
        setError(AudioError::IoError, "Cannot create output file: " + outputPath);
        delete m_levelIODevice;
        m_levelIODevice = nullptr;
        delete m_outputFile;
        m_outputFile = nullptr;
        return false;
    }
    
    // Initialize audio input
    initializeAudioInput();
    if (!m_audioSource) {
        m_levelIODevice->close();
        delete m_levelIODevice;
        m_levelIODevice = nullptr;
        delete m_outputFile;
        m_outputFile = nullptr;
        return false;
    }
    
    // Start recording
    m_currentOutputPath = outputPath;
    m_recordingDuration = 0;
    m_recordedBytes = 0;
    m_currentInputLevel = 0.0;
    
    m_audioSource->start(m_levelIODevice);
    
    // Start monitoring timers
    m_recordingTimer.start();
    m_levelTimer->start();
    m_durationTimer->start();
    
    setState(AudioRecordingState::Recording);
    
    // Save recording to storage
    saveRecordingToStorage();
    
    emit recordingStarted();
    
    clearError();
    return true;
}

void AudioRecorderService::pauseRecording() {
    QMutexLocker locker(&m_stateMutex);
    
    if (m_state != AudioRecordingState::Recording) {
        qWarning() << "Cannot pause - not currently recording";
        return;
    }
    
    if (m_audioSource) {
        m_audioSource->suspend();
    }
    
    // Pause timers
    m_levelTimer->stop();
    m_durationTimer->stop();
    
    setState(AudioRecordingState::Paused);
    emit recordingPaused();
}

void AudioRecorderService::resumeRecording() {
    QMutexLocker locker(&m_stateMutex);
    
    if (m_state != AudioRecordingState::Paused) {
        qWarning() << "Cannot resume - not currently paused";
        return;
    }
    
    if (m_audioSource) {
        m_audioSource->resume();
    }
    
    // Resume timers
    m_levelTimer->start();
    m_durationTimer->start();
    
    setState(AudioRecordingState::Recording);
    emit recordingResumed();
}

void AudioRecorderService::stopRecording() {
    QMutexLocker locker(&m_stateMutex);
    
    if (m_state == AudioRecordingState::Stopped) {
        return;
    }
    
    // Stop timers
    m_levelTimer->stop();
    m_durationTimer->stop();
    
    // Update final duration
    if (m_recordingTimer.isValid()) {
        m_recordingDuration = m_recordingTimer.elapsed();
    }
    
    QString filePath = m_currentOutputPath;
    qint64 duration = m_recordingDuration;
    
    // Stop and cleanup audio input
    if (m_audioSource) {
        m_audioSource->stop();
    }
    cleanupAudioInput();
    
    // Close output file and level device
    if (m_levelIODevice) {
        m_recordedBytes = m_levelIODevice->size();
        m_levelIODevice->close();
        delete m_levelIODevice;
        m_levelIODevice = nullptr;
    }
    
    if (m_outputFile) {
        delete m_outputFile;
        m_outputFile = nullptr;
    }
    
    // Mark recording as complete in storage
    markRecordingComplete();
    
    setState(AudioRecordingState::Stopped);
    emit recordingStopped(filePath, duration);
}

void AudioRecorderService::cancelRecording() {
    QMutexLocker locker(&m_stateMutex);
    
    if (m_state == AudioRecordingState::Stopped) {
        return;
    }
    
    // Stop timers
    m_levelTimer->stop();
    m_durationTimer->stop();
    
    // Stop and cleanup audio input
    if (m_audioSource) {
        m_audioSource->stop();
    }
    cleanupAudioInput();
    
    // Close and delete output file and level device
    QString filePath;
    if (m_levelIODevice) {
        filePath = m_outputFile ? m_outputFile->fileName() : QString();
        m_levelIODevice->close();
        delete m_levelIODevice;
        m_levelIODevice = nullptr;
    }
    
    if (m_outputFile) {
        if (filePath.isEmpty()) {
            filePath = m_outputFile->fileName();
        }
        delete m_outputFile;
        m_outputFile = nullptr;
        
        // Remove the partially recorded file
        if (!filePath.isEmpty()) {
            QFile::remove(filePath);
        }
    }
    
    m_recordingDuration = 0;
    m_recordedBytes = 0;
    m_currentOutputPath.clear();
    
    setState(AudioRecordingState::Stopped);
    emit recordingCancelled();
}

AudioRecordingState AudioRecorderService::getState() const {
    QMutexLocker locker(&m_stateMutex);
    return m_state;
}

AudioError AudioRecorderService::getLastError() const {
    return m_lastError;
}

QString AudioRecorderService::getErrorString() const {
    return m_errorString;
}

qint64 AudioRecorderService::getRecordingDuration() const {
    return m_recordingDuration;
}

qint64 AudioRecorderService::getRecordedBytes() const {
    return m_recordedBytes;
}

double AudioRecorderService::getCurrentInputLevel() const {
    return m_currentInputLevel;
}

QByteArray AudioRecorderService::getCurrentAudioData() const {
    return m_currentAudioData;
}

bool AudioRecorderService::isClipping() const {
    return isClippingLevel(m_currentInputLevel);
}

void AudioRecorderService::setAutoGainControl(bool enabled) {
    m_autoGainControl = enabled;
}

void AudioRecorderService::setNoiseReduction(bool enabled) {
    m_noiseReduction = enabled;
}

void AudioRecorderService::setInputGain(double gain) {
    m_inputGain = qBound(0.0, gain, 2.0);
}

void AudioRecorderService::onDeviceChanged() {
    qDebug() << "Audio device changed - updating available devices";
    
    // Check if current device is still available
    if (!isDeviceAvailable()) {
        auto devices = getAvailableDevices();
        if (!devices.isEmpty()) {
            setRecordingDevice(devices.first());
        } else {
            setError(AudioError::DeviceNotFound, "No audio input devices available");
        }
    }
}

void AudioRecorderService::onVolumeChanged(double volume) {
    Q_UNUSED(volume)
    // Volume changes are handled by the system
    // This could trigger input level recalculation if needed
    calculateInputLevel();
}

void AudioRecorderService::handleStateChanged(QAudio::State state) {
    switch (state) {
        case QAudio::ActiveState:
            // Audio input is active
            break;
        case QAudio::SuspendedState:
            // Audio input is suspended (paused)
            break;
        case QAudio::StoppedState:
            // Audio input has stopped
            if (m_state == AudioRecordingState::Recording || m_state == AudioRecordingState::Paused) {
                // Unexpected stop - check for errors
                if (m_audioSource && m_audioSource->error() != QAudio::NoError) {
                    AudioError error = AudioError::UnknownError;
                    QString errorMsg = "Audio input error";
                    
                    switch (m_audioSource->error()) {
                        case QAudio::OpenError:
                            error = AudioError::DeviceAccessDenied;
                            errorMsg = "Cannot open audio device";
                            break;
                        case QAudio::IOError:
                            error = AudioError::IoError;
                            errorMsg = "Audio I/O error";
                            break;
                        case QAudio::UnderrunError:
                            error = AudioError::InsufficientMemory;
                            errorMsg = "Audio buffer underrun";
                            break;
                        case QAudio::FatalError:
                            error = AudioError::UnknownError;
                            errorMsg = "Fatal audio error";
                            break;
                        default:
                            break;
                    }
                    
                    setError(error, errorMsg);
                    setState(AudioRecordingState::Error);
                    emit errorOccurred(error, errorMsg);
                }
            }
            break;
        case QAudio::IdleState:
            // Audio input is idle
            break;
    }
}

void AudioRecorderService::handleInputLevelChanged() {
    // Thread-safe check for valid state and device
    QMutexLocker locker(&m_stateMutex);
    
    if (m_state == AudioRecordingState::Recording && m_levelIODevice) {
        // Get current level from the real audio stream
        try {
            m_currentInputLevel = m_levelIODevice->getCurrentLevel() * m_inputGain;
            m_currentAudioData = m_levelIODevice->getLastAudioData();
            
            // Note: inputLevelChanged signal is now emitted directly from AudioLevelIODevice
            // via the connected lambda in startRecording()
            
            if (isClipping()) {
                qWarning() << "Audio input clipping detected";
            }
        } catch (...) {
            // Ignore errors during device switching - level monitoring will resume
            qDebug() << "Audio level calculation error during device switching";
        }
    }
}

void AudioRecorderService::updateRecordingDuration() {
    if (m_state == AudioRecordingState::Recording && m_recordingTimer.isValid()) {
        m_recordingDuration = m_recordingTimer.elapsed();
        updateMetrics();
        emit durationChanged(m_recordingDuration);
    }
}

void AudioRecorderService::initializeAudioInput() {
    if (m_audioSource) {
        cleanupAudioInput();
    }
    
    m_audioSource = new QAudioSource(m_currentDevice, m_audioFormat, this);
    
    // Apply settings
    if (m_autoGainControl) {
        // Qt doesn't have direct AGC control, but we can adjust buffer sizes
        m_audioSource->setBufferSize(AUDIO_BUFFER_SIZE);
    }
    
    // Connect state change signals
    connect(m_audioSource, QOverload<QAudio::State>::of(&QAudioSource::stateChanged),
            this, &AudioRecorderService::handleStateChanged);
    
    // Note: Qt6 removed setNotifyInterval - using timer for level monitoring instead
}

void AudioRecorderService::cleanupAudioInput() {
    if (m_audioSource) {
        m_audioSource->stop();
        m_audioSource->disconnect();
        m_audioSource->deleteLater();
        m_audioSource = nullptr;
    }
}

void AudioRecorderService::setState(AudioRecordingState newState) {
    if (m_state != newState) {
        AudioRecordingState oldState = m_state;
        m_state = newState;
        emit stateChanged(newState, oldState);
    }
}

void AudioRecorderService::setError(AudioError error, const QString& errorMessage) {
    m_lastError = error;
    m_errorString = errorMessage;
    qWarning() << "AudioRecorderService error:" << errorMessage;
}

void AudioRecorderService::clearError() {
    m_lastError = AudioError::NoError;
    m_errorString.clear();
}

bool AudioRecorderService::validateOutputPath(const QString& path) {
    if (path.isEmpty()) {
        return false;
    }
    
    QFileInfo fileInfo(path);
    QDir parentDir = fileInfo.absoluteDir();
    
    // Check if parent directory exists or can be created
    if (!parentDir.exists() && !parentDir.mkpath(parentDir.absolutePath())) {
        return false;
    }
    
    // Check if we can write to the directory
    return QFileInfo(parentDir.absolutePath()).isWritable();
}

void AudioRecorderService::calculateInputLevel() {
    // This method is now deprecated - real level calculation is done in AudioLevelIODevice
    // Keep for compatibility but levels are calculated from actual audio stream
    if (m_levelIODevice && m_state == AudioRecordingState::Recording) {
        m_currentInputLevel = m_levelIODevice->getCurrentLevel() * m_inputGain;
        m_currentAudioData = m_levelIODevice->getLastAudioData();
    } else {
        m_currentInputLevel = 0.0;
        m_currentAudioData.clear();
    }
}

double AudioRecorderService::calculateRMSLevel(const QByteArray& buffer) const {
    if (buffer.isEmpty()) {
        return 0.0;
    }
    
    const int16_t* samples = reinterpret_cast<const int16_t*>(buffer.data());
    int sampleCount = buffer.size() / sizeof(int16_t);
    
    if (sampleCount == 0) {
        return 0.0;
    }
    
    // Calculate RMS level
    double sum = 0.0;
    for (int i = 0; i < sampleCount; ++i) {
        double sample = static_cast<double>(samples[i]) / 32768.0; // Normalize to [-1, 1]
        sum += sample * sample;
    }
    
    double rms = qSqrt(sum / sampleCount);
    return qBound(0.0, rms, 1.0);
}

bool AudioRecorderService::isClippingLevel(double level) const {
    return level >= CLIPPING_THRESHOLD;
}

void AudioRecorderService::updateMetrics() {
    if (m_levelIODevice) {
        // Get file size from the level monitoring device
        m_recordedBytes = m_levelIODevice->size();
        
        // If file size is still 0, estimate based on duration and format
        if (m_recordedBytes == 0 && m_recordingDuration > 0) {
            // Estimate: 16kHz * 1 channel * 2 bytes per sample = 32,000 bytes per second
            double estimatedBytesPerMs = (m_audioFormat.sampleRate() * m_audioFormat.channelCount() * 
                                        (m_audioFormat.sampleFormat() == QAudioFormat::Int16 ? 2 : 4)) / 1000.0;
            m_recordedBytes = static_cast<qint64>(m_recordingDuration * estimatedBytesPerMs);
        }
    } else if (m_outputFile) {
        // Fallback to direct file access
        m_outputFile->flush();
        m_recordedBytes = m_outputFile->size();
    }
}

bool AudioRecorderService::isDeviceValid(const QAudioDevice& device) const {
    if (device.isNull()) {
        return false;
    }
    
    auto availableDevices = getAvailableDevices();
    return availableDevices.contains(device);
}

void AudioRecorderService::setupDeviceMonitoring() {
    // Future: Could implement device hotplug monitoring here
}

void AudioRecorderService::cleanupDeviceMonitoring() {
    // Future: Could cleanup device monitoring here
}

bool AudioRecorderService::isFormatSupported(const QAudioFormat& format) const {
    if (m_currentDevice.isNull()) {
        return false;
    }
    
    return m_currentDevice.isFormatSupported(format);
}

void AudioRecorderService::applyRecommendedSettings() {
    setAudioFormat(getRecommendedFormat());
    setAutoGainControl(true);
    setNoiseReduction(true);
    setInputGain(1.0);
}

// Storage management methods
void AudioRecorderService::setStorageManager(IStorageManager* storageManager) {
    m_storageManager = storageManager;
}

IStorageManager* AudioRecorderService::getStorageManager() const {
    return m_storageManager;
}

void AudioRecorderService::setCurrentSessionId(const QString& sessionId) {
    m_currentSessionId = sessionId;
}

QString AudioRecorderService::getCurrentSessionId() const {
    return m_currentSessionId;
}

QString AudioRecorderService::getCurrentRecordingId() const {
    return m_currentRecordingId;
}

void AudioRecorderService::saveRecordingToStorage() {
    if (!m_storageManager || m_currentSessionId.isEmpty()) {
        return;
    }

    // Create recording entry
    Recording recording(m_currentSessionId, m_currentOutputPath);
    recording.setTimestamp(QDateTime::currentDateTime());
    recording.setStatus(RecordingStatus::Recording);
    recording.setDeviceName(m_currentDevice.description());
    recording.setSampleRate(m_audioFormat.sampleRate());
    recording.setLanguage("en"); // Default language

    auto* recordingStorage = m_storageManager->getRecordingStorage();
    if (recordingStorage) {
        m_currentRecordingId = recordingStorage->saveRecording(recording);
        qDebug() << "Saved recording to storage with ID:" << m_currentRecordingId;
    }
}

void AudioRecorderService::updateRecordingInStorage() {
    if (!m_storageManager || m_currentRecordingId.isEmpty()) {
        return;
    }

    auto* recordingStorage = m_storageManager->getRecordingStorage();
    if (!recordingStorage) {
        return;
    }

    // Get current recording and update it
    Recording recording = recordingStorage->getRecording(m_currentRecordingId);
    if (recording.isValid()) {
        recording.setDuration(m_recordingDuration);
        recording.setFileSize(m_recordedBytes);
        recordingStorage->updateRecording(recording);
    }
}

void AudioRecorderService::markRecordingComplete() {
    if (!m_storageManager || m_currentRecordingId.isEmpty()) {
        return;
    }

    auto* recordingStorage = m_storageManager->getRecordingStorage();
    if (!recordingStorage) {
        return;
    }

    // Get current recording and mark as completed
    Recording recording = recordingStorage->getRecording(m_currentRecordingId);
    if (recording.isValid()) {
        recording.setDuration(m_recordingDuration);
        recording.setFileSize(m_recordedBytes);
        recording.setStatus(RecordingStatus::Completed);
        recordingStorage->updateRecording(recording);
        qDebug() << "Marked recording as complete:" << m_currentRecordingId;
    }
}
