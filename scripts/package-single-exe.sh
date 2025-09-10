#!/bin/bash

# QuillScribe Single Executable Packager
# Cross-platform script for creating portable executables
# Supports Windows (NSIS), macOS (DMG), and Linux (AppImage)

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

# Default values
PLATFORM=""
CONFIG="Release"
HELP=false

# Parse command line arguments
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
        --help|-h)
            HELP=true
            shift
            ;;
        *)
            echo "Unknown option: $1"
            exit 1
            ;;
    esac
done

if [[ "$HELP" == "true" ]]; then
    echo "QuillScribe Single Executable Packager"
    echo ""
    echo "Usage: ./scripts/package-single-exe.sh [OPTIONS]"
    echo ""
    echo "Options:"
    echo "  --platform <name>  Target platform: windows, macos, linux, or auto-detect"
    echo "  --config <config>  Build configuration: Debug or Release (default: Release)"
    echo "  --help, -h         Show this help message"
    echo ""
    echo "Platform-specific outputs:"
    echo "  Windows: QuillScribe-Portable.exe (NSIS-based)"
    echo "  macOS:   QuillScribe.dmg (disk image)"
    echo "  Linux:   QuillScribe-x86_64.AppImage"
    echo ""
    echo "Requirements:"
    echo "  Windows: NSIS (makensis.exe in PATH)"
    echo "  macOS:   hdiutil (built-in)"
    echo "  Linux:   linuxdeploy, linuxdeploy-plugin-qt"
    exit 0
fi

echo "=== QuillScribe Single Executable Packager ==="
echo "Project root: $PROJECT_ROOT"

cd "$PROJECT_ROOT"

# Auto-detect platform if not specified
if [[ -z "$PLATFORM" ]]; then
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
    else
        echo "❌ Unable to detect platform. Please specify with --platform"
        exit 1
    fi
fi

echo "Target platform: $PLATFORM"
echo "Build configuration: $CONFIG"

# Determine build directory and executable path based on platform
case "$PLATFORM" in
    "windows"|"wsl")
        BUILD_DIR="build/windows-msvc-release"
        EXE_PATH="$BUILD_DIR/src/Release/quillscribe.exe"
        PRESET="windows-msvc-release"
        ;;
    "macos")
        BUILD_DIR="build/macos-release"
        EXE_PATH="$BUILD_DIR/src/Release/quillscribe.app"
        PRESET="macos-release"
        ;;
    "linux")
        BUILD_DIR="build/release"
        EXE_PATH="$BUILD_DIR/src/quillscribe"
        PRESET="release"
        ;;
    *)
        echo "❌ Unsupported platform: $PLATFORM"
        exit 1
        ;;
esac

# Check if the executable exists
if [[ ! -e "$EXE_PATH" ]]; then
    echo "❌ Executable not found at: $EXE_PATH"
    echo "Please build the project first:"
    echo "  ./scripts/build.sh"
    echo "Or for Windows: ./scripts/build-windows.ps1 -Config Release -Deploy"
    exit 1
fi

echo "✓ Found executable: $EXE_PATH"

# Windows packaging function
package_windows() {
    echo ""
    echo "Creating Windows portable executable..."
    
    # Check for NSIS
    MAKENSIS=""
    if command -v makensis >/dev/null 2>&1; then
        MAKENSIS="makensis"
    elif [[ -f "/c/Program Files (x86)/NSIS/makensis.exe" ]]; then
        MAKENSIS="/c/Program Files (x86)/NSIS/makensis.exe"
    elif [[ -f "C:\\Program Files (x86)\\NSIS\\makensis.exe" ]]; then
        MAKENSIS="C:\\Program Files (x86)\\NSIS\\makensis.exe"
    fi
    
    if [[ -z "$MAKENSIS" ]]; then
        echo "❌ NSIS not found!"
        echo "Please install NSIS from: https://nsis.sourceforge.io/Download"
        echo "Or use Chocolatey: choco install nsis"
        exit 1
    fi
    
    echo "✓ Found NSIS: $MAKENSIS"
    
    # Create NSIS script
    cat > installer.nsi << 'EOF'
!define APPNAME "QuillScribe"
!define COMPANYNAME "QuillScribe"
!define DESCRIPTION "Voice-to-Text Application with AI Enhancement"
!define VERSIONMAJOR 1
!define VERSIONMINOR 0
!define VERSIONBUILD 0

; Compression
SetCompressor /SOLID lzma
SetCompressorDictSize 64

; Installer settings
RequestExecutionLevel user
InstallDir $TEMP\QuillScribe_$$
SilentInstall silent
AutoCloseWindow true
ShowInstDetails nevershow

; Metadata
Name "${APPNAME}"
OutFile "QuillScribe-Portable.exe"
VIProductVersion "${VERSIONMAJOR}.${VERSIONMINOR}.${VERSIONBUILD}.0"
VIAddVersionKey "ProductName" "${APPNAME}"
VIAddVersionKey "CompanyName" "${COMPANYNAME}"
VIAddVersionKey "LegalCopyright" "© ${COMPANYNAME}"
VIAddVersionKey "FileDescription" "${DESCRIPTION}"
VIAddVersionKey "FileVersion" "${VERSIONMAJOR}.${VERSIONMINOR}.${VERSIONBUILD}.0"
VIAddVersionKey "ProductVersion" "${VERSIONMAJOR}.${VERSIONMINOR}.${VERSIONBUILD}.0"

Section "MainSection" SEC01
    SetOutPath "$INSTDIR"
    
    ; Copy all application files
    File /r "build\windows-msvc-release\src\Release\*.*"
    
    ; Copy whisper models if they exist
    IfFileExists "models\whisper\*.*" 0 +3
        CreateDirectory "$INSTDIR\models\whisper"
        File /r "models\whisper\*.*"
    
    ; Run the application and wait for it to finish
    ExecWait '"$INSTDIR\quillscribe.exe"' $0
    
    ; Cleanup - remove all extracted files
    RMDir /r "$INSTDIR"
    
    ; Exit with the same code as the application
    SetErrorLevel $0
SectionEnd
EOF
    
    # Build the executable
    if "$MAKENSIS" installer.nsi; then
        if [[ -f "QuillScribe-Portable.exe" ]]; then
            FILE_SIZE=$(stat -c%s "QuillScribe-Portable.exe" 2>/dev/null || stat -f%z "QuillScribe-Portable.exe" 2>/dev/null || echo "unknown")
            FILE_SIZE_MB=$((FILE_SIZE / 1024 / 1024))
            
            echo "✓ Windows portable executable created!"
            echo "File: QuillScribe-Portable.exe"
            echo "Size: ${FILE_SIZE_MB} MB"
        else
            echo "❌ Failed to create portable executable"
            exit 1
        fi
    else
        echo "❌ NSIS compilation failed"
        exit 1
    fi
    
    # Cleanup
    rm -f installer.nsi
}

# macOS packaging function  
package_macos() {
    echo ""
    echo "Creating macOS disk image..."
    
    if [[ ! -d "$EXE_PATH" ]]; then
        echo "❌ App bundle not found: $EXE_PATH"
        exit 1
    fi
    
    # Create DMG
    DMG_NAME="QuillScribe-macOS.dmg"
    rm -f "$DMG_NAME"
    
    # Create temporary directory for DMG contents
    DMG_DIR="dmg_temp"
    rm -rf "$DMG_DIR"
    mkdir -p "$DMG_DIR"
    
    # Copy app bundle
    cp -R "$EXE_PATH" "$DMG_DIR/"
    
    # Create symbolic link to Applications folder
    ln -s /Applications "$DMG_DIR/Applications"
    
    # Create DMG
    if hdiutil create -volname "QuillScribe" -srcfolder "$DMG_DIR" -ov -format UDZO "$DMG_NAME"; then
        FILE_SIZE=$(stat -f%z "$DMG_NAME" 2>/dev/null || echo "unknown")
        FILE_SIZE_MB=$((FILE_SIZE / 1024 / 1024))
        
        echo "✓ macOS disk image created!"
        echo "File: $DMG_NAME"
        echo "Size: ${FILE_SIZE_MB} MB"
    else
        echo "❌ Failed to create DMG"
        exit 1
    fi
    
    # Cleanup
    rm -rf "$DMG_DIR"
}

# Linux packaging function
package_linux() {
    echo ""
    echo "Creating Linux AppImage..."
    
    # Check for linuxdeploy
    LINUXDEPLOY="./linuxdeploy-x86_64.AppImage"
    LINUXDEPLOY_QT="./linuxdeploy-plugin-qt-x86_64.AppImage"
    
    if [[ ! -f "$LINUXDEPLOY" ]]; then
        echo "Downloading linuxdeploy..."
        wget -q "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage"
        chmod +x linuxdeploy-x86_64.AppImage
    fi
    
    if [[ ! -f "$LINUXDEPLOY_QT" ]]; then
        echo "Downloading linuxdeploy Qt plugin..."
        wget -q "https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage"
        chmod +x linuxdeploy-plugin-qt-x86_64.AppImage
    fi
    
    # Create AppDir structure
    APPDIR="AppDir"
    rm -rf "$APPDIR"
    mkdir -p "$APPDIR/usr/bin"
    mkdir -p "$APPDIR/usr/share/applications"
    mkdir -p "$APPDIR/usr/share/icons/hicolor/scalable/apps"
    
    # Copy executable
    cp "$EXE_PATH" "$APPDIR/usr/bin/"
    
    # Copy desktop file and icon
    cp QuillScribe.desktop "$APPDIR/usr/share/applications/"
    cp quillscribe.svg "$APPDIR/usr/share/icons/hicolor/scalable/apps/"
    
    # Also copy to root AppDir
    cp QuillScribe.desktop "$APPDIR/"
    cp quillscribe.svg "$APPDIR/quillscribe.svg"
    
    # Deploy with Qt plugin
    export QMAKE="${Qt6_DIR}/bin/qmake"
    if "$LINUXDEPLOY" --appdir "$APPDIR" --plugin qt --output appimage; then
        APPIMAGE_NAME=$(ls QuillScribe-*.AppImage 2>/dev/null | head -1)
        if [[ -n "$APPIMAGE_NAME" ]]; then
            mv "$APPIMAGE_NAME" "QuillScribe-Linux-x86_64.AppImage"
            
            FILE_SIZE=$(stat -c%s "QuillScribe-Linux-x86_64.AppImage")
            FILE_SIZE_MB=$((FILE_SIZE / 1024 / 1024))
            
            echo "✓ Linux AppImage created!"
            echo "File: QuillScribe-Linux-x86_64.AppImage"
            echo "Size: ${FILE_SIZE_MB} MB"
        else
            echo "❌ AppImage not found after creation"
            exit 1
        fi
    else
        echo "❌ Failed to create AppImage"
        exit 1
    fi
    
    # Cleanup
    rm -rf "$APPDIR"
}

# Platform-specific packaging
case "$PLATFORM" in
    "windows"|"wsl")
        package_windows
        ;;
    "macos")
        package_macos
        ;;
    "linux")
        package_linux
        ;;
esac

echo ""
echo "=== Packaging Complete ==="
echo "Your single executable is ready for distribution!"

case "$PLATFORM" in
    "windows"|"wsl")
        echo ""
        echo "Usage:"
        echo "  - Double-click QuillScribe-Portable.exe to run"
        echo "  - No installation required"
        echo "  - Automatically cleans up when closed"
        ;;
    "macos")
        echo ""
        echo "Usage:"
        echo "  - Double-click QuillScribe-macOS.dmg"
        echo "  - Drag QuillScribe to Applications folder"
        echo "  - Or run directly from the DMG"
        ;;
    "linux")
        echo ""
        echo "Usage:"
        echo "  - chmod +x QuillScribe-Linux-x86_64.AppImage"
        echo "  - ./QuillScribe-Linux-x86_64.AppImage"
        echo "  - Or double-click in file manager"
        ;;
esac
