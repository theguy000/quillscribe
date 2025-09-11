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
!define VERSION "1.0.0"

SetCompressor /SOLID lzma
RequestExecutionLevel user

; Try user profile first, fallback to temp if needed
InstallDir "$PROFILE\QuillScribe_Portable"
InstallDirRegKey HKCU "Software\${APPNAME}\Portable" "InstallDir"

SilentInstall silent
AutoCloseWindow true
ShowInstDetails nevershow
Name "${APPNAME} Portable"
OutFile "QuillScribe-Portable.exe"

Section "MainSection" SEC01
    ; Check if we can write to user profile, fallback to temp
    SetOutPath "$INSTDIR"
    ClearErrors
    FileOpen $0 "$INSTDIR\test_write.tmp" w
    IfErrors 0 WriteOK
        ; Fallback to temp directory with better naming
        StrCpy $INSTDIR "$TEMP\QuillScribe_Portable"
        SetOutPath "$INSTDIR"
        Goto ExtractFiles
    WriteOK:
        FileClose $0
        Delete "$INSTDIR\test_write.tmp"
    
    ExtractFiles:
    ; Extract all files
    File /r "build\windows-msvc-release\src\Release\*.*"
    
    ; Create portable marker file
    FileOpen $0 "$INSTDIR\portable.txt" w
    FileWrite $0 "QuillScribe Portable Mode$\r$\n"
    FileWrite $0 "This file indicates the application should run in portable mode.$\r$\n"
    FileWrite $0 "Data will be stored relative to the executable directory.$\r$\n"
    FileClose $0
    
    ; Set working directory and execute
    SetOutPath "$INSTDIR"
    ExecWait '$INSTDIR\quillscribe.exe' $0
    
    ; Only remove temp directory if we used temp
    StrCmp $INSTDIR "$TEMP\QuillScribe_Portable" 0 SkipCleanup
        RMDir /r "$INSTDIR"
    SkipCleanup:
    
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
