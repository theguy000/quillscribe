// Audio Recording Interface Contract
// Contract for audio recording functionality in voice-to-text application

#pragma once

#include <QObject>
#include <QString>
#include <QAudioFormat>
#include <QAudioDevice>
#include <QMediaDevices>
#include <QIODevice>

enum class AudioRecordingState {
    Stopped,
    Recording, 
    Paused,
    Error
};

enum class AudioError {
    NoError,
    DeviceNotFound,
    DeviceAccessDenied,
    FormatNotSupported,
    InsufficientMemory,
    IoError,
    UnknownError
};

/**
 * @brief Interface for audio recording operations
 * 
 * Contract Requirements:
 * - FR-001: Must capture voice input through device microphone with one-touch recording
 * - FR-011: Must provide visual feedback during recording (waveform, timer, recording status)  
 * - FR-012: Must allow users to pause and resume recordings
 * - PR-004: Recording must start within 500ms of button press
 */
class IAudioRecorder : public QObject {
    Q_OBJECT

public:
    explicit IAudioRecorder(QObject* parent = nullptr) : QObject(parent) {}
    virtual ~IAudioRecorder() = default;

    // Device Management
    virtual QList<QAudioDevice> getAvailableDevices() const = 0;
    virtual bool setRecordingDevice(const QAudioDevice& device) = 0;
    virtual QAudioDevice getCurrentDevice() const = 0;
    virtual bool isDeviceAvailable() const = 0;

    // Format Configuration
    virtual void setAudioFormat(const QAudioFormat& format) = 0;
    virtual QAudioFormat getAudioFormat() const = 0;
    virtual QAudioFormat getRecommendedFormat() const = 0; // 16kHz, 16-bit, mono

    // Recording Control
    virtual bool startRecording(const QString& outputPath) = 0;
    virtual void pauseRecording() = 0;
    virtual void resumeRecording() = 0;
    virtual void stopRecording() = 0;
    virtual void cancelRecording() = 0;

    // Status Information
    virtual AudioRecordingState getState() const = 0;
    virtual AudioError getLastError() const = 0;
    virtual QString getErrorString() const = 0;
    virtual qint64 getRecordingDuration() const = 0; // milliseconds
    virtual qint64 getRecordedBytes() const = 0;

    // Real-time Monitoring
    virtual double getCurrentInputLevel() const = 0; // 0.0 to 1.0
    virtual QByteArray getCurrentAudioData() const = 0; // For waveform display
    virtual bool isClipping() const = 0; // Audio level too high

    // Settings
    virtual void setAutoGainControl(bool enabled) = 0;
    virtual void setNoiseReduction(bool enabled) = 0;
    virtual void setInputGain(double gain) = 0; // 0.0 to 2.0

signals:
    // State change notifications
    void stateChanged(AudioRecordingState newState, AudioRecordingState oldState);
    void errorOccurred(AudioError error, const QString& errorString);
    
    // Real-time updates
    void durationChanged(qint64 duration); // milliseconds
    void inputLevelChanged(double level); // 0.0 to 1.0
    void audioDataReady(const QByteArray& data); // For waveform visualization
    
    // Recording lifecycle
    void recordingStarted();
    void recordingPaused();
    void recordingResumed();
    void recordingStopped(const QString& filePath, qint64 duration);
    void recordingCancelled();

public slots:
    // External control
    virtual void onDeviceChanged() = 0;
    virtual void onVolumeChanged(double volume) = 0;
};

/**
 * @brief Factory interface for creating audio recorders
 */
class IAudioRecorderFactory {
public:
    virtual ~IAudioRecorderFactory() = default;
    virtual IAudioRecorder* createRecorder() = 0;
    virtual QStringList getSupportedFormats() const = 0;
    virtual bool isSupported() const = 0;
};

// Contract Test Requirements:
// 1. Test recording start time < 500ms (FR-004)
// 2. Test pause/resume functionality (FR-012) 
// 3. Test device enumeration and selection
// 4. Test audio format validation
// 5. Test real-time level monitoring (FR-011)
// 6. Test error handling for device access issues
// 7. Test recording duration accuracy
// 8. Test file output format compliance
// 9. Test memory usage during long recordings
// 10. Test device change handling during recording

