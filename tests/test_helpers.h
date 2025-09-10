#pragma once

// Test helpers and utilities for QuillScribe tests
// This header provides common testing utilities across all test types

#include <chrono>
#include <string>

#include <gtest/gtest.h>

// Qt headers (conditional compilation when Qt is available)
#if defined(QT_AVAILABLE) && defined(QT_CORE_LIB)
#include <QTest>
#include <QString>
#include <QTemporaryDir>
#include <QStandardPaths>
#endif

namespace QuillScribe::Testing {

/**
 * @brief Base test fixture for QuillScribe tests
 * Provides common setup and teardown functionality
 */
class QuillScribeTestBase : public ::testing::Test {
protected:
    void SetUp() override {
#if defined(QT_AVAILABLE) && defined(QT_CORE_LIB)
        // Create temporary directory for test files
        m_tempDir = std::make_unique<QTemporaryDir>();
        ASSERT_TRUE(m_tempDir->isValid());
#endif
    }
    
    void TearDown() override {
#if defined(QT_AVAILABLE) && defined(QT_CORE_LIB)
        // Cleanup is automatic with QTemporaryDir
        m_tempDir.reset();
#endif
    }
    
#if defined(QT_AVAILABLE) && defined(QT_CORE_LIB)
    [[nodiscard]] QString getTempPath() const {
        return m_tempDir->path();
    }
    
    [[nodiscard]] QString getTestDataPath(const QString& filename) const {
        return getTempPath() + "/" + filename;
    }

private:
    std::unique_ptr<QTemporaryDir> m_tempDir;
#else
    [[nodiscard]] static std::string getTempPath() {
        return "/tmp/quillscribe_test";
    }
    
    [[nodiscard]] static std::string getTestDataPath(const std::string& filename) {
        return getTempPath() + "/" + filename;
    }
#endif
};

/**
 * @brief Performance test helper for timing operations
 */
class PerformanceTimer {
public:
    PerformanceTimer() : m_startTime(std::chrono::high_resolution_clock::now()) {}
    
    [[nodiscard]] std::chrono::milliseconds elapsed() const {
        auto now = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(now - m_startTime);
    }
    
    void assertElapsedLessThan(std::chrono::milliseconds maxTime, const std::string& operation) const {
        auto actualTime = elapsed();
        ASSERT_LT(actualTime, maxTime) 
            << operation << " took " << actualTime.count() << "ms, expected < " << maxTime.count() << "ms";
    }

private:
    std::chrono::high_resolution_clock::time_point m_startTime;
};

/**
 * @brief Mock data generators for testing
 */
class TestDataGenerator {
public:
#if defined(QT_AVAILABLE) && defined(QT_CORE_LIB)
    // Generate test audio file path (placeholder - actual audio generation in integration tests)
    static QString generateTestAudioPath(const QString& basePath) {
        return basePath + "/test_audio.wav";
    }
    
    // Generate test text for enhancement testing
    static QString generateTestText(int wordCount = 50) {
        QString text = "This is a test transcription with various words. ";
        QString result;
        for (int i = 0; i < wordCount / 10; ++i) {
            result += text;
        }
        return result.trimmed();
    }
    
    // Generate test configuration
    static QString generateTestConfigPath(const QString& basePath) {
        return basePath + "/test_config.json";
    }
#else
    // Generate test audio file path (fallback for non-Qt builds)
    static std::string generateTestAudioPath(const std::string& basePath) {
        return basePath + "/test_audio.wav";
    }
    
    // Generate test text for enhancement testing
    static std::string generateTestText(int wordCount = 50) {
        const std::string TEXT = "This is a test transcription with various words. ";
        std::string result;
        for (int i = 0; i < wordCount / 10; ++i) {
            result += TEXT;
        }
        return result;
    }
    
    // Generate test configuration
    static std::string generateTestConfigPath(const std::string& basePath) {
        return basePath + "/test_config.json";
    }
#endif
};

} // namespace QuillScribe::Testing

