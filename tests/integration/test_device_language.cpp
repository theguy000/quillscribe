// Integration Test for Device and Language Management
// Tests user story: Support device selection and language switching
// Must FAIL initially - no implementation exists yet (TDD)

#include <gtest/gtest.h>
#include <QtTest>
#include <QSignalSpy>
#include <QAudioDeviceInfo>
#include <QAudioFormat>

// TODO: These will fail to compile until interfaces and implementations exist
// #include "../../specs/001-voice-to-text/contracts/audio-recording-interface.h"
// #include "../../specs/001-voice-to-text/contracts/transcription-service-interface.h"

class DeviceLanguageIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // TODO: Initialize real services when implementations exist
        // audioRecorder = createAudioRecorder();
        // transcriptionService = createTranscriptionService();
    }

    void TearDown() override {
        // TODO: Cleanup real services
    }

    // TODO: Add real service pointers when implementations exist
    // IAudioRecorder* audioRecorder = nullptr;
    // ITranscriptionService* transcriptionService = nullptr;
};

// Integration Test 1: Audio Device Enumeration and Selection
TEST_F(DeviceLanguageIntegrationTest, AudioDeviceEnumerationSelection) {
    // TDD: This test MUST FAIL initially since no implementations exist
    
    // TODO: Uncomment when real implementations exist
    /*
    // Test device enumeration
    QList<QAudioDeviceInfo> availableDevices = audioRecorder->getAvailableDevices();
    
    ASSERT_GT(availableDevices.size(), 0) << "Should have at least one audio input device available";
    
    // Verify device information is complete
    for (const auto& device : availableDevices) {
        EXPECT_FALSE(device.deviceName().isEmpty()) << "Device should have a name";
        EXPECT_TRUE(device.isFormatSupported(audioRecorder->getRecommendedFormat())) 
            << "Device should support recommended format";
    }
    
    // Test device availability check
    bool deviceAvailable = audioRecorder->isDeviceAvailable();
    EXPECT_TRUE(deviceAvailable) << "At least one device should be available";
    
    // Get current device
    QAudioDeviceInfo currentDevice = audioRecorder->getCurrentDevice();
    EXPECT_FALSE(currentDevice.deviceName().isEmpty()) << "Should have a current device set";
    
    // Test device switching
    if (availableDevices.size() > 1) {
        // Find different device
        QAudioDeviceInfo targetDevice;
        for (const auto& device : availableDevices) {
            if (device.deviceName() != currentDevice.deviceName()) {
                targetDevice = device;
                break;
            }
        }
        
        if (!targetDevice.deviceName().isEmpty()) {
            QSignalSpy deviceChangedSpy(audioRecorder, &IAudioRecorder::stateChanged);
            
            bool deviceSet = audioRecorder->setRecordingDevice(targetDevice);
            EXPECT_TRUE(deviceSet) << "Should be able to set different recording device";
            
            QAudioDeviceInfo newCurrentDevice = audioRecorder->getCurrentDevice();
            EXPECT_EQ(newCurrentDevice.deviceName(), targetDevice.deviceName()) 
                << "Current device should change to new device";
            
            // Should handle device change gracefully
            EXPECT_GE(deviceChangedSpy.count(), 0) << "Device change should be handled properly";
        }
    }
    
    // Test device change handling during recording
    QTemporaryDir tempDir;
    QString recordingPath = tempDir.filePath("device_change_test.wav");
    
    bool recordingStarted = audioRecorder->startRecording(recordingPath);
    ASSERT_TRUE(recordingStarted) << "Should start recording";
    
    // Simulate device change during recording
    audioRecorder->onDeviceChanged();
    
    // Recording should continue or handle gracefully
    AudioRecordingState state = audioRecorder->getState();
    EXPECT_TRUE(state == AudioRecordingState::Recording || 
               state == AudioRecordingState::Error) << "Should handle device changes during recording";
    
    if (state == AudioRecordingState::Error) {
        AudioError error = audioRecorder->getLastError();
        EXPECT_EQ(error, AudioError::DeviceNotFound) << "Should report appropriate error for device issues";
    }
    
    audioRecorder->stopRecording();
    */
    
    FAIL() << "Device enumeration test not implemented - needs IAudioRecorder implementation";
}

// Integration Test 2: Audio Format Configuration and Validation
TEST_F(DeviceLanguageIntegrationTest, AudioFormatConfigurationValidation) {
    // TDD: This test MUST FAIL initially
    
    // TODO: Uncomment when real implementations exist
    /*
    // Test recommended format for whisper.cpp
    QAudioFormat recommendedFormat = audioRecorder->getRecommendedFormat();
    
    EXPECT_EQ(recommendedFormat.sampleRate(), 16000) << "Recommended format should be 16kHz for whisper.cpp";
    EXPECT_EQ(recommendedFormat.channelCount(), 1) << "Recommended format should be mono";
    EXPECT_EQ(recommendedFormat.sampleSize(), 16) << "Recommended format should be 16-bit";
    EXPECT_EQ(recommendedFormat.codec(), "audio/pcm") << "Recommended format should be PCM";
    
    // Test setting audio format
    audioRecorder->setAudioFormat(recommendedFormat);
    
    QAudioFormat currentFormat = audioRecorder->getAudioFormat();
    EXPECT_EQ(currentFormat.sampleRate(), recommendedFormat.sampleRate()) << "Should set sample rate";
    EXPECT_EQ(currentFormat.channelCount(), recommendedFormat.channelCount()) << "Should set channel count";
    EXPECT_EQ(currentFormat.sampleSize(), recommendedFormat.sampleSize()) << "Should set sample size";
    
    // Test format compatibility with devices
    auto availableDevices = audioRecorder->getAvailableDevices();
    ASSERT_GT(availableDevices.size(), 0) << "Should have available devices";
    
    for (const auto& device : availableDevices) {
        bool formatSupported = device.isFormatSupported(recommendedFormat);
        
        if (formatSupported) {
            audioRecorder->setRecordingDevice(device);
            
            // Test recording with this format
            QTemporaryDir tempDir;
            QString testPath = tempDir.filePath("format_test.wav");
            
            bool recordingStarted = audioRecorder->startRecording(testPath);
            EXPECT_TRUE(recordingStarted) << "Should record with recommended format on device: " 
                                         << device.deviceName();
            
            if (recordingStarted) {
                QTest::qWait(2000); // 2 seconds
                audioRecorder->stopRecording();
                
                EXPECT_TRUE(QFile::exists(testPath)) << "Should create recording file with correct format";
                
                // File should be created in the right format for whisper.cpp processing
                QFileInfo fileInfo(testPath);
                EXPECT_GT(fileInfo.size(), 0) << "Recording file should have content";
            }
        }
    }
    
    // Test invalid format handling
    QAudioFormat invalidFormat;
    invalidFormat.setSampleRate(44100); // Different from recommended
    invalidFormat.setChannelCount(2);   // Stereo instead of mono
    
    audioRecorder->setAudioFormat(invalidFormat);
    
    // Should either accept and convert, or maintain recommended format
    QAudioFormat resultFormat = audioRecorder->getAudioFormat();
    
    // Either the format was changed, or it remained at recommended settings
    bool formatHandled = (resultFormat.sampleRate() == invalidFormat.sampleRate()) ||
                        (resultFormat.sampleRate() == recommendedFormat.sampleRate());
    EXPECT_TRUE(formatHandled) << "Should handle format changes appropriately";
    */
    
    FAIL() << "Audio format configuration test not implemented - needs format handling";
}

// Integration Test 3: Language Detection and Multi-language Support
TEST_F(DeviceLanguageIntegrationTest, LanguageDetectionMultiLanguageSupport) {
    // TDD: This test MUST FAIL initially
    
    // TODO: Uncomment when real implementations exist
    /*
    // Test supported languages
    QStringList supportedLanguages = transcriptionService->getSupportedLanguages();
    
    ASSERT_GT(supportedLanguages.size(), 0) << "Should support multiple languages";
    EXPECT_TRUE(supportedLanguages.contains("en")) << "Should support English";
    
    // Test language setting
    transcriptionService->setDefaultLanguage("en");
    
    // Test automatic language detection
    QTemporaryDir tempDir;
    QString englishRecording = tempDir.filePath("english_test.wav");
    
    // Create test recording (would be actual speech in real test)
    audioRecorder->startRecording(englishRecording);
    QTest::qWait(10000); // 10 seconds of English speech
    audioRecorder->stopRecording();
    
    QString detectedLanguage = transcriptionService->detectLanguage(englishRecording);
    EXPECT_FALSE(detectedLanguage.isEmpty()) << "Should detect language from audio";
    EXPECT_EQ(detectedLanguage.toLower(), "en") << "Should detect English correctly";
    
    // Test transcription with specific language
    TranscriptionRequest englishRequest;
    englishRequest.audioFilePath = englishRecording;
    englishRequest.language = "en";
    englishRequest.preferredProvider = TranscriptionProvider::WhisperCpp;
    
    QString englishRequestId = transcriptionService->submitTranscription(englishRequest);
    ASSERT_FALSE(englishRequestId.isEmpty()) << "Should submit English transcription";
    
    // Wait for completion
    int maxWait = 15000;
    int waited = 0;
    while (waited < maxWait) {
        if (transcriptionService->getTranscriptionStatus(englishRequestId) == TranscriptionStatus::Completed) {
            break;
        }
        QTest::qWait(500);
        waited += 500;
    }
    
    EXPECT_EQ(transcriptionService->getTranscriptionStatus(englishRequestId), TranscriptionStatus::Completed);
    
    TranscriptionResult englishResult = transcriptionService->getTranscriptionResult(englishRequestId);
    EXPECT_FALSE(englishResult.text.isEmpty()) << "Should produce English transcription";
    EXPECT_EQ(englishResult.language, "en") << "Result should indicate English language";
    
    // Test auto language detection
    TranscriptionRequest autoRequest;
    autoRequest.audioFilePath = englishRecording;
    autoRequest.language = "auto";
    autoRequest.preferredProvider = TranscriptionProvider::WhisperCpp;
    
    QString autoRequestId = transcriptionService->submitTranscription(autoRequest);
    ASSERT_FALSE(autoRequestId.isEmpty()) << "Should submit auto-detect transcription";
    
    // Wait for completion
    waited = 0;
    while (waited < maxWait) {
        if (transcriptionService->getTranscriptionStatus(autoRequestId) == TranscriptionStatus::Completed) {
            break;
        }
        QTest::qWait(500);
        waited += 500;
    }
    
    EXPECT_EQ(transcriptionService->getTranscriptionStatus(autoRequestId), TranscriptionStatus::Completed);
    
    TranscriptionResult autoResult = transcriptionService->getTranscriptionResult(autoRequestId);
    EXPECT_FALSE(autoResult.text.isEmpty()) << "Should produce auto-detected transcription";
    EXPECT_FALSE(autoResult.language.isEmpty()) << "Should detect and report language";
    
    // Results should be similar for same audio
    EXPECT_EQ(autoResult.language, "en") << "Auto-detection should identify English";
    
    // Test language switching during operation
    if (supportedLanguages.contains("es") || supportedLanguages.contains("fr")) {
        QString alternativeLanguage = supportedLanguages.contains("es") ? "es" : "fr";
        transcriptionService->setDefaultLanguage(alternativeLanguage);
        
        // Service should handle language switching
        EXPECT_NO_THROW({
            TranscriptionRequest switchRequest;
            switchRequest.audioFilePath = englishRecording;
            switchRequest.language = alternativeLanguage;
            switchRequest.preferredProvider = TranscriptionProvider::WhisperCpp;
            
            QString switchRequestId = transcriptionService->submitTranscription(switchRequest);
            // Don't wait for completion in this test, just verify it accepts the request
            EXPECT_FALSE(switchRequestId.isEmpty()) << "Should accept different language requests";
        });
    }
    */
    
    FAIL() << "Language detection test not implemented - needs whisper.cpp with language support";
}

// Integration Test 4: Device and Language Settings Persistence
TEST_F(DeviceLanguageIntegrationTest, DeviceLanguageSettingsPersistence) {
    // TDD: This test MUST FAIL initially
    
    // TODO: Uncomment when real implementations exist
    /*
    // Test device preference persistence
    auto availableDevices = audioRecorder->getAvailableDevices();
    ASSERT_GT(availableDevices.size(), 0) << "Should have available devices";
    
    QAudioDeviceInfo preferredDevice = availableDevices.first();
    bool deviceSet = audioRecorder->setRecordingDevice(preferredDevice);
    ASSERT_TRUE(deviceSet) << "Should set preferred device";
    
    // Test language preference persistence
    transcriptionService->setDefaultLanguage("en");
    
    // Simulate application restart by creating new service instances
    // In real implementation, would involve saving/loading configuration
    
    // After restart, preferences should be restored
    QAudioDeviceInfo currentDevice = audioRecorder->getCurrentDevice();
    EXPECT_EQ(currentDevice.deviceName(), preferredDevice.deviceName()) 
        << "Device preference should persist across sessions";
    
    // Language preference should also persist
    // In real implementation, would check stored default language
    EXPECT_TRUE(true) << "Language preference persistence would be tested with real config system";
    
    // Test format preferences persistence
    QAudioFormat customFormat = audioRecorder->getRecommendedFormat();
    customFormat.setSampleRate(22050); // Custom sample rate
    
    audioRecorder->setAudioFormat(customFormat);
    
    QAudioFormat currentFormat = audioRecorder->getAudioFormat();
    EXPECT_EQ(currentFormat.sampleRate(), customFormat.sampleRate()) 
        << "Format preferences should be applied";
    
    // Test settings validation on load
    // Invalid settings should fall back to defaults
    QAudioFormat invalidFormat;
    invalidFormat.setSampleRate(0); // Invalid sample rate
    
    audioRecorder->setAudioFormat(invalidFormat);
    
    QAudioFormat resultFormat = audioRecorder->getAudioFormat();
    EXPECT_GT(resultFormat.sampleRate(), 0) << "Should fallback to valid format when invalid format is set";
    */
    
    FAIL() << "Settings persistence test not implemented - needs configuration system";
}

// Integration Test 5: Device Hot-plug and Dynamic Changes
TEST_F(DeviceLanguageIntegrationTest, DeviceHotplugDynamicChanges) {
    // TDD: This test MUST FAIL initially
    
    // TODO: Uncomment when real implementations exist
    /*
    QSignalSpy deviceChangedSpy(audioRecorder, &IAudioRecorder::stateChanged);
    QSignalSpy errorSpy(audioRecorder, &IAudioRecorder::errorOccurred);
    
    // Get initial device list
    auto initialDevices = audioRecorder->getAvailableDevices();
    ASSERT_GT(initialDevices.size(), 0) << "Should have initial devices";
    
    QAudioDeviceInfo currentDevice = audioRecorder->getCurrentDevice();
    ASSERT_FALSE(currentDevice.deviceName().isEmpty()) << "Should have current device";
    
    // Test device availability monitoring
    bool isAvailable = audioRecorder->isDeviceAvailable();
    EXPECT_TRUE(isAvailable) << "Current device should be available";
    
    // Simulate device hotplug (device added)
    // In real implementation, this would be triggered by system events
    audioRecorder->onDeviceChanged();
    
    // Check if device list is updated
    auto updatedDevices = audioRecorder->getAvailableDevices();
    EXPECT_GE(updatedDevices.size(), 0) << "Should handle device list updates";
    
    // Test recording during device changes
    QTemporaryDir tempDir;
    QString testRecording = tempDir.filePath("hotplug_test.wav");
    
    bool recordingStarted = audioRecorder->startRecording(testRecording);
    ASSERT_TRUE(recordingStarted) << "Should start recording";
    
    // Simulate device removal during recording
    audioRecorder->onDeviceChanged();
    
    // Should handle device change gracefully
    AudioRecordingState state = audioRecorder->getState();
    
    if (state == AudioRecordingState::Error) {
        // Should report appropriate error
        AudioError error = audioRecorder->getLastError();
        EXPECT_TRUE(error == AudioError::DeviceNotFound || 
                   error == AudioError::DeviceAccessDenied) << "Should report device-related error";
        
        EXPECT_GT(errorSpy.count(), 0) << "Should emit error signal";
        
        // Should attempt recovery
        QString errorString = audioRecorder->getErrorString();
        EXPECT_FALSE(errorString.isEmpty()) << "Should provide error description";
        
    } else if (state == AudioRecordingState::Recording) {
        // Recording continues - device change handled seamlessly
        EXPECT_TRUE(true) << "Device change handled seamlessly during recording";
        
        QTest::qWait(2000);
        audioRecorder->stopRecording();
        
        EXPECT_TRUE(QFile::exists(testRecording)) << "Recording should complete despite device changes";
    }
    
    // Test device recovery
    if (audioRecorder->getState() == AudioRecordingState::Error) {
        audioRecorder->clearErrorState();
        
        // Try to start recording again (should work if recovery is successful)
        QString recoveryTest = tempDir.filePath("recovery_test.wav");
        bool recoveryStarted = audioRecorder->startRecording(recoveryTest);
        
        if (recoveryStarted) {
            EXPECT_EQ(audioRecorder->getState(), AudioRecordingState::Recording) 
                << "Should recover from device errors";
            audioRecorder->stopRecording();
        }
    }
    
    // Test device preference handling when preferred device is unavailable
    if (initialDevices.size() > 1) {
        // Try to set a device that might not be available
        audioRecorder->setRecordingDevice(initialDevices.last());
        
        // Should either succeed or gracefully fall back
        bool isCurrentAvailable = audioRecorder->isDeviceAvailable();
        if (!isCurrentAvailable) {
            // Should fall back to an available device
            QAudioDeviceInfo fallbackDevice = audioRecorder->getCurrentDevice();
            EXPECT_FALSE(fallbackDevice.deviceName().isEmpty()) << "Should fall back to available device";
        }
    }
    */
    
    FAIL() << "Device hot-plug test not implemented - needs device change detection";
}

// Integration Test 6: Language-Specific Transcription Quality
TEST_F(DeviceLanguageIntegrationTest, LanguageSpecificTranscriptionQuality) {
    // TDD: This test MUST FAIL initially
    
    // TODO: Uncomment when real implementations exist
    /*
    // This test would verify that language-specific models perform better than generic ones
    // It requires actual audio samples in different languages
    
    QTemporaryDir tempDir;
    
    // Test English transcription accuracy
    QString englishAudio = tempDir.filePath("english_sample.wav");
    
    // Create English audio sample
    audioRecorder->startRecording(englishAudio);
    QTest::qWait(15000); // 15 seconds of English speech
    audioRecorder->stopRecording();
    
    // Test with English-specific settings
    TranscriptionRequest englishRequest;
    englishRequest.audioFilePath = englishAudio;
    englishRequest.language = "en";
    englishRequest.preferredProvider = TranscriptionProvider::WhisperCpp;
    
    QString englishId = transcriptionService->submitTranscription(englishRequest);
    ASSERT_FALSE(englishId.isEmpty()) << "Should submit English transcription";
    
    // Wait for completion
    int maxWait = 20000;
    int waited = 0;
    while (waited < maxWait) {
        if (transcriptionService->getTranscriptionStatus(englishId) == TranscriptionStatus::Completed) {
            break;
        }
        QTest::qWait(500);
        waited += 500;
    }
    
    EXPECT_EQ(transcriptionService->getTranscriptionStatus(englishId), TranscriptionStatus::Completed);
    
    TranscriptionResult englishResult = transcriptionService->getTranscriptionResult(englishId);
    
    // Quality checks for English transcription
    EXPECT_FALSE(englishResult.text.isEmpty()) << "Should produce English transcription";
    EXPECT_GE(englishResult.confidence, 0.90) << "English transcription should have high confidence";
    EXPECT_EQ(englishResult.language, "en") << "Should detect English language";
    
    // Test auto-detection with same audio
    TranscriptionRequest autoRequest;
    autoRequest.audioFilePath = englishAudio;
    autoRequest.language = "auto";
    autoRequest.preferredProvider = TranscriptionProvider::WhisperCpp;
    
    QString autoId = transcriptionService->submitTranscription(autoRequest);
    ASSERT_FALSE(autoId.isEmpty()) << "Should submit auto-detection transcription";
    
    waited = 0;
    while (waited < maxWait) {
        if (transcriptionService->getTranscriptionStatus(autoId) == TranscriptionStatus::Completed) {
            break;
        }
        QTest::qWait(500);
        waited += 500;
    }
    
    TranscriptionResult autoResult = transcriptionService->getTranscriptionResult(autoId);
    
    // Auto-detection should be comparable to language-specific
    EXPECT_EQ(autoResult.language, "en") << "Auto-detection should identify English";
    
    // Confidence should be similar (within 10%)
    double confidenceDiff = std::abs(autoResult.confidence - englishResult.confidence);
    EXPECT_LT(confidenceDiff, 0.10) << "Auto-detection confidence should be comparable to language-specific";
    
    // Test transcription quality metrics
    double englishQuality = transcriptionService->getProviderAccuracy(TranscriptionProvider::WhisperCpp);
    EXPECT_GT(englishQuality, 0.90) << "Provider should maintain high accuracy for English";
    
    // Log quality results
    std::cout << "English transcription quality results:" << std::endl;
    std::cout << "Language-specific confidence: " << englishResult.confidence << std::endl;
    std::cout << "Auto-detection confidence: " << autoResult.confidence << std::endl;
    std::cout << "Provider accuracy: " << englishQuality << std::endl;
    std::cout << "Processing time: " << englishResult.processingTime << "ms" << std::endl;
    
    // If multiple languages are supported, test language switching performance
    auto supportedLanguages = transcriptionService->getSupportedLanguages();
    if (supportedLanguages.size() > 1) {
        for (const QString& language : supportedLanguages) {
            if (language != "en") {
                // Test switching to different language
                transcriptionService->setDefaultLanguage(language);
                
                TranscriptionRequest langRequest;
                langRequest.audioFilePath = englishAudio; // Same audio, different expected language
                langRequest.language = language;
                langRequest.preferredProvider = TranscriptionProvider::WhisperCpp;
                
                QString langId = transcriptionService->submitTranscription(langRequest);
                if (!langId.isEmpty()) {
                    // Wait briefly to see if it processes
                    QTest::qWait(5000);
                    
                    TranscriptionStatus status = transcriptionService->getTranscriptionStatus(langId);
                    if (status == TranscriptionStatus::Completed) {
                        TranscriptionResult langResult = transcriptionService->getTranscriptionResult(langId);
                        
                        // Wrong language should have lower confidence
                        EXPECT_LT(langResult.confidence, englishResult.confidence) 
                            << "Wrong language setting should have lower confidence";
                    }
                }
                
                break; // Only test one alternative language
            }
        }
    }
    */
    
    FAIL() << "Language-specific quality test not implemented - needs multi-language whisper support";
}

// Performance Test: Device and Language Performance Impact
TEST_F(DeviceLanguageIntegrationTest, DISABLED_DeviceLanguagePerformanceImpact) {
    // TODO: Implement when services are available
    /*
    // Test performance impact of different devices and languages
    
    QTemporaryDir tempDir;
    QString testAudio = tempDir.filePath("performance_test.wav");
    
    // Create standard test recording
    audioRecorder->startRecording(testAudio);
    QTest::qWait(30000); // 30 seconds
    audioRecorder->stopRecording();
    
    auto supportedLanguages = transcriptionService->getSupportedLanguages();
    
    std::map<QString, qint64> languagePerformance;
    
    for (const QString& language : supportedLanguages) {
        if (language == "auto") continue; // Skip auto-detection for this test
        
        TranscriptionRequest request;
        request.audioFilePath = testAudio;
        request.language = language;
        request.preferredProvider = TranscriptionProvider::WhisperCpp;
        
        auto startTime = std::chrono::high_resolution_clock::now();
        QString requestId = transcriptionService->submitTranscription(request);
        
        if (!requestId.isEmpty()) {
            // Wait for completion
            while (transcriptionService->getTranscriptionStatus(requestId) != TranscriptionStatus::Completed &&
                   transcriptionService->getTranscriptionStatus(requestId) != TranscriptionStatus::Failed) {
                QTest::qWait(500);
            }
            
            auto endTime = std::chrono::high_resolution_clock::now();
            qint64 processingTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
            
            if (transcriptionService->getTranscriptionStatus(requestId) == TranscriptionStatus::Completed) {
                languagePerformance[language] = processingTime;
                
                TranscriptionResult result = transcriptionService->getTranscriptionResult(requestId);
                std::cout << "Language " << language.toStdString() << ": " 
                          << processingTime << "ms, confidence: " << result.confidence << std::endl;
            }
        }
    }
    
    // Analyze performance differences
    if (languagePerformance.size() > 1) {
        auto minTime = std::min_element(languagePerformance.begin(), languagePerformance.end(),
                                       [](const auto& a, const auto& b) { return a.second < b.second; });
        auto maxTime = std::max_element(languagePerformance.begin(), languagePerformance.end(),
                                       [](const auto& a, const auto& b) { return a.second < b.second; });
        
        std::cout << "Fastest: " << minTime->first.toStdString() << " (" << minTime->second << "ms)" << std::endl;
        std::cout << "Slowest: " << maxTime->first.toStdString() << " (" << maxTime->second << "ms)" << std::endl;
        
        // Performance difference should be reasonable (within 3x)
        EXPECT_LT(static_cast<double>(maxTime->second) / minTime->second, 3.0) 
            << "Language performance difference should be reasonable";
    }
    */
    
    SUCCEED() << "Performance test skipped - implementation needed";
}
