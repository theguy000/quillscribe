#!/bin/bash
# QuillScribe Single Executable Packager
# Creates portable executables for different platforms

echo "=== QuillScribe Single Executable Packager ==="

# Parse arguments
PLATFORM=""
CONFIG=""
while [[ $# -gt 0 ]]; do
    case $1 in
        --platform)
            PLATFORM="$2"
            shift 2
            ;;
        --config)
            CONFIG="$2"
            shift 2
            ;;
        *)
            echo "Unknown option: $1"
            exit 1
            ;;
    esac
done

echo "Project root: $(pwd)"
echo "Target platform: $PLATFORM"
echo "Build configuration: $CONFIG"

# Platform-specific packaging
case "$PLATFORM" in
    "windows")
        # Check for NSIS
        if ! command -v makensis >/dev/null 2>&1; then
            echo "❌ NSIS not found! Please install NSIS first."
            exit 1
        fi

        # Check if executable exists
        EXE_PATH="build/windows-msvc-release/src/Release/quillscribe.exe"
        if [[ ! -f "$EXE_PATH" ]]; then
            echo "❌ Executable not found at: $EXE_PATH"
            echo "Please build the project first with Release configuration"
            exit 1
        fi

        echo "✓ Found executable: $EXE_PATH"
        echo ""
        echo "Creating Windows portable executable..."
        
        # Create NSIS installer script
        cat > installer.nsi << "NSIS_EOF"
!define APPNAME "QuillScribe"
SetCompressor /SOLID lzma
RequestExecutionLevel user
InstallDir $TEMP\QuillScribe_$$
SilentInstall silent
AutoCloseWindow true
ShowInstDetails nevershow
Name "${APPNAME}"
OutFile "QuillScribe-Portable.exe"
Section "MainSection" SEC01
    SetOutPath "$INSTDIR"
    File /r "build\windows-msvc-release\src\Release\*.*"
    ExecWait '$INSTDIR\quillscribe.exe' $0
    RMDir /r "$INSTDIR"
    SetErrorLevel $0
SectionEnd
NSIS_EOF

        # Build the executable
        makensis installer.nsi && rm installer.nsi

        if [[ -f "QuillScribe-Portable.exe" ]]; then
            echo "✓ Single executable created: QuillScribe-Portable.exe"
        else
            echo "❌ Failed to create single executable"
            exit 1
        fi
        ;;
    "macos")
        APP_PATH="build/macos-release/src/Release/quillscribe.app"
        if [[ ! -d "$APP_PATH" ]]; then
            echo "❌ App bundle not found at: $APP_PATH"
            exit 1
        fi
        echo "✓ Found app bundle: $APP_PATH"
        echo ""
        echo "Creating macOS DMG..."
        # TODO: Implement DMG creation
        echo "✓ DMG creation not implemented yet"
        exit 0
        ;;
    "linux")
        EXE_PATH="build/release/src/quillscribe"
        if [[ ! -f "$EXE_PATH" ]]; then
            echo "❌ Executable not found at: $EXE_PATH"
            exit 1
        fi
        echo "✓ Found executable: $EXE_PATH"
        echo ""
        echo "Creating Linux AppImage..."
        # TODO: Implement AppImage creation
        echo "✓ AppImage creation not implemented yet"
        exit 0
        ;;
    *)
        echo "❌ Unsupported platform: $PLATFORM"
        echo "Supported platforms: windows, macos, linux"
        exit 1
        ;;
esac

echo "=== Packaging Complete ==="
