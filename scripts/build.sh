#!/bin/bash

# QuillScribe Build Script
# Configures and builds the project with all dependencies
# Supports Linux/WSL, Windows (MSVC), and macOS builds

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

echo "=== QuillScribe Build Script ==="
echo "Project root: $PROJECT_ROOT"

cd "$PROJECT_ROOT"

# Detect platform
PLATFORM="unknown"
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    if grep -qi microsoft /proc/version 2>/dev/null; then
        PLATFORM="wsl"
    else
        PLATFORM="linux"
    fi
elif [[ "$OSTYPE" == "darwin"* ]]; then
    PLATFORM="macos"
elif [[ "$OSTYPE" == "msys" || "$OSTYPE" == "win32" ]]; then
    PLATFORM="windows"
fi

echo "Detected platform: $PLATFORM"

# Check if CMake is available
if ! command -v cmake >/dev/null 2>&1; then
    echo "Error: CMake is not installed or not in PATH"
    echo "Please install CMake 3.20 or later"
    exit 1
fi

CMAKE_VERSION=$(cmake --version | head -n1)
echo "Using: $CMAKE_VERSION"

# Choose build preset based on platform
PRESET="default"
BUILD_TYPE="Debug"
DEPLOY_NEEDED=false

case "$PLATFORM" in
    "windows")
        PRESET="windows-msvc"
        BUILD_TYPE="Debug"
        DEPLOY_NEEDED=true
        echo "Using Windows MSVC preset for native microphone support"
        ;;
    "macos")
        PRESET="macos"
        BUILD_TYPE="Debug"
        DEPLOY_NEEDED=true
        echo "Using macOS preset for native microphone support"
        ;;
    "wsl")
        PRESET="default"
        echo "Using default preset for WSL"
        echo "⚠️  For better audio support, consider native Windows build"
        echo "   Run this script from Windows PowerShell/Command Prompt instead"
        ;;
    "linux")
        PRESET="default"
        echo "Using default preset for Linux"
        ;;
    *)
        echo "Unknown platform, using default preset"
        ;;
esac

# Allow preset override via environment variable
if [[ -n "$QUILLSCRIBE_PRESET" ]]; then
    PRESET="$QUILLSCRIBE_PRESET"
    echo "Using preset override: $PRESET"
fi

# Configure build
echo ""
echo "Configuring build with preset: $PRESET"
if cmake --preset "$PRESET"; then
    echo "✓ Configuration successful"
else
    echo "❌ Configuration failed"
    echo ""
    echo "Common issues:"
    case "$PLATFORM" in
        "windows")
            echo "  - Qt 6.5+ not found: Install Qt for MSVC and set CMAKE_PREFIX_PATH"
            echo "    Example: cmake --preset windows-msvc -DCMAKE_PREFIX_PATH=C:\\Qt\\6.6.3\\msvc2019_64"
            echo "  - MSVC not found: Install Visual Studio with C++ tools"
            echo "  - Ninja not found: Install Ninja build system"
            ;;
        "macos")
            echo "  - Qt 6.5+ not found: Install Qt for macOS and set CMAKE_PREFIX_PATH"
            echo "    Example: cmake --preset macos -DCMAKE_PREFIX_PATH=/path/to/Qt/6.6.3/macos"
            echo "  - Xcode tools not found: Install Xcode command line tools"
            ;;
        *)
            echo "  - Qt 6.5+ not found: Install Qt and set CMAKE_PREFIX_PATH"
            echo "  - Missing dependencies: Install vcpkg or conan dependencies"
            echo "  - Compiler not found: Install a C++17 compatible compiler"
            ;;
    esac
    exit 1
fi

# Build project
echo ""
echo "Building project..."
if cmake --build --preset "$PRESET"; then
    echo "✓ Build successful"
else
    echo "❌ Build failed"
    echo ""
    echo "Check the build output above for specific errors"
    exit 1
fi

# Deploy Qt libraries if needed (Windows/macOS)
if [[ "$DEPLOY_NEEDED" == "true" ]]; then
    echo ""
    echo "Deploying Qt libraries..."
    
    case "$PLATFORM" in
        "windows")
            BUILD_DIR="build/windows-msvc"
            EXE_PATH="$BUILD_DIR/src/Debug/quillscribe.exe"
            
            if [[ -f "$EXE_PATH" ]]; then
                # Try to find windeployqt
                WINDEPLOYQT=""
                if command -v windeployqt >/dev/null 2>&1; then
                    WINDEPLOYQT="windeployqt"
                elif [[ -n "$CMAKE_PREFIX_PATH" && -f "$CMAKE_PREFIX_PATH/bin/windeployqt.exe" ]]; then
                    WINDEPLOYQT="$CMAKE_PREFIX_PATH/bin/windeployqt.exe"
                fi
                
                if [[ -n "$WINDEPLOYQT" ]]; then
                    echo "Running windeployqt..."
                    "$WINDEPLOYQT" "$EXE_PATH" --multimedia --network --sql
                    echo "✓ Qt libraries deployed"
                else
                    echo "⚠️  windeployqt not found - you may need to copy Qt DLLs manually"
                    echo "   Add Qt bin directory to PATH or run windeployqt manually"
                fi
            else
                echo "⚠️  Executable not found at $EXE_PATH"
            fi
            ;;
        "macos")
            BUILD_DIR="build/macos"
            APP_PATH="$BUILD_DIR/src/Debug/quillscribe.app"
            
            if [[ -d "$APP_PATH" ]]; then
                # Try to find macdeployqt
                MACDEPLOYQT=""
                if command -v macdeployqt >/dev/null 2>&1; then
                    MACDEPLOYQT="macdeployqt"
                elif [[ -n "$CMAKE_PREFIX_PATH" && -f "$CMAKE_PREFIX_PATH/bin/macdeployqt" ]]; then
                    MACDEPLOYQT="$CMAKE_PREFIX_PATH/bin/macdeployqt"
                fi
                
                if [[ -n "$MACDEPLOYQT" ]]; then
                    echo "Running macdeployqt..."
                    "$MACDEPLOYQT" "$APP_PATH"
                    echo "✓ Qt libraries deployed"
                else
                    echo "⚠️  macdeployqt not found - you may need to copy Qt frameworks manually"
                fi
            else
                echo "⚠️  App bundle not found at $APP_PATH"
            fi
            ;;
    esac
fi

# Run tests (if any exist)
echo ""
echo "Running tests..."
TEST_EXIT_CODE=0

# Determine test executable path based on preset
case "$PRESET" in
    "windows-msvc")
        TEST_EXE="build/windows-msvc/tests/Debug/contract_tests.exe"
        ;;
    "macos")
        TEST_EXE="build/macos/tests/Debug/contract_tests"
        ;;
    *)
        TEST_EXE="build/default/tests/contract_tests"
        ;;
esac

if [[ -f "$TEST_EXE" ]]; then
    if "$TEST_EXE"; then
        echo "✓ All tests passed"
    else
        TEST_EXIT_CODE=$?
        echo "❌ Some tests failed (exit code: $TEST_EXIT_CODE)"
    fi
else
    echo "No tests found to run (looked for: $TEST_EXE)"
fi

# Determine build directory for final messages
case "$PRESET" in
    "windows-msvc")
        BUILD_DIR="build/windows-msvc"
        EXE_PATH="$BUILD_DIR/src/Debug/quillscribe.exe"
        ;;
    "macos")
        BUILD_DIR="build/macos"
        EXE_PATH="$BUILD_DIR/src/Debug/quillscribe.app"
        ;;
    *)
        BUILD_DIR="build/default"
        EXE_PATH="$BUILD_DIR/src/quillscribe"
        ;;
esac

echo ""
if [ $TEST_EXIT_CODE -eq 0 ]; then
    echo "=== Build Complete ==="
    echo "Build artifacts are in: $BUILD_DIR/"
    echo ""
    
    case "$PLATFORM" in
        "windows")
            echo "Windows native build complete!"
            echo "  Executable: $EXE_PATH"
            echo "  Run: $EXE_PATH"
            if [[ "$DEPLOY_NEEDED" == "true" ]]; then
                echo "  ✓ Qt libraries deployed - ready to run"
            fi
            ;;
        "macos")
            echo "macOS native build complete!"
            echo "  App bundle: $EXE_PATH"
            echo "  Run: open $EXE_PATH"
            if [[ "$DEPLOY_NEEDED" == "true" ]]; then
                echo "  ✓ Qt frameworks deployed - ready to run"
            fi
            ;;
        "wsl")
            echo "WSL build complete!"
            echo "  Executable: $EXE_PATH"
            echo "  For audio support, run: ./scripts/setup-wsl-audio.sh"
            echo "  Then test with: QT_DEBUG_PLUGINS=1 $EXE_PATH"
            ;;
        *)
            echo "Linux build complete!"
            echo "  Executable: $EXE_PATH"
            echo "  Run: $EXE_PATH"
            ;;
    esac
    
    echo ""
    echo "Next steps:"
    echo "  1. Setup whisper models: ./scripts/setup-whisper-models.sh"
    echo "  2. Start implementing tests (T007-T015)"
    echo "  3. Follow TDD workflow from tasks.md"
else
    echo "=== Build Complete with Test Failures ==="
    echo "Build artifacts are in: $BUILD_DIR/"
    echo ""
    echo "❌ Tests failed - please fix failing tests before proceeding"
    echo "Check the test output above for details"
    exit $TEST_EXIT_CODE
fi

