#pragma once

#include "../models/BaseModel.h"
#include "../../specs/001-voice-to-text/contracts/audio-recording-interface.h"
#include "../../specs/001-voice-to-text/contracts/storage-interface.h"
#include <QObject>
#include <QString>
#include <QAudioSource>
#include <QAudioDevice>
#include <QMediaDevices>
#include <QAudioFormat>
#include <QIODevice>
#include <QFile>
#include <QTimer>
#include <QElapsedTimer>
#include <QMutex>
#include <QMutexLocker>

class IStorageManager;
class AudioLevelIODevice;

/**
 * @brief AudioRecorderService implementation
 * 
 * Provides audio recording functionality using Qt Multimedia framework.
 * Implements the IAudioRecorder interface for voice capture and processing.
 */
class AudioRecorderService : public IAudioRecorder {
    Q_OBJECT

public:
    explicit AudioRecorderService(QObject* parent = nullptr);
    explicit AudioRecorderService(IStorageManager* storageManager, QObject* parent = nullptr);
    ~AudioRecorderService() override;

    // Storage management
    void setStorageManager(IStorageManager* storageManager);
    IStorageManager* getStorageManager() const;

    // Device Management
    QList<QAudioDevice> getAvailableDevices() const override;
    bool setRecordingDevice(const QAudioDevice& device) override;
    QAudioDevice getCurrentDevice() const override;
    bool isDeviceAvailable() const override;

    // Format Configuration
    void setAudioFormat(const QAudioFormat& format) override;
    QAudioFormat getAudioFormat() const override;
    QAudioFormat getRecommendedFormat() const override;

    // Recording Control
    bool startRecording(const QString& outputPath) override;
    void pauseRecording() override;
    void resumeRecording() override;
    void stopRecording() override;
    void cancelRecording() override;

    // Status Information
    AudioRecordingState getState() const override;
    AudioError getLastError() const override;
    QString getErrorString() const override;
    qint64 getRecordingDuration() const override;
    qint64 getRecordedBytes() const override;

    // Real-time Monitoring
    double getCurrentInputLevel() const override;
    QByteArray getCurrentAudioData() const override;
    bool isClipping() const override;

    // Settings
    void setAutoGainControl(bool enabled) override;
    void setNoiseReduction(bool enabled) override;
    void setInputGain(double gain) override;

    // Session management
    void setCurrentSessionId(const QString& sessionId);
    QString getCurrentSessionId() const;
    QString getCurrentRecordingId() const;

public slots:
    void onDeviceChanged() override;
    void onVolumeChanged(double volume) override;

private slots:
    void handleStateChanged(QAudio::State state);
    void handleInputLevelChanged();
    void updateRecordingDuration();

private:
    // Core recording components
    QAudioSource* m_audioSource;
    QFile* m_outputFile;
    AudioLevelIODevice* m_levelIODevice;
    QAudioDevice m_currentDevice;
    QAudioFormat m_audioFormat;
    
    // State management
    AudioRecordingState m_state;
    AudioError m_lastError;
    QString m_errorString;
    QString m_currentOutputPath;
    QString m_currentRecordingId;
    QString m_currentSessionId;
    
    // Recording metrics
    QElapsedTimer m_recordingTimer;
    qint64 m_recordingDuration; // milliseconds
    qint64 m_recordedBytes;
    double m_currentInputLevel;
    QByteArray m_currentAudioData;
    
    // Settings
    bool m_autoGainControl;
    bool m_noiseReduction;
    double m_inputGain;
    
    // Monitoring
    QTimer* m_levelTimer;
    QTimer* m_durationTimer;
    mutable QMutex m_stateMutex;
    
    // Storage integration
    IStorageManager* m_storageManager;
    
    // Helper methods
    void initializeAudioInput();
    void cleanupAudioInput();
    void setState(AudioRecordingState newState);
    void setError(AudioError error, const QString& errorMessage);
    void clearError();
    
    // Storage integration
    void saveRecordingToStorage();
    void updateRecordingInStorage();
    void markRecordingComplete();
    
    bool validateOutputPath(const QString& path);
    void calculateInputLevel();
    double calculateRMSLevel(const QByteArray& buffer) const;
    bool isClippingLevel(double level) const;
    void updateMetrics();
    
    // Device helper methods
    bool isDeviceValid(const QAudioDevice& device) const;
    void setupDeviceMonitoring();
    void cleanupDeviceMonitoring();
    
    // Format helper methods
    bool isFormatSupported(const QAudioFormat& format) const;
    void applyRecommendedSettings();
    
    // Constants
    static constexpr double CLIPPING_THRESHOLD = 0.95;
    static constexpr int LEVEL_UPDATE_INTERVAL_MS = 50;
    static constexpr int DURATION_UPDATE_INTERVAL_MS = 100;
    static constexpr int AUDIO_BUFFER_SIZE = 4096;
};
