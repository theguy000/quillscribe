# QuillScribe Windows Build Script
# Builds the project natively on Windows with MSVC and Qt
# Provides full microphone support via WASAPI

param(
    [string]$QtPath = "",
    [string]$Config = "Debug",
    [switch]$Deploy = $false,
    [switch]$Help = $false
)

if ($Help) {
    Write-Host "QuillScribe Windows Build Script"
    Write-Host ""
    Write-Host "Usage: .\scripts\build-windows.ps1 [OPTIONS]"
    Write-Host ""
    Write-Host "Options:"
    Write-Host "  -QtPath <path>    Path to Qt installation (e.g., C:\Qt\6.6.3\msvc2019_64)"
    Write-Host "  -Config <config>  Build configuration: Debug or Release (default: Debug)"
    Write-Host "  -Deploy           Run windeployqt to deploy Qt libraries"
    Write-Host "  -Help             Show this help message"
    Write-Host ""
    Write-Host "Examples:"
    Write-Host "  .\scripts\build-windows.ps1 -QtPath C:\Qt\6.6.3\msvc2019_64"
    Write-Host "  .\scripts\build-windows.ps1 -Config Release -Deploy"
    exit 0
}

$ErrorActionPreference = "Stop"
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$ProjectRoot = Split-Path -Parent $ScriptDir

Write-Host "=== QuillScribe Windows Build Script ===" -ForegroundColor Cyan
Write-Host "Project root: $ProjectRoot"
Write-Host "Build configuration: $Config"

Set-Location $ProjectRoot

# Auto-detect Qt installation if not provided
if (-not $QtPath) {
    Write-Host "Auto-detecting Qt installation..." -ForegroundColor Yellow
    
    $CommonQtPaths = @(
        "C:\Qt\6.6.3\msvc2019_64",
        "C:\Qt\6.6.2\msvc2019_64", 
        "C:\Qt\6.6.1\msvc2019_64",
        "C:\Qt\6.6.0\msvc2019_64",
        "C:\Qt\6.5.3\msvc2019_64",
        "C:\Qt\6.5.2\msvc2019_64",
        "C:\Qt\6.5.1\msvc2019_64",
        "C:\Qt\6.5.0\msvc2019_64"
    )
    
    foreach ($Path in $CommonQtPaths) {
        if (Test-Path "$Path\bin\qmake.exe") {
            $QtPath = $Path
            Write-Host "Found Qt at: $QtPath" -ForegroundColor Green
            break
        }
    }
    
    if (-not $QtPath) {
        Write-Host "❌ Qt installation not found!" -ForegroundColor Red
        Write-Host "Please install Qt 6.5+ for MSVC and specify the path with -QtPath"
        Write-Host "Download from: https://www.qt.io/download-qt-installer"
        exit 1
    }
}

if (-not (Test-Path "$QtPath\bin\qmake.exe")) {
    Write-Host "❌ Qt not found at: $QtPath" -ForegroundColor Red
    Write-Host "Please check the Qt installation path"
    exit 1
}

Write-Host "✓ Using Qt at: $QtPath" -ForegroundColor Green

# Check for required tools
Write-Host ""
Write-Host "Checking build tools..." -ForegroundColor Yellow

# Check CMake
if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {
    Write-Host "❌ CMake not found!" -ForegroundColor Red
    Write-Host "Please install CMake 3.20+ and add it to PATH"
    Write-Host "Download from: https://cmake.org/download/"
    exit 1
}

$CmakeVersion = & cmake --version | Select-Object -First 1
Write-Host "✓ Found $CmakeVersion" -ForegroundColor Green

# Check Ninja
if (-not (Get-Command ninja -ErrorAction SilentlyContinue)) {
    Write-Host "❌ Ninja not found!" -ForegroundColor Red
    Write-Host "Please install Ninja build system and add it to PATH"
    Write-Host "Download from: https://ninja-build.org/"
    Write-Host "Or install via: winget install Ninja-build.Ninja"
    exit 1
}

Write-Host "✓ Found Ninja build system" -ForegroundColor Green

# Check MSVC
$VsWhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (Test-Path $VsWhere) {
    $VsInstallation = & $VsWhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if ($VsInstallation) {
        Write-Host "✓ Found Visual Studio at: $VsInstallation" -ForegroundColor Green
    } else {
        Write-Host "❌ Visual Studio with C++ tools not found!" -ForegroundColor Red
        Write-Host "Please install Visual Studio with C++ development tools"
        exit 1
    }
} else {
    Write-Host "⚠️  Visual Studio installer not found - assuming MSVC is available" -ForegroundColor Yellow
}

# Setup MSVC environment
Write-Host ""
Write-Host "Setting up MSVC environment..." -ForegroundColor Yellow

$VcVarsScript = "$VsInstallation\VC\Auxiliary\Build\vcvars64.bat"
if (Test-Path $VcVarsScript) {
    Write-Host "Running vcvars64.bat..." -ForegroundColor Gray
    cmd /c "`"$VcVarsScript`" && set" | ForEach-Object {
        if ($_ -match "^(.*?)=(.*)$") {
            Set-Item "env:$($matches[1])" $matches[2]
        }
    }
    Write-Host "✓ MSVC environment configured" -ForegroundColor Green
} else {
    Write-Host "⚠️  vcvars64.bat not found - assuming environment is already set" -ForegroundColor Yellow
}

# Determine preset and build directory
$Preset = if ($Config -eq "Release") { "windows-msvc-release" } else { "windows-msvc" }
$BuildDir = "build\$Preset"

Write-Host ""
Write-Host "Configuring build with preset: $Preset" -ForegroundColor Yellow

# Configure CMake
try {
    & cmake --preset $Preset -DCMAKE_PREFIX_PATH="$QtPath"
    Write-Host "✓ Configuration successful" -ForegroundColor Green
} catch {
    Write-Host "❌ Configuration failed!" -ForegroundColor Red
    Write-Host "Common issues:" -ForegroundColor Yellow
    Write-Host "  - Qt path incorrect: $QtPath"
    Write-Host "  - MSVC not properly configured"
    Write-Host "  - Missing dependencies"
    Write-Host ""
    Write-Host "Error details:" -ForegroundColor Red
    Write-Host $_.Exception.Message
    exit 1
}

# Build project
Write-Host ""
Write-Host "Building project..." -ForegroundColor Yellow

try {
    & cmake --build $BuildDir --config $Config
    Write-Host "✓ Build successful" -ForegroundColor Green
} catch {
    Write-Host "❌ Build failed!" -ForegroundColor Red
    Write-Host "Check the build output above for specific errors"
    exit 1
}

# Deploy Qt libraries if requested or if Release build
if ($Deploy -or $Config -eq "Release") {
    Write-Host ""
    Write-Host "Deploying Qt libraries..." -ForegroundColor Yellow
    
    $ExePath = "$BuildDir\src\$Config\quillscribe.exe"
    $WinDeployQt = "$QtPath\bin\windeployqt.exe"
    
    if (Test-Path $ExePath) {
        if (Test-Path $WinDeployQt) {
            try {
                & $WinDeployQt $ExePath --multimedia --network --sql --debug-info
                Write-Host "✓ Qt libraries deployed successfully" -ForegroundColor Green
            } catch {
                Write-Host "⚠️  windeployqt failed - you may need to copy Qt DLLs manually" -ForegroundColor Yellow
            }
        } else {
            Write-Host "⚠️  windeployqt not found at: $WinDeployQt" -ForegroundColor Yellow
        }
    } else {
        Write-Host "⚠️  Executable not found at: $ExePath" -ForegroundColor Yellow
    }
}

# Run tests
Write-Host ""
Write-Host "Running tests..." -ForegroundColor Yellow

$TestExe = "$BuildDir\tests\$Config\contract_tests.exe"
if (Test-Path $TestExe) {
    try {
        & $TestExe
        Write-Host "✓ All tests passed" -ForegroundColor Green
        $TestsPassed = $true
    } catch {
        Write-Host "❌ Some tests failed" -ForegroundColor Red
        $TestsPassed = $false
    }
} else {
    Write-Host "No tests found at: $TestExe" -ForegroundColor Yellow
    $TestsPassed = $true
}

# Final summary
Write-Host ""
Write-Host "=== Build Complete ===" -ForegroundColor Cyan
Write-Host "Build artifacts are in: $BuildDir\"
Write-Host "Executable: $BuildDir\src\$Config\quillscribe.exe"

if ($Deploy -or $Config -eq "Release") {
    Write-Host "✓ Qt libraries deployed - ready to run" -ForegroundColor Green
}

Write-Host ""
Write-Host "To run QuillScribe:" -ForegroundColor Green
Write-Host "  $BuildDir\src\$Config\quillscribe.exe"
Write-Host ""
Write-Host "Next steps:" -ForegroundColor Yellow
Write-Host "  1. Setup whisper models: .\scripts\setup-whisper-models.sh"
Write-Host "  2. Test microphone input (should work natively on Windows!)"
Write-Host "  3. Follow development workflow from tasks.md"

if (-not $TestsPassed) {
    Write-Host ""
    Write-Host "❌ Tests failed - please fix failing tests before proceeding" -ForegroundColor Red
    exit 1
}
