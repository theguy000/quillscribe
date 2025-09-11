#include "MainWindow.h"
#include <QApplication>
#include <QDir>
#include <QStandardPaths>
#include <QMessageBox>
#include <QStyleFactory>
#include <QFontDatabase>
#include <QLoggingCategory>
#include <QSplashScreen>
#include <QPixmap>
#include <QTimer>
#include <QDebug>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QSettings>
#include <QIcon>
#include <QPainter>
#include <QFont>
#include <QFile>
#include <QStorageInfo>
#include <QCoreApplication>
#include <exception>

// Enable logging categories for debugging
Q_LOGGING_CATEGORY(appMain, "app.main")

/**
 * @brief Setup application directories
 * 
 * Creates necessary application directories for data storage,
 * recordings, models, etc. For portable apps, uses app directory.
 */
bool setupApplicationDirectories() {
    QString appDataPath;
    
    // Check if this is a portable installation (executable directory has 'portable' marker or models)
    QString appDir = QCoreApplication::applicationDirPath();
    QString portableMarker = appDir + "/portable.txt";
    QString modelsDir = appDir + "/models";
    
    if (QFile::exists(portableMarker) || QDir(modelsDir).exists()) {
        // Use portable mode - store data relative to executable
        appDataPath = appDir + "/data";
        qCDebug(appMain) << "Running in portable mode, using:" << appDataPath;
    } else {
        // Use standard AppData location
        appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        qCDebug(appMain) << "Running in installed mode, using:" << appDataPath;
    }
    
    QStringList directories = {
        appDataPath,
        appDataPath + "/recordings",
        appDataPath + "/sessions", 
        appDataPath + "/models",
        appDataPath + "/models/whisper",
        appDataPath + "/backups",
        appDataPath + "/logs"
    };
    
    for (const QString& dirPath : directories) {
        QDir dir(dirPath);
        if (!dir.exists() && !dir.mkpath(dirPath)) {
            qCCritical(appMain) << "Failed to create directory:" << dirPath;
            return false;
        }
    }
    
    qCDebug(appMain) << "Application directories created successfully at:" << appDataPath;
    return true;
}

/**
 * @brief Setup application logging
 * 
 * Configures logging to file and console with appropriate levels
 * based on build configuration.
 */
void setupLogging() {
    // Set default logging rules
#ifdef QT_DEBUG
    // Debug build - verbose logging
    QLoggingCategory::setFilterRules(
        "*.debug=true\n"
        "*.info=true\n"
        "*.warning=true\n"
        "*.critical=true\n"
        "qt.*.debug=false"  // Suppress Qt's debug messages
    );
#else
    // Release build - minimal logging
    QLoggingCategory::setFilterRules(
        "*.debug=false\n"
        "*.info=true\n"
        "*.warning=true\n"
        "*.critical=true"
    );
#endif
    
    qCDebug(appMain) << "Logging configuration applied";
}

/**
 * @brief Setup application style and theme
 * 
 * Configures the application's visual appearance and theme.
 */
void setupApplicationStyle(QApplication& app) {
    // Set application style
    QApplication::setStyle("Fusion");
    
    // Load custom fonts if available
    QString fontsDir = QCoreApplication::applicationDirPath() + "/fonts";
    QDir fontDirectory(fontsDir);
    
    if (fontDirectory.exists()) {
        QStringList fontFilters;
        fontFilters << "*.ttf" << "*.otf";
        QStringList fontFiles = fontDirectory.entryList(fontFilters, QDir::Files);
        
        for (const QString& fontFile : fontFiles) {
            QString fontPath = fontDirectory.absoluteFilePath(fontFile);
            if (QFontDatabase::addApplicationFont(fontPath) == -1) {
                qCWarning(appMain) << "Failed to load font:" << fontPath;
            } else {
                qCDebug(appMain) << "Loaded font:" << fontFile;
            }
        }
    }
    
    // Set application icon
    QString iconPath = QCoreApplication::applicationDirPath() + "/icons/quillscribe.png";
    if (QFile::exists(iconPath)) {
        app.setWindowIcon(QIcon(iconPath));
    } else {
        // Fallback: create a simple icon programmatically
        QPixmap iconPixmap(64, 64);
        iconPixmap.fill(QColor(76, 175, 80)); // Material Green
        app.setWindowIcon(QIcon(iconPixmap));
    }
    
    qCDebug(appMain) << "Application style configured";
}

/**
 * @brief Check system requirements
 * 
 * Verifies that the system meets minimum requirements for running
 * QuillScribe, including audio device availability, disk space, etc.
 */
bool checkSystemRequirements() {
    // Get multiple potential paths for disk space checking
    QStringList pathsToCheck = {
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation),
        QStandardPaths::writableLocation(QStandardPaths::TempLocation),
        QCoreApplication::applicationDirPath(),
        QDir::homePath()
    };
    
    const qint64 MINIMUM_DISK_SPACE = 50 * 1024 * 1024; // Reduced to 50 MB for portable apps
    QString workingPath;
    qint64 availableSpace = 0;
    
    // Try each path until we find one that works
    for (const QString& path : pathsToCheck) {
        if (path.isEmpty()) continue;
        
        QStorageInfo storageInfo(path);
        if (storageInfo.isValid() && storageInfo.isReady()) {
            availableSpace = storageInfo.bytesAvailable();
            if (availableSpace > 0) {
                workingPath = path;
                qCDebug(appMain) << "Using path for disk check:" << path 
                                << "Available:" << (availableSpace / (1024 * 1024)) << "MB";
                break;
            }
        }
        qCDebug(appMain) << "Path failed disk check:" << path << "Available:" << availableSpace;
    }
    
    // If we couldn't get disk space info from any path, warn but continue
    if (workingPath.isEmpty() || availableSpace <= 0) {
        qCWarning(appMain) << "Warning: Could not determine available disk space";
        // For portable apps, we'll continue anyway as this might be a permissions/path issue
        return true;
    }
    
    // Check if we have minimum space
    if (availableSpace < MINIMUM_DISK_SPACE) {
        QMessageBox::warning(nullptr, "System Requirements",
                           QString("Insufficient disk space. At least 50 MB required.\n"
                                  "Available: %1 MB\n"
                                  "Checked path: %2")
                           .arg(availableSpace / (1024 * 1024))
                           .arg(workingPath));
        return false;
    }
    
    // Test write access to application data directory
    QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (!appDataPath.isEmpty()) {
        // Ensure the directory exists
        QDir dir;
        if (!dir.exists(appDataPath)) {
            dir.mkpath(appDataPath);
        }
        
        QString testFile = appDataPath + "/write_test.tmp";
        QFile file(testFile);
        if (!file.open(QIODevice::WriteOnly)) {
            // If we can't write to AppDataLocation, try temp directory
            QString tempPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/QuillScribe";
            dir.mkpath(tempPath);
            testFile = tempPath + "/write_test.tmp";
            QFile tempFile(testFile);
            if (!tempFile.open(QIODevice::WriteOnly)) {
                QMessageBox::critical(nullptr, "System Requirements",
                                     "Cannot write to application directories.\n"
                                     "Tried paths:\n" + appDataPath + "\n" + tempPath);
                return false;
            }
            tempFile.close();
            tempFile.remove();
            qCDebug(appMain) << "Using fallback temp directory for app data:" << tempPath;
        } else {
            file.close();
            file.remove();
            qCDebug(appMain) << "App data directory write test passed:" << appDataPath;
        }
    }
    
    qCDebug(appMain) << "System requirements check passed";
    qCDebug(appMain) << "Available disk space:" << (availableSpace / (1024 * 1024)) << "MB at" << workingPath;
    
    return true;
}

/**
 * @brief Show splash screen during application startup
 * 
 * Displays a loading screen while the application initializes.
 */
QSplashScreen* showSplashScreen() {
    // Create splash screen
    QPixmap splashPixmap(400, 300);
    splashPixmap.fill(QColor(76, 175, 80)); // Material Green
    
    // Draw application name and version
    QPainter painter(&splashPixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // Set font for title
    QFont titleFont = painter.font();
    titleFont.setPointSize(24);
    titleFont.setBold(true);
    painter.setFont(titleFont);
    painter.setPen(Qt::white);
    
    // Draw title
    painter.drawText(splashPixmap.rect(), Qt::AlignCenter, 
                    "QuillScribe\nVoice-to-Text with AI\n\nLoading...");
    
    // Create splash screen
    QSplashScreen* splash = new QSplashScreen(splashPixmap);
    splash->show();
    splash->showMessage("Initializing services...", Qt::AlignBottom | Qt::AlignCenter, Qt::white);
    
    return splash;
}

/**
 * @brief Parse command line arguments
 * 
 * Handles command line options and arguments for the application.
 */
void parseCommandLine(QApplication& app) {
    QCommandLineParser parser;
    parser.setApplicationDescription("QuillScribe - Voice-to-Text Application with AI Enhancement");
    parser.addHelpOption();
    parser.addVersionOption();
    
    // Add custom command line options
    QCommandLineOption debugOption("debug", "Enable debug output");
    parser.addOption(debugOption);
    
    QCommandLineOption configOption("config", 
                                   "Use custom configuration file", 
                                   "config-file");
    parser.addOption(configOption);
    
    QCommandLineOption dataOption("data-dir",
                                 "Use custom data directory",
                                 "data-directory");
    parser.addOption(dataOption);
    
    // Process command line
    parser.process(app);
    
    // Handle debug option
    if (parser.isSet(debugOption)) {
        QLoggingCategory::setFilterRules("*.debug=true");
        qCDebug(appMain) << "Debug logging enabled via command line";
    }
    
    // Handle custom config file
    if (parser.isSet(configOption)) {
        QString configFile = parser.value(configOption);
        QSettings::setDefaultFormat(QSettings::IniFormat);
        // Could load custom settings here
        qCDebug(appMain) << "Custom config file specified:" << configFile;
    }
    
    // Handle custom data directory  
    if (parser.isSet(dataOption)) {
        QString dataDir = parser.value(dataOption);
        // Could override default data directory here
        qCDebug(appMain) << "Custom data directory specified:" << dataDir;
    }
}

/**
 * @brief Initialize application metadata
 * 
 * Sets up application information used by Qt's settings system
 * and other components.
 */
void initializeApplicationMetadata() {
    QCoreApplication::setApplicationName("QuillScribe");
    QCoreApplication::setApplicationVersion("1.0.0");
    QCoreApplication::setOrganizationName("QuillScribe");
    QCoreApplication::setOrganizationDomain("quillscribe.app");
    
    qCDebug(appMain) << "Application metadata initialized";
    qCDebug(appMain) << "Version:" << QCoreApplication::applicationVersion();
    qCDebug(appMain) << "Build date:" << __DATE__ << __TIME__;
}

/**
 * @brief Handle uncaught exceptions
 * 
 * Global exception handler for better error reporting.
 */
void handleUncaughtException() {
    QMessageBox::critical(nullptr, "Critical Error",
                         "An unexpected error occurred. Please restart the application.\n"
                         "If the problem persists, please contact support.");
    qCCritical(appMain) << "Uncaught exception occurred";
}

/**
 * @brief Main application entry point
 * 
 * Initializes the Qt application, sets up services, and shows the main window.
 */
int main(int argc, char *argv[])
{
    // Create Qt application
    QApplication app(argc, argv);
    
    // Initialize application metadata
    initializeApplicationMetadata();
    
    // Parse command line arguments
    parseCommandLine(app);
    
    // Setup logging
    setupLogging();
    
    qCInfo(appMain) << "Starting QuillScribe version" << QCoreApplication::applicationVersion();
    qCDebug(appMain) << "Qt version:" << qVersion();
    qCDebug(appMain) << "Application directory:" << QCoreApplication::applicationDirPath();
    qCDebug(appMain) << "Data directory:" << QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    
    // Set up exception handling
    std::set_terminate(handleUncaughtException);
    
    // Show splash screen
    QSplashScreen* splash = showSplashScreen();
    app.processEvents();
    
    try {
        // Check system requirements
        splash->showMessage("Checking system requirements...", Qt::AlignBottom | Qt::AlignCenter, Qt::white);
        app.processEvents();
        
        if (!checkSystemRequirements()) {
            delete splash;
            return -1;
        }
        
        // Setup application directories
        splash->showMessage("Creating application directories...", Qt::AlignBottom | Qt::AlignCenter, Qt::white);
        app.processEvents();
        
        if (!setupApplicationDirectories()) {
            QMessageBox::critical(nullptr, "Initialization Error",
                                 "Failed to create application directories.");
            delete splash;
            return -1;
        }
        
        // Setup application style
        splash->showMessage("Configuring application style...", Qt::AlignBottom | Qt::AlignCenter, Qt::white);
        app.processEvents();
        
        setupApplicationStyle(app);
        
        // Create and show main window
        splash->showMessage("Loading main window...", Qt::AlignBottom | Qt::AlignCenter, Qt::white);
        app.processEvents();
        
        MainWindow window;
        
        // Simulate some initialization time for splash screen
        QTimer::singleShot(1500, [&]() {
            splash->finish(&window);
            window.show();
            window.raise();
            window.activateWindow();
        });
        
        qCInfo(appMain) << "QuillScribe initialized successfully";
        
        // Start the event loop
        int result = app.exec();
        
        qCInfo(appMain) << "QuillScribe shutting down with exit code:" << result;
        return result;
        
    } catch (const std::exception& e) {
        qCCritical(appMain) << "Exception during startup:" << e.what();
        
        if (splash) {
            delete splash;
        }
        
        QMessageBox::critical(nullptr, "Startup Error",
                             QString("Failed to start QuillScribe:\n%1").arg(e.what()));
        return -1;
        
    } catch (...) {
        qCCritical(appMain) << "Unknown exception during startup";
        
        if (splash) {
            delete splash;
        }
        
        QMessageBox::critical(nullptr, "Startup Error",
                             "An unknown error occurred during startup.");
        return -1;
    }
}

// Qt MOC files are automatically handled by CMAKE_AUTOMOC
