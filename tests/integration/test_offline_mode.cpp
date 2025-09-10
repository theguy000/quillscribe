// Integration Test for Offline Mode Functionality
// Tests user story: Core features work without internet connection
// Must FAIL initially - no implementation exists yet (TDD)

#include <gtest/gtest.h>
#include <QtTest>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QNetworkAccessManager>
#include <QNetworkReply>

// TODO: These will fail to compile until interfaces and implementations exist
// #include "../../specs/001-voice-to-text/contracts/audio-recording-interface.h"
// #include "../../specs/001-voice-to-text/contracts/transcription-service-interface.h"
// #include "../../specs/001-voice-to-text/contracts/ai-enhancement-interface.h"
// #include "../../specs/001-voice-to-text/contracts/storage-interface.h"

class OfflineModeIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        tempDir = new QTemporaryDir();
        // TODO: Initialize real services when implementations exist
        // audioRecorder = createAudioRecorder();
        // transcriptionService = createTranscriptionService();
        // enhancementService = createTextEnhancementService();
        // storageManager = createStorageManager();
    }

    void TearDown() override {
        delete tempDir;
        // TODO: Cleanup real services
    }

    QTemporaryDir* tempDir = nullptr;
    // TODO: Add real service pointers when implementations exist
    // IAudioRecorder* audioRecorder = nullptr;
    // ITranscriptionService* transcriptionService = nullptr;
    // ITextEnhancementService* enhancementService = nullptr;
    // IStorageManager* storageManager = nullptr;
    
    bool isNetworkAvailable() {
        // Simple network availability check
        QNetworkAccessManager manager;
        QNetworkRequest request(QUrl("http://www.google.com"));
        request.setRawHeader("User-Agent", "QuillScribe Test");
        
        QNetworkReply* reply = manager.get(request);
        QEventLoop loop;
        QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        
        QTimer timer;
        timer.setSingleShot(true);
        QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
        timer.start(3000); // 3 second timeout
        
        loop.exec();
        
        bool available = (reply->error() == QNetworkReply::NoError);
        reply->deleteLater();
        return available;
    }
};

// Integration Test 1: Recording Functionality Works Offline (FR-008)
TEST_F(OfflineModeIntegrationTest, RecordingFunctionalityWorksOffline) {
    // TDD: This test MUST FAIL initially since no implementations exist
    
    // TODO: Uncomment when real implementations exist
    /*
    // Verify recording works without network dependency
    QString recordingPath = tempDir->filePath("offline_recording.wav");
    
    // Test recording functionality
    bool recordingStarted = audioRecorder->startRecording(recordingPath);
    ASSERT_TRUE(recordingStarted) << "Recording should start successfully offline";
    EXPECT_EQ(audioRecorder->getState(), AudioRecordingState::Recording);
    
    // Record for 10 seconds
    QTest::qWait(10000);
    
    // Check real-time monitoring works offline
    double inputLevel = audioRecorder->getCurrentInputLevel();
    EXPECT_GE(inputLevel, 0.0) << "Input level monitoring should work offline";
    EXPECT_LE(inputLevel, 1.0) << "Input level should be valid range";
    
    QByteArray audioData = audioRecorder->getCurrentAudioData();
    EXPECT_GT(audioData.size(), 0) << "Should provide audio data for visualization offline";
    
    // Stop recording
    audioRecorder->stopRecording();
    EXPECT_EQ(audioRecorder->getState(), AudioRecordingState::Stopped);
    
    // Verify file was created
    EXPECT_TRUE(QFile::exists(recordingPath)) << "Recording file should be created offline";
    
    QFileInfo fileInfo(recordingPath);
    EXPECT_GT(fileInfo.size(), 0) << "Recording file should have content";
    
    // Verify recording duration
    qint64 duration = audioRecorder->getRecordingDuration();
    EXPECT_GE(duration, 9500) << "Duration should be at least 9.5 seconds";
    EXPECT_LE(duration, 10500) << "Duration should be at most 10.5 seconds";
    
    // Test multiple recordings offline
    for (int i = 0; i < 3; ++i) {
        QString multiRecordingPath = tempDir->filePath(QString("offline_multi_%1.wav").arg(i + 1));
        
        bool started = audioRecorder->startRecording(multiRecordingPath);
        EXPECT_TRUE(started) << "Multiple recordings should work offline";
        
        QTest::qWait(2000); // 2 seconds each
        
        audioRecorder->stopRecording();
        EXPECT_TRUE(QFile::exists(multiRecordingPath)) << "Multiple recording files should be created offline";
    }
    */
    
    FAIL() << "Offline recording test not implemented - needs IAudioRecorder implementation";
}

// Integration Test 2: Local Transcription with whisper.cpp Works Offline (FR-008)
TEST_F(OfflineModeIntegrationTest, LocalTranscriptionWorksOffline) {
    // TDD: This test MUST FAIL initially
    
    // TODO: Uncomment when real implementations exist
    /*
    // Verify transcription service is offline-capable
    ASSERT_TRUE(transcriptionService->isOfflineCapable()) << "Transcription service should support offline mode (FR-008)";
    
    // Check whisper.cpp models are available
    auto availableProviders = transcriptionService->getAvailableProviders();
    ASSERT_GT(availableProviders.size(), 0) << "Should have offline transcription providers";
    
    bool hasOfflineProvider = std::any_of(availableProviders.begin(), availableProviders.end(),
        [](TranscriptionProvider p) {
            return p == TranscriptionProvider::WhisperCpp || 
                   p == TranscriptionProvider::WhisperCppTiny ||
                   p == TranscriptionProvider::WhisperCppBase;
        });
    
    ASSERT_TRUE(hasOfflineProvider) << "Should have whisper.cpp providers for offline transcription";
    
    // Set offline provider (prefer base model for accuracy)
    TranscriptionProvider offlineProvider = TranscriptionProvider::WhisperCppBase;
    if (!transcriptionService->isProviderAvailable(offlineProvider)) {
        offlineProvider = TranscriptionProvider::WhisperCppTiny; // Fallback to tiny
    }
    
    bool providerSet = transcriptionService->setProvider(offlineProvider);
    ASSERT_TRUE(providerSet) << "Should set offline provider";
    EXPECT_EQ(transcriptionService->getCurrentProvider(), offlineProvider);
    
    // Verify model is downloaded
    bool modelDownloaded = transcriptionService->isModelDownloaded(offlineProvider);
    if (!modelDownloaded) {
        // Try to download model (this would require internet initially)
        bool downloadStarted = transcriptionService->downloadModel(offlineProvider);
        if (downloadStarted) {
            // Wait for download (would be longer in real scenario)
            QTest::qWait(10000);
            modelDownloaded = transcriptionService->isModelDownloaded(offlineProvider);
        }
    }
    
    ASSERT_TRUE(modelDownloaded) << "Offline transcription model should be available";
    
    // Create test audio file for transcription
    QString testAudioPath = tempDir->filePath("offline_transcription_test.wav");
    
    // Record audio for transcription test
    audioRecorder->startRecording(testAudioPath);
    QTest::qWait(15000); // 15 seconds of speech
    audioRecorder->stopRecording();
    
    ASSERT_TRUE(QFile::exists(testAudioPath)) << "Test audio file should exist";
    
    // Test offline transcription
    TranscriptionRequest request;
    request.audioFilePath = testAudioPath;
    request.language = "auto";
    request.preferredProvider = offlineProvider;
    
    QString requestId = transcriptionService->submitTranscription(request);
    ASSERT_FALSE(requestId.isEmpty()) << "Should submit offline transcription request";
    
    // Wait for transcription completion
    int maxWait = 30000; // 30 seconds for offline processing (can be slower)
    int waited = 0;
    while (waited < maxWait) {
        TranscriptionStatus status = transcriptionService->getTranscriptionStatus(requestId);
        if (status == TranscriptionStatus::Completed) {
            break;
        } else if (status == TranscriptionStatus::Failed) {
            FAIL() << "Offline transcription failed: " << transcriptionService->getErrorString();
        }
        QTest::qWait(500);
        waited += 500;
    }
    
    EXPECT_EQ(transcriptionService->getTranscriptionStatus(requestId), TranscriptionStatus::Completed)
        << "Offline transcription should complete successfully";
    
    // Verify transcription result
    TranscriptionResult result = transcriptionService->getTranscriptionResult(requestId);
    
    EXPECT_FALSE(result.text.isEmpty()) << "Should produce transcribed text offline";
    EXPECT_GT(result.confidence, 0.0) << "Should provide confidence score";
    EXPECT_EQ(result.provider, offlineProvider) << "Should use offline provider";
    EXPECT_GT(result.processingTime, 0) << "Should track processing time";
    
    // Verify no network dependency
    EXPECT_TRUE(true) << "Transcription completed without network dependency";
    
    std::cout << "Offline transcription result: " << result.text.toStdString() << std::endl;
    std::cout << "Processing time: " << result.processingTime << "ms" << std::endl;
    std::cout << "Confidence: " << result.confidence << std::endl;
    */
    
    FAIL() << "Offline transcription test not implemented - needs whisper.cpp integration";
}

// Integration Test 3: Storage Operations Work Offline
TEST_F(OfflineModeIntegrationTest, StorageOperationsWorkOffline) {
    // TDD: This test MUST FAIL initially
    
    // TODO: Uncomment when real implementations exist
    /*
    // Initialize local storage
    QString dbPath = tempDir->filePath("offline_storage.sqlite");
    bool storageInitialized = storageManager->initialize(dbPath);
    ASSERT_TRUE(storageInitialized) << "Local storage should initialize without network";
    
    auto sessionStorage = storageManager->getUserSessionStorage();
    auto recordingStorage = storageManager->getRecordingStorage();
    auto transcriptionStorage = storageManager->getTranscriptionStorage();
    
    ASSERT_NE(sessionStorage, nullptr) << "Should provide session storage offline";
    ASSERT_NE(recordingStorage, nullptr) << "Should provide recording storage offline";
    ASSERT_NE(transcriptionStorage, nullptr) << "Should provide transcription storage offline";
    
    // Test session creation offline
    QString sessionId = sessionStorage->createNewSession("Offline Test Session");
    ASSERT_FALSE(sessionId.isEmpty()) << "Should create session offline";
    
    UserSession session = sessionStorage->getUserSession(sessionId);
    EXPECT_EQ(session.name, "Offline Test Session") << "Should save session data offline";
    EXPECT_TRUE(session.isActive) << "Session should be active";
    
    // Test recording storage offline
    Recording recording;
    recording.sessionId = sessionId;
    recording.filePath = tempDir->filePath("offline_test_recording.wav");
    recording.duration = 12000;
    recording.timestamp = QDateTime::currentDateTime();
    recording.title = "Offline Recording Test";
    
    QString recordingId = recordingStorage->saveRecording(recording);
    ASSERT_FALSE(recordingId.isEmpty()) << "Should save recording offline";
    
    Recording savedRecording = recordingStorage->getRecording(recordingId);
    EXPECT_EQ(savedRecording.title, "Offline Recording Test") << "Should retrieve recording data offline";
    EXPECT_EQ(savedRecording.sessionId, sessionId) << "Should maintain relationships offline";
    
    // Test transcription storage offline
    Transcription transcription;
    transcription.recordingId = recordingId;
    transcription.text = "This is a test transcription created in offline mode.";
    transcription.confidence = 0.92;
    transcription.provider = static_cast<int>(TranscriptionProvider::WhisperCpp);
    transcription.processingTime = 5500;
    transcription.timestamp = QDateTime::currentDateTime();
    
    QString transcriptionId = transcriptionStorage->saveTranscription(transcription);
    ASSERT_FALSE(transcriptionId.isEmpty()) << "Should save transcription offline";
    
    Transcription savedTranscription = transcriptionStorage->getTranscription(transcriptionId);
    EXPECT_EQ(savedTranscription.text, transcription.text) << "Should retrieve transcription offline";
    EXPECT_EQ(savedTranscription.recordingId, recordingId) << "Should maintain transcription-recording link offline";
    
    // Test queries work offline
    auto allRecordings = recordingStorage->getAllRecordings();
    EXPECT_GT(allRecordings.size(), 0) << "Should query recordings offline";
    
    auto sessionRecordings = recordingStorage->getRecordingsBySession(sessionId);
    EXPECT_EQ(sessionRecordings.size(), 1) << "Should filter recordings by session offline";
    
    auto recordingTranscription = transcriptionStorage->getTranscriptionByRecording(recordingId);
    EXPECT_EQ(recordingTranscription.id, transcriptionId) << "Should find transcription by recording offline";
    
    // Test statistics work offline
    int recordingCount = recordingStorage->getRecordingCount();
    EXPECT_EQ(recordingCount, 1) << "Should count records offline";
    
    qint64 totalDuration = recordingStorage->getTotalRecordingDuration();
    EXPECT_EQ(totalDuration, 12000) << "Should calculate statistics offline";
    
    // Test database maintenance offline
    bool integrityOk = storageManager->checkIntegrity();
    EXPECT_TRUE(integrityOk) << "Should check database integrity offline";
    
    qint64 dbSize = storageManager->getDatabaseSize();
    EXPECT_GT(dbSize, 0) << "Should report database size offline";
    */
    
    FAIL() << "Offline storage test not implemented - needs complete storage implementation";
}

// Integration Test 4: AI Enhancement Graceful Offline Handling
TEST_F(OfflineModeIntegrationTest, AIEnhancementGracefulOfflineHandling) {
    // TDD: This test MUST FAIL initially
    
    // TODO: Uncomment when real implementations exist
    /*
    QSignalSpy networkStatusSpy(enhancementService, &ITextEnhancementService::networkStatusChanged);
    
    // Simulate offline status
    enhancementService->onNetworkStatusChanged(false);
    
    QString testText = "This text needs enhancement but we are currently offline.";
    
    // Test enhancement attempt while offline
    EnhancementRequest request;
    request.text = testText;
    request.settings.mode = EnhancementMode::StyleImprovement;
    request.preferredProvider = EnhancementProvider::GeminiPro;
    
    QString requestId = enhancementService->submitEnhancement(request);
    
    if (!requestId.isEmpty()) {
        // Should handle offline gracefully
        QTest::qWait(2000);
        
        EnhancementStatus status = enhancementService->getEnhancementStatus(requestId);
        
        if (status == EnhancementStatus::Failed) {
            EnhancementError error = enhancementService->getLastError();
            EXPECT_EQ(error, EnhancementError::NetworkError) << "Should report network error when offline";
            
            QString errorMessage = enhancementService->getErrorString();
            EXPECT_FALSE(errorMessage.isEmpty()) << "Should provide clear error message";
            EXPECT_TRUE(errorMessage.contains("network") || errorMessage.contains("offline") || 
                       errorMessage.contains("connection")) << "Error message should indicate network issue";
        } else if (status == EnhancementStatus::Pending) {
            // Should queue for later processing
            EXPECT_GT(enhancementService->getQueueLength(), 0) << "Should queue requests when offline";
        }
    }
    
    // Test provider availability reporting
    bool geminiProAvailable = enhancementService->isProviderAvailable(EnhancementProvider::GeminiPro);
    bool geminiFlashAvailable = enhancementService->isProviderAvailable(EnhancementProvider::GeminiFlash);
    
    EXPECT_FALSE(geminiProAvailable || geminiFlashAvailable) << "Network-dependent providers should be unavailable offline";
    
    // Test retry mechanism when network returns
    enhancementService->onNetworkStatusChanged(true); // Simulate network return
    
    if (networkStatusSpy.count() > 0) {
        // Should attempt to retry failed requests
        enhancementService->retryFailedEnhancements();
        
        // If we had queued requests, they should now process
        QTest::qWait(5000);
        
        if (!requestId.isEmpty()) {
            EnhancementStatus retryStatus = enhancementService->getEnhancementStatus(requestId);
            // Status might be Completed, Processing, or still Failed depending on actual connectivity
            EXPECT_TRUE(retryStatus == EnhancementStatus::Completed || 
                       retryStatus == EnhancementStatus::Processing ||
                       retryStatus == EnhancementStatus::Failed) << "Should handle retry appropriately";
        }
    }
    
    // Test offline mode indication in UI/status
    auto availableProviders = enhancementService->getAvailableProviders();
    // In offline mode, only local providers (if any) should be available
    bool hasOnlineProvider = std::any_of(availableProviders.begin(), availableProviders.end(),
        [](EnhancementProvider p) {
            return p == EnhancementProvider::GeminiPro || p == EnhancementProvider::GeminiFlash;
        });
    
    // This depends on current network status at test time
    if (!isNetworkAvailable()) {
        EXPECT_FALSE(hasOnlineProvider) << "Online providers should not be available when offline";
    }
    */
    
    FAIL() << "AI enhancement offline handling test not implemented - needs network status handling";
}

// Integration Test 5: Complete Offline Workflow Integration
TEST_F(OfflineModeIntegrationTest, CompleteOfflineWorkflowIntegration) {
    // TDD: This test MUST FAIL initially
    
    // TODO: Uncomment when real implementations exist
    /*
    // Test complete workflow: Record → Transcribe → Store (all offline)
    
    // Initialize storage
    QString dbPath = tempDir->filePath("complete_offline.sqlite");
    storageManager->initialize(dbPath);
    
    auto sessionStorage = storageManager->getUserSessionStorage();
    auto recordingStorage = storageManager->getRecordingStorage();
    auto transcriptionStorage = storageManager->getTranscriptionStorage();
    
    // Create session
    QString sessionId = sessionStorage->createNewSession("Complete Offline Workflow");
    ASSERT_FALSE(sessionId.isEmpty()) << "Should create session offline";
    
    // Step 1: Record audio offline
    QString recordingPath = tempDir->filePath("complete_workflow_recording.wav");
    
    bool recordingStarted = audioRecorder->startRecording(recordingPath);
    ASSERT_TRUE(recordingStarted) << "Should start recording offline";
    
    QTest::qWait(20000); // 20 second recording
    
    audioRecorder->stopRecording();
    ASSERT_TRUE(QFile::exists(recordingPath)) << "Recording file should be created offline";
    
    qint64 recordingDuration = audioRecorder->getRecordingDuration();
    
    // Step 2: Save recording to storage offline
    Recording recording;
    recording.sessionId = sessionId;
    recording.filePath = recordingPath;
    recording.duration = recordingDuration;
    recording.timestamp = QDateTime::currentDateTime();
    recording.title = "Complete Workflow Test";
    
    QString recordingId = recordingStorage->saveRecording(recording);
    ASSERT_FALSE(recordingId.isEmpty()) << "Should save recording offline";
    
    // Step 3: Transcribe offline using whisper.cpp
    ASSERT_TRUE(transcriptionService->isOfflineCapable()) << "Should support offline transcription";
    
    TranscriptionProvider offlineProvider = TranscriptionProvider::WhisperCppBase;
    if (!transcriptionService->isProviderAvailable(offlineProvider)) {
        offlineProvider = TranscriptionProvider::WhisperCppTiny;
    }
    
    ASSERT_TRUE(transcriptionService->isProviderAvailable(offlineProvider)) << "Offline provider should be available";
    ASSERT_TRUE(transcriptionService->isModelDownloaded(offlineProvider)) << "Offline model should be available";
    
    transcriptionService->setProvider(offlineProvider);
    
    TranscriptionRequest request;
    request.audioFilePath = recordingPath;
    request.preferredProvider = offlineProvider;
    
    QString transcriptionRequestId = transcriptionService->submitTranscription(request);
    ASSERT_FALSE(transcriptionRequestId.isEmpty()) << "Should submit transcription offline";
    
    // Wait for transcription
    int maxWait = 45000; // 45 seconds for longer audio
    int waited = 0;
    while (waited < maxWait) {
        if (transcriptionService->getTranscriptionStatus(transcriptionRequestId) == TranscriptionStatus::Completed) {
            break;
        }
        QTest::qWait(1000);
        waited += 1000;
    }
    
    EXPECT_EQ(transcriptionService->getTranscriptionStatus(transcriptionRequestId), TranscriptionStatus::Completed)
        << "Should complete transcription offline";
    
    TranscriptionResult result = transcriptionService->getTranscriptionResult(transcriptionRequestId);
    ASSERT_FALSE(result.text.isEmpty()) << "Should produce transcription text offline";
    
    // Step 4: Save transcription to storage offline
    Transcription transcription;
    transcription.recordingId = recordingId;
    transcription.text = result.text;
    transcription.confidence = result.confidence;
    transcription.provider = static_cast<int>(result.provider);
    transcription.processingTime = result.processingTime;
    transcription.timestamp = QDateTime::currentDateTime();
    
    QString transcriptionId = transcriptionStorage->saveTranscription(transcription);
    ASSERT_FALSE(transcriptionId.isEmpty()) << "Should save transcription offline";
    
    // Step 5: Verify complete workflow data integrity
    UserSession finalSession = sessionStorage->getUserSession(sessionId);
    EXPECT_EQ(finalSession.name, "Complete Offline Workflow") << "Session should be preserved";
    
    Recording finalRecording = recordingStorage->getRecording(recordingId);
    EXPECT_EQ(finalRecording.sessionId, sessionId) << "Recording should be linked to session";
    EXPECT_EQ(finalRecording.title, "Complete Workflow Test") << "Recording data should be preserved";
    
    Transcription finalTranscription = transcriptionStorage->getTranscription(transcriptionId);
    EXPECT_EQ(finalTranscription.recordingId, recordingId) << "Transcription should be linked to recording";
    EXPECT_EQ(finalTranscription.text, result.text) << "Transcription text should be preserved";
    
    // Test data relationships
    Transcription linkedTranscription = transcriptionStorage->getTranscriptionByRecording(recordingId);
    EXPECT_EQ(linkedTranscription.id, transcriptionId) << "Should find transcription by recording";
    
    auto sessionRecordings = recordingStorage->getRecordingsBySession(sessionId);
    EXPECT_EQ(sessionRecordings.size(), 1) << "Session should contain one recording";
    EXPECT_EQ(sessionRecordings.first().id, recordingId) << "Should find recording by session";
    
    // Update session statistics
    finalSession.recordingCount = 1;
    finalSession.totalDuration = recordingDuration;
    bool sessionUpdated = sessionStorage->updateUserSession(finalSession);
    EXPECT_TRUE(sessionUpdated) << "Should update session statistics offline";
    
    // Verify all operations completed without network
    SUCCEED() << "Complete offline workflow executed successfully";
    
    std::cout << "Offline Workflow Summary:" << std::endl;
    std::cout << "Recording duration: " << recordingDuration << "ms" << std::endl;
    std::cout << "Transcription length: " << result.text.length() << " characters" << std::endl;
    std::cout << "Transcription confidence: " << result.confidence << std::endl;
    std::cout << "Processing time: " << result.processingTime << "ms" << std::endl;
    */
    
    FAIL() << "Complete offline workflow test not implemented - needs all offline services integrated";
}

// Integration Test 6: Offline Mode Performance Validation
TEST_F(OfflineModeIntegrationTest, OfflineModePerformanceValidation) {
    // TDD: This test MUST FAIL initially
    
    // TODO: Uncomment when real implementations exist
    /*
    // Test that offline mode meets performance requirements
    
    // Test 1: Recording start latency (should be same offline/online)
    QString testPath = tempDir->filePath("performance_test.wav");
    
    auto startTime = std::chrono::high_resolution_clock::now();
    bool recordingStarted = audioRecorder->startRecording(testPath);
    auto endTime = std::chrono::high_resolution_clock::now();
    
    auto startLatency = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    EXPECT_TRUE(recordingStarted) << "Recording should start offline";
    EXPECT_LT(startLatency.count(), 500) << "Recording start should be < 500ms offline (same as online)";
    
    audioRecorder->cancelRecording();
    
    // Test 2: Transcription performance offline (may be slower than online services but within limits)
    // Create test audio
    audioRecorder->startRecording(testPath);
    QTest::qWait(60000); // 1 minute recording
    audioRecorder->stopRecording();
    
    TranscriptionProvider offlineProvider = TranscriptionProvider::WhisperCppTiny; // Use fastest for performance test
    transcriptionService->setProvider(offlineProvider);
    
    TranscriptionRequest request;
    request.audioFilePath = testPath;
    request.preferredProvider = offlineProvider;
    
    auto transcriptionStart = std::chrono::high_resolution_clock::now();
    QString requestId = transcriptionService->submitTranscription(request);
    
    // Wait for completion
    while (transcriptionService->getTranscriptionStatus(requestId) != TranscriptionStatus::Completed &&
           transcriptionService->getTranscriptionStatus(requestId) != TranscriptionStatus::Failed) {
        QTest::qWait(200);
    }
    
    auto transcriptionEnd = std::chrono::high_resolution_clock::now();
    auto transcriptionTime = std::chrono::duration_cast<std::chrono::milliseconds>(transcriptionEnd - transcriptionStart);
    
    EXPECT_EQ(transcriptionService->getTranscriptionStatus(requestId), TranscriptionStatus::Completed);
    
    // Allow more time for offline transcription, but should still be reasonable
    EXPECT_LT(transcriptionTime.count(), 10000) << "Offline transcription should complete within 10 seconds for 1-minute audio";
    
    // Test 3: Storage operations performance (should be fast offline)
    QString dbPath = tempDir->filePath("performance_storage.sqlite");
    
    auto storageInitStart = std::chrono::high_resolution_clock::now();
    bool storageInit = storageManager->initialize(dbPath);
    auto storageInitEnd = std::chrono::high_resolution_clock::now();
    
    auto initTime = std::chrono::duration_cast<std::chrono::milliseconds>(storageInitEnd - storageInitStart);
    
    EXPECT_TRUE(storageInit) << "Storage should initialize offline";
    EXPECT_LT(initTime.count(), 1000) << "Storage initialization should be < 1 second offline";
    
    std::cout << "Offline Performance Results:" << std::endl;
    std::cout << "Recording start latency: " << startLatency.count() << "ms" << std::endl;
    std::cout << "Transcription time (1-min audio): " << transcriptionTime.count() << "ms" << std::endl;
    std::cout << "Storage initialization: " << initTime.count() << "ms" << std::endl;
    */
    
    FAIL() << "Offline performance validation not implemented - needs performance measurements";
}

// Performance Test: Offline vs Online Performance Comparison
TEST_F(OfflineModeIntegrationTest, DISABLED_OfflineVsOnlinePerformanceComparison) {
    // TODO: Implement when services are available
    /*
    // This test would compare performance between offline and online modes
    // to ensure offline functionality is acceptably fast
    
    if (isNetworkAvailable()) {
        std::cout << "Network available - could run online vs offline comparison" << std::endl;
        // Would test both modes and compare
    } else {
        std::cout << "Network not available - testing offline mode only" << std::endl;
        // Would test offline mode performance only
    }
    */
    
    SUCCEED() << "Performance comparison test skipped - implementation needed";
}
