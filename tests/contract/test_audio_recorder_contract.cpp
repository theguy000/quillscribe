// Contract Test for IAudioRecorder Interface
// This test validates the audio recording contract requirements
// Must FAIL initially - no implementation exists yet (TDD)

#include <gtest/gtest.h>
#include <QtTest>
#include <QSignalSpy>
#include <QAudioFormat>
#include <QAudioDevice>
#include <QTimer>
#include <QTemporaryDir>
#include <chrono>

// The contract we are testing
#include "../../specs/001-voice-to-text/contracts/audio-recording-interface.h"
// The concrete implementation we are testing against
#include "../../src/services/AudioRecorderService.h"

using namespace std::chrono;

class AudioRecorderContractTest : public ::testing::Test {
protected:
    void SetUp() override {
        recorder = new AudioRecorderService();
        tempDir = new QTemporaryDir();
        isCIEnvironment = qEnvironmentVariableIsSet("AUDIO_TEST_MODE") && 
                         qEnvironmentVariable("AUDIO_TEST_MODE") == "CI";
        
        // Log audio device availability for CI debugging
        if (isCIEnvironment) {
            auto devices = recorder->getAvailableDevices();
            qDebug() << "CI Audio Setup - Available input devices:" << devices.size();
            for (int i = 0; i < devices.size(); ++i) {
                qDebug() << "  Device" << i << ":" << devices[i].description() 
                         << "ID:" << devices[i].id();
            }
            if (devices.isEmpty()) {
                qDebug() << "CI Audio Setup - No audio input devices detected";
            }
        }
    }

    void TearDown() override {
        delete recorder;
        delete tempDir;
    }

    // Helper function to check if we have audio devices available
    bool hasAudioDevices() {
        auto devices = recorder->getAvailableDevices();
        return !devices.isEmpty();
    }

    // Helper function to skip test in CI when no audio devices
    // Returns true if test should be skipped, false if test should continue
    bool skipIfNoAudioDevices(const QString& testName) {
        if (isCIEnvironment && !hasAudioDevices()) {
            GTEST_SKIP() << testName.toStdString() << " - Skipping in CI environment without audio devices";
            return true;
        }
        return false;
    }

    AudioRecorderService* recorder = nullptr;
    QTemporaryDir* tempDir = nullptr;
    bool isCIEnvironment = false;
};

// Contract Test 1: Recording Start Time < 500ms (FR-004, PR-004)
TEST_F(AudioRecorderContractTest, RecordingStartLatencyUnder500ms) {
    ASSERT_NE(recorder, nullptr);
    if (skipIfNoAudioDevices("RecordingStartLatencyUnder500ms")) return;

    QString testPath = tempDir->filePath("test_recording.wav");
    
    // Measure recording start time
    auto startTime = high_resolution_clock::now();
    bool result = recorder->startRecording(testPath);
    auto endTime = high_resolution_clock::now();
    
    auto latency = duration_cast<milliseconds>(endTime - startTime);
    
    // TDD: These should FAIL initially until implementation exists
    EXPECT_TRUE(result) << "Recording should start successfully";
    EXPECT_LT(latency.count(), 500) << "Recording start latency must be < 500ms (FR-004)";
    EXPECT_EQ(recorder->getState(), AudioRecordingState::Recording);
}

// Contract Test 2: Pause/Resume Functionality (FR-012)
TEST_F(AudioRecorderContractTest, PauseResumeRecording) {
    if (skipIfNoAudioDevices("PauseResumeRecording")) return;
    
    QString testPath = tempDir->filePath("pause_test.wav");
    
    // Start recording
    recorder->startRecording(testPath);
    EXPECT_EQ(recorder->getState(), AudioRecordingState::Recording);
    
    // Pause recording
    recorder->pauseRecording();
    EXPECT_EQ(recorder->getState(), AudioRecordingState::Paused);
    
    // Resume recording
    recorder->resumeRecording();
    EXPECT_EQ(recorder->getState(), AudioRecordingState::Recording);
    
    recorder->stopRecording();
    EXPECT_EQ(recorder->getState(), AudioRecordingState::Stopped);
}

// Contract Test 3: Device Enumeration and Selection
TEST_F(AudioRecorderContractTest, DeviceEnumerationAndSelection) {
    auto devices = recorder->getAvailableDevices();
    
    // In CI environment, we expect this test to be meaningful only if audio devices exist
    if (isCIEnvironment) {
        if (devices.isEmpty()) {
            GTEST_SKIP() << "DeviceEnumerationAndSelection - No audio devices in CI environment";
        }
    } else {
        // TDD: Should FAIL - no devices available in mock
        EXPECT_GT(devices.size(), 0) << "Should have at least one audio device available";
    }
    
    if (!devices.isEmpty()) {
        bool deviceSet = recorder->setRecordingDevice(devices.first());
        EXPECT_TRUE(deviceSet) << "Should be able to set recording device";
        
        auto currentDevice = recorder->getCurrentDevice();
        EXPECT_EQ(currentDevice.description(), devices.first().description());
    }
}

// Contract Test 4: Audio Format Validation
TEST_F(AudioRecorderContractTest, AudioFormatValidation) {
    auto recommendedFormat = recorder->getRecommendedFormat();
    
    // Validate recommended format for whisper.cpp (16kHz, 16-bit, mono)
    EXPECT_EQ(recommendedFormat.sampleRate(), 16000);
    EXPECT_EQ(recommendedFormat.channelCount(), 1);
    EXPECT_EQ(recommendedFormat.sampleFormat(), QAudioFormat::Int16);
    
    // Test format setting
    recorder->setAudioFormat(recommendedFormat);
    auto currentFormat = recorder->getAudioFormat();
    EXPECT_EQ(currentFormat.sampleRate(), recommendedFormat.sampleRate());
}

// Contract Test 5: Real-time Level Monitoring (FR-011)
TEST_F(AudioRecorderContractTest, RealtimeLevelMonitoring) {
    if (skipIfNoAudioDevices("RealtimeLevelMonitoring")) return;
    
    QSignalSpy inputLevelSpy(recorder, &IAudioRecorder::inputLevelChanged);
    QSignalSpy audioDataSpy(recorder, &IAudioRecorder::audioDataReady);
    
    QString testPath = tempDir->filePath("monitor_test.wav");
    recorder->startRecording(testPath);
    
    // TDD: Should FAIL - no signals emitted in mock
    EXPECT_GE(recorder->getCurrentInputLevel(), 0.0);
    EXPECT_LE(recorder->getCurrentInputLevel(), 1.0);
    
    // Wait for some audio data signals (in real implementation)
    QTest::qWait(100);
    
    // TDD: These will fail until real implementation
    EXPECT_GT(inputLevelSpy.count(), 0) << "Should emit input level changes during recording";
    EXPECT_GT(audioDataSpy.count(), 0) << "Should emit audio data for visualization";
}

// Contract Test 6: Error Handling for Device Access Issues
TEST_F(AudioRecorderContractTest, ErrorHandlingDeviceAccess) {
    QSignalSpy errorSpy(recorder, &IAudioRecorder::errorOccurred);
    
    // Try to use non-existent device
    QAudioDevice invalidDevice;
    bool result = recorder->setRecordingDevice(invalidDevice);
    
    // TDD: Should FAIL until proper error handling implemented
    EXPECT_FALSE(result);
    EXPECT_NE(recorder->getLastError(), AudioError::NoError);
    EXPECT_FALSE(recorder->getErrorString().isEmpty());
    
    // Verify error signal was emitted
    EXPECT_GT(errorSpy.count(), 0);
}

// Contract Test 7: Recording Duration Accuracy
TEST_F(AudioRecorderContractTest, RecordingDurationAccuracy) {
    if (skipIfNoAudioDevices("RecordingDurationAccuracy")) return;
    
    QSignalSpy durationSpy(recorder, &IAudioRecorder::durationChanged);
    
    QString testPath = tempDir->filePath("duration_test.wav");
    recorder->startRecording(testPath);
    
    // Simulate 1 second of recording
    QTest::qWait(1000);
    
    qint64 duration = recorder->getRecordingDuration();
    
    // TDD: Should FAIL - mock returns 0
    EXPECT_GE(duration, 900) << "Duration should be at least 900ms after 1s";
    EXPECT_LE(duration, 1100) << "Duration should be at most 1100ms after 1s";
    
    recorder->stopRecording();
    EXPECT_GT(durationSpy.count(), 0) << "Should emit duration changes during recording";
}

// Contract Test 8: File Output Format Compliance
TEST_F(AudioRecorderContractTest, FileOutputFormatCompliance) {
    if (skipIfNoAudioDevices("FileOutputFormatCompliance")) return;
    
    QSignalSpy recordingStopped(recorder, &IAudioRecorder::recordingStopped);
    
    QString testPath = tempDir->filePath("output_test.wav");
    recorder->startRecording(testPath);
    
    // Record for short time
    QTest::qWait(500);
    recorder->stopRecording();
    
    // TDD: Should FAIL - no file created in mock
    EXPECT_TRUE(QFile::exists(testPath)) << "Recording file should be created";
    
    // Verify signal emission
    EXPECT_GT(recordingStopped.count(), 0);
    if (recordingStopped.count() > 0) {
        auto arguments = recordingStopped.first();
        QString filePath = arguments.at(0).toString();
        qint64 duration = arguments.at(1).toLongLong();
        
        EXPECT_EQ(filePath, testPath);
        EXPECT_GT(duration, 0);
    }
}

// Contract Test 9: Memory Usage During Long Recordings
TEST_F(AudioRecorderContractTest, MemoryUsageLongRecordings) {
    if (skipIfNoAudioDevices("MemoryUsageLongRecordings")) return;
    
    QString testPath = tempDir->filePath("long_recording.wav");
    
    // Start recording
    recorder->startRecording(testPath);
    
    // Simulate longer recording (reduced time for testing)
    for (int i = 0; i < 5; ++i) {
        QTest::qWait(200);
        qint64 bytes = recorder->getRecordedBytes();
        
        // TDD: Should FAIL - mock returns 0
        EXPECT_GT(bytes, 0) << "Should track recorded bytes";
        
        // Memory usage should not grow excessively
        // In real implementation, would check actual memory usage
    }
    
    recorder->stopRecording();
}

// Contract Test 10: Device Change Handling During Recording
TEST_F(AudioRecorderContractTest, DeviceChangeHandlingDuringRecording) {
    if (skipIfNoAudioDevices("DeviceChangeHandlingDuringRecording")) return;
    
    QString testPath = tempDir->filePath("device_change_test.wav");
    
    recorder->startRecording(testPath);
    EXPECT_EQ(recorder->getState(), AudioRecordingState::Recording);
    
    // Simulate device change
    recorder->onDeviceChanged();
    
    // TDD: Should FAIL - no proper handling in mock
    // Recording should continue or handle gracefully
    AudioRecordingState state = recorder->getState();
    EXPECT_TRUE(state == AudioRecordingState::Recording || 
                state == AudioRecordingState::Error) << "Should handle device changes gracefully";
    
    recorder->stopRecording();
}

// Performance Test: Recording Start Latency Benchmark
TEST_F(AudioRecorderContractTest, DISABLED_BenchmarkRecordingStartLatency) {
    const int numIterations = 10;
    std::vector<int> latencies;
    
    for (int i = 0; i < numIterations; ++i) {
        QString testPath = tempDir->filePath(QString("benchmark_%1.wav").arg(i));
        
        auto startTime = high_resolution_clock::now();
        recorder->startRecording(testPath);
        auto endTime = high_resolution_clock::now();
        
        recorder->cancelRecording();
        
        int latency = duration_cast<milliseconds>(endTime - startTime).count();
        latencies.push_back(latency);
    }
    
    // Calculate average latency
    double avgLatency = std::accumulate(latencies.begin(), latencies.end(), 0.0) / numIterations;
    
    EXPECT_LT(avgLatency, 500.0) << "Average recording start latency should be < 500ms";
    
    // Log individual measurements for analysis
    for (size_t i = 0; i < latencies.size(); ++i) {
        std::cout << "Iteration " << i + 1 << ": " << latencies[i] << "ms" << std::endl;
    }
    std::cout << "Average latency: " << avgLatency << "ms" << std::endl;
}

// Include moc file for Qt's MOC system
#include "test_audio_recorder_contract.moc"