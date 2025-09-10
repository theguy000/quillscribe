#!/bin/bash
# QuillScribe Single Executable Packager
# Creates portable Windows executable using NSIS

echo "=== QuillScribe Single Executable Packager ==="

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
echo "✓ Creating Windows portable executable..."

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
    ExecWait "\"$INSTDIR\quillscribe.exe\"" $0
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

echo "=== Packaging Complete ==="
