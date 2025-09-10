// Google Test main entry point for QuillScribe tests
// This file provides a custom main function for all test executables

#include <gtest/gtest.h>

// Qt headers (conditional compilation when Qt is available)
#if defined(QT_AVAILABLE) && defined(QT_CORE_LIB)
#include <QApplication>
#include <QTest>
#endif

int main(int argc, char** argv) {
#if defined(QT_AVAILABLE) && defined(QT_CORE_LIB)
    // Initialize Qt application for tests that need Qt components
    QApplication app(argc, argv);
#endif
    
    // Initialize Google Test
    ::testing::InitGoogleTest(&argc, argv);
    
    // Set up any global test configuration here
    // For example, test data paths, mock configurations, etc.
    
    // Run all tests
    return RUN_ALL_TESTS();
}

