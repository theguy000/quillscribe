@echo off
setlocal enabledelayedexpansion

rem QuillScribe Windows Build Script
rem Builds the project natively on Windows with MSVC and Qt
rem Provides full microphone support via WASAPI

set "CONFIG=Debug"
set "DEPLOY=false"
set "RUN=false"
set "QTPATH="
set "TOOLCHAIN=msvc"
set "QTRoot="
set "MINGW_DIR="
set "NINJA_DIR="
set "CMAKE_DIR="
set "NINJA_EXE="

rem Parse command line arguments
:parse_args
if "%1"=="" goto :end_parse
if /i "%1"=="-config" (
    set "CONFIG=%2"
    shift
    shift
    goto :parse_args
)
if /i "%1"=="-qtpath" (
    set "QTPATH=%2"
    shift
    shift
    goto :parse_args
)
if /i "%1"=="-deploy" (
    set "DEPLOY=true"
    shift
    goto :parse_args
)
if /i "%1"=="-run" (
    set "RUN=true"
    shift
    goto :parse_args
)
if /i "%1"=="-help" (
    goto :show_help
)
if /i "%1"=="--help" (
    goto :show_help
)
shift
goto :parse_args

:show_help
echo QuillScribe Windows Build Script
echo.
echo Usage: scripts\build-windows.bat [OPTIONS]
echo.
echo Options:
echo   -config ^<config^>  Build configuration: Debug or Release (default: Debug)
echo   -qtpath ^<path^>    Path to Qt installation (e.g., C:\Qt\6.6.3\msvc2019_64)
echo   -deploy           Run windeployqt to deploy Qt libraries
echo   -run              Launch the app after successful build/deploy
echo   -help             Show this help message
echo.
echo Examples:
echo   scripts\build-windows.bat -qtpath C:\Qt\6.6.3\msvc2019_64
echo   scripts\build-windows.bat -config Release -deploy
goto :end

:end_parse

rem Get script directory and project root (resolve to absolute path)
set "SCRIPT_DIR=%~dp0"
pushd "%SCRIPT_DIR%.." >nul
set "PROJECT_ROOT=%CD%"
popd >nul
cd /d "%PROJECT_ROOT%"

echo === QuillScribe Windows Build Script ===
echo Project root: %PROJECT_ROOT%
echo Build configuration: %CONFIG%
echo.

rem Auto-detect Qt installation if not provided
if "%QTPATH%"=="" (
    echo Auto-detecting Qt installation...
    
    rem Check c:/qt directory first as requested
    if exist "c:\qt" (
        echo Checking c:\qt directory for Qt installations...
        
        rem Look for Qt versions in c:\qt (prefer MSVC, fallback to MinGW)
        for /d %%i in (c:\qt\6.*) do (
            echo Checking Qt version: %%i
            rem First try MSVC (preferred for this project)
            for /d %%j in (%%i\msvc*_64) do (
                if exist "%%j\bin\qmake.exe" (
                    set "QTPATH=%%j"
                    echo Found Qt with MSVC at: !QTPATH!
                    goto :qt_found
                )
            )
            rem Fallback to MinGW if MSVC not found
            for /d %%j in (%%i\mingw*_64) do (
                if exist "%%j\bin\qmake.exe" (
                    set "QTPATH=%%j"
                    echo Found Qt with MinGW at: !QTPATH!
                    echo WARNING: Using MinGW instead of MSVC - some features may not work optimally
                    goto :qt_found
                )
            )
        )
    )
    
    rem Fallback to common paths if c:\qt doesn't work
    echo Checking common Qt installation paths...
    
    set "QT_PATHS[0]=C:\Qt\6.6.3\msvc2019_64"
    set "QT_PATHS[1]=C:\Qt\6.6.2\msvc2019_64"
    set "QT_PATHS[2]=C:\Qt\6.6.1\msvc2019_64"
    set "QT_PATHS[3]=C:\Qt\6.6.0\msvc2019_64"
    set "QT_PATHS[4]=C:\Qt\6.5.3\msvc2019_64"
    set "QT_PATHS[5]=C:\Qt\6.5.2\msvc2019_64"
    set "QT_PATHS[6]=C:\Qt\6.5.1\msvc2019_64"
    set "QT_PATHS[7]=C:\Qt\6.5.0\msvc2019_64"
    
    for /l %%i in (0,1,7) do (
        if exist "!QT_PATHS[%%i]!\bin\qmake.exe" (
            set "QTPATH=!QT_PATHS[%%i]!"
            echo Found Qt at: !QTPATH!
            goto :qt_found
        )
    )
    
    echo ERROR: Qt installation not found!
    echo Please install Qt 6.5+ for MSVC and specify the path with -qtpath
    echo Download from: https://www.qt.io/download-qt-installer
    goto :error
)

:qt_found
if not exist "%QTPATH%\bin\qmake.exe" (
    echo ERROR: Qt not found at: %QTPATH%
    echo Please check the Qt installation path
    goto :error
)

echo Found Qt at: %QTPATH%
echo.

rem Determine Qt root (typically C:\Qt)
for %%r in ("%QTPATH%\..\..") do set "QTRoot=%%~fr"
if not exist "%QTRoot%" (
    rem Fallback: try parent once
    for %%r in ("%QTPATH%\..") do set "QTRoot=%%~fr"
)
echo Qt root detected: %QTRoot%

rem Detect toolchain from QTPATH
set "_tmp=%QTPATH:mingw=%"
if /i not "%_tmp%"=="%QTPATH%" (
    set "TOOLCHAIN=mingw"
)
echo Using toolchain: %TOOLCHAIN%

rem Check for required tools
echo Checking build tools...

rem Check CMake
cmake --version >nul 2>&1
if errorlevel 1 (
    rem Try to find CMake within Qt Tools
    if exist "%QTRoot%\Tools\CMake_64\bin\cmake.exe" (
        set "CMAKE_DIR=%QTRoot%\Tools\CMake_64\bin"
        set "PATH=%CMAKE_DIR%;%PATH%"
    ) else (
        if exist "%QTRoot%\Tools\CMake\bin\cmake.exe" (
            set "CMAKE_DIR=%QTRoot%\Tools\CMake\bin"
            set "PATH=%CMAKE_DIR%;%PATH%"
        )
    )
)

cmake --version >nul 2>&1
if errorlevel 1 (
    echo ERROR: CMake not found!
    echo Please install CMake 3.20+ and add it to PATH, or install Qt Tools CMake.
    echo Download from: https://cmake.org/download/
    goto :error
)

for /f "tokens=3" %%i in ('cmake --version ^| findstr "cmake version"') do (
    echo Found CMake version %%i
)

rem Check Ninja
rem Try PATH first
ninja --version >nul 2>&1
if errorlevel 1 (
    rem Try to find Ninja within Qt directories
    if exist "%QTPATH%\bin\ninja.exe" (
        set "NINJA_EXE=%QTPATH%\bin\ninja.exe"
        set "NINJA_DIR=%QTPATH%\bin"
        set "PATH=%NINJA_DIR%;%PATH%"
    ) else (
        if exist "%QTRoot%\Tools\Ninja\ninja.exe" (
            set "NINJA_EXE=%QTRoot%\Tools\Ninja\ninja.exe"
            set "NINJA_DIR=%QTRoot%\Tools\Ninja"
            set "PATH=%NINJA_DIR%;%PATH%"
        )
    )
)

if not "%NINJA_EXE%"=="" (
    "%NINJA_EXE%" --version >nul 2>&1
    if errorlevel 1 (
        echo ERROR: Ninja found at %NINJA_EXE% but failed to run
        goto :error
    )
    echo Found Ninja at %NINJA_EXE%
) else (
    ninja --version >nul 2>&1
    if errorlevel 1 (
        echo ERROR: Ninja not found!
        echo Please install Ninja build system and add it to PATH
        echo Download from: https://ninja-build.org/
        echo Or install via: winget install Ninja-build.Ninja
        goto :error
    ) else (
        echo Found Ninja in PATH
    )
)

if /i "%TOOLCHAIN%"=="msvc" (
    echo Checking MSVC toolchain...
    set "VS_WHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
    if exist "%VS_WHERE%" (
        for /f "usebackq tokens=*" %%i in (`"%VS_WHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
            set "VS_INSTALLATION=%%i"
        )
        
        if "!VS_INSTALLATION!"=="" (
            echo ERROR: Visual Studio with C++ tools not found!
            echo Please install Visual Studio with C++ development tools
            goto :error
        )
        
        echo Found Visual Studio at: !VS_INSTALLATION!
    ) else (
        echo WARNING: Visual Studio installer not found - assuming MSVC is available
        set "VS_INSTALLATION="
    )
) else (
    echo Skipping MSVC detection for MinGW toolchain
)

if /i "%TOOLCHAIN%"=="msvc" (
    echo.
    echo Setting up MSVC environment...
    rem Setup MSVC environment if VS found
    if not "!VS_INSTALLATION!"=="" (
        set "VC_VARS=!VS_INSTALLATION!\VC\Auxiliary\Build\vcvars64.bat"
        if exist "!VC_VARS!" (
            echo Running vcvars64.bat...
            call "!VC_VARS!"
            echo MSVC environment configured
        ) else (
            echo WARNING: vcvars64.bat not found - assuming environment is already set
        )
    ) else (
        echo WARNING: Using existing environment - ensure MSVC is available
    )
) else (
    echo.
    echo Setting up MinGW environment...
    rem Add MinGW toolchain from Qt Tools to PATH
    rem Prefer the toolchain that matches QTPATH first
    if exist "%QTPATH%\bin\g++.exe" (
        set "MINGW_DIR=%QTPATH%\bin"
    )
    for /d %%d in ("%QTRoot%\Tools\mingw*_64") do (
        if exist "%%d\bin\g++.exe" (
            set "MINGW_DIR=%%d\bin"
            goto :mingw_found
        )
    )
    :mingw_found
    if not "%MINGW_DIR%"=="" (
        set "PATH=%MINGW_DIR%;%PATH%"
        echo MinGW toolchain detected at: %MINGW_DIR%
    ) else (
        echo WARNING: MinGW toolchain not found under %QTRoot%\Tools\mingw*_64
        echo Ensure Qt MinGW toolchain is installed via Qt Maintenance Tool
    )
)

echo.
echo === Preflight Summary ===
echo   Project root    : %PROJECT_ROOT%
echo   Qt path         : %QTPATH%
echo   Toolchain       : %TOOLCHAIN%
if defined CMAKE_DIR (
    echo   CMake         : %CMAKE_DIR%\cmake.exe
) else (
    for /f "tokens=*" %%p in ('where cmake 2^>nul') do (
        echo   CMake         : %%p
        goto :_cmake_done
    )
    echo   CMake         : from PATH (location unknown)
:_cmake_done
)
if defined NINJA_EXE (
    echo   Ninja         : %NINJA_EXE%
) else (
    for /f "tokens=*" %%p in ('where ninja 2^>nul') do (
        echo   Ninja         : %%p
        goto :_ninja_done
    )
    echo   Ninja         : from PATH (location unknown)
:_ninja_done
)
if /i "%TOOLCHAIN%"=="msvc" (
    if defined VS_INSTALLATION (
        echo   MSVC         : !VS_INSTALLATION!
    ) else (
        echo   MSVC         : not detected (install Visual Studio with C++ tools)
    )
) else (
    if defined MINGW_DIR (
        echo   MinGW        : %MINGW_DIR%
    ) else (
        echo   MinGW        : not detected under %QTRoot%\Tools (install MinGW via Qt Maintenance Tool)
    )
)
echo ===========================

rem Determine preset and build directory
if /i "%TOOLCHAIN%"=="msvc" (
    if /i "%CONFIG%"=="Release" (
        set "PRESET=windows-msvc-release"
    ) else (
        set "PRESET=windows-msvc"
    )
) else (
    if /i "%CONFIG%"=="Release" (
        set "PRESET=windows-mingw-release"
    ) else (
        set "PRESET=windows-mingw"
    )
)

set "BUILD_DIR=build\%PRESET%"

rem Create build directory if it doesn't exist
if not exist "%BUILD_DIR%" (
    echo Creating build directory: %BUILD_DIR%
    mkdir "%BUILD_DIR%"
)

echo.
echo Configuring build with preset: %PRESET%

rem Configure CMake
set "CMAKE_MAKE_PROGRAM_ARG="
if not "%NINJA_EXE%"=="" set "CMAKE_MAKE_PROGRAM_ARG=-DCMAKE_MAKE_PROGRAM=%NINJA_EXE%"
cmake --preset "%PRESET%" -DCMAKE_PREFIX_PATH="%QTPATH%" %CMAKE_MAKE_PROGRAM_ARG%
if errorlevel 1 (
    echo ERROR: Configuration failed!
    echo Common issues:
    echo   - Qt path incorrect: %QTPATH%
    echo   - MSVC not properly configured
    echo   - Missing dependencies
    goto :error
)

echo Configuration successful

echo.
echo Building project...

rem Build project
cmake --build "%BUILD_DIR%" --config "%CONFIG%"
if errorlevel 1 (
    echo ERROR: Build failed!
    echo Check the build output above for specific errors
    goto :error
)

echo Build successful

rem Deploy Qt libraries if requested or if Release build
if "%DEPLOY%"=="true" (
    goto :do_deploy
)
if /i "%CONFIG%"=="Release" (
    goto :do_deploy
)
goto :skip_deploy

:do_deploy
echo.
echo Deploying Qt libraries...

set "EXE_PATH=%BUILD_DIR%\src\%CONFIG%\quillscribe.exe"
set "WIN_DEPLOY_QT=%QTPATH%\bin\windeployqt.exe"

if exist "%EXE_PATH%" (
    if exist "%WIN_DEPLOY_QT%" (
        set "WINDEPLOY_FLAGS=--multimedia --network --sql"
        if /i "%CONFIG%"=="Debug" (
            set "WINDEPLOY_FLAGS=%WINDEPLOY_FLAGS% --debug --pdb"
        ) else (
            set "WINDEPLOY_FLAGS=%WINDEPLOY_FLAGS% --release"
        )
        "%WIN_DEPLOY_QT%" "%EXE_PATH%" %WINDEPLOY_FLAGS%
        if errorlevel 1 (
            echo WARNING: windeployqt failed - you may need to copy Qt DLLs manually
        ) else (
            echo Qt libraries deployed successfully
        )
    ) else (
        echo WARNING: windeployqt not found at: %WIN_DEPLOY_QT%
    )
) else (
    echo WARNING: Executable not found at: %EXE_PATH%
)

rem Ensure platform plugin exists (qwindows)
set "EXE_DIR=%BUILD_DIR%\src\%CONFIG%"
if exist "%EXE_DIR%\platforms\qwindows.dll" (
    rem ok
) else if exist "%EXE_DIR%\platforms\qwindowsd.dll" (
    rem ok
) else (
    echo Ensuring Qt platform plugin is present...
    if not exist "%EXE_DIR%\platforms" mkdir "%EXE_DIR%\platforms" >nul 2>&1
    if /i "%CONFIG%"=="Debug" (
        if exist "%QTPATH%\plugins\platforms\qwindowsd.dll" (
            copy /Y "%QTPATH%\plugins\platforms\qwindowsd.dll" "%EXE_DIR%\platforms\" >nul
        ) else if exist "%QTPATH%\plugins\platforms\qwindows.dll" (
            copy /Y "%QTPATH%\plugins\platforms\qwindows.dll" "%EXE_DIR%\platforms\" >nul
        )
    ) else (
        if exist "%QTPATH%\plugins\platforms\qwindows.dll" (
            copy /Y "%QTPATH%\plugins\platforms\qwindows.dll" "%EXE_DIR%\platforms\" >nul
        )
    )
    if not exist "%EXE_DIR%\platforms\qwindows.dll" if not exist "%EXE_DIR%\platforms\qwindowsd.dll" (
        echo WARNING: qwindows platform plugin is still missing.
        echo          Ensure Qt plugins are installed. If using Debug, install Qt debug components.
    ) else (
        echo Platform plugin present.
    )
)

:skip_deploy

rem Run tests
echo.
echo Running tests...

set "TEST_EXE=%BUILD_DIR%\tests\%CONFIG%\contract_tests.exe"
set "TESTS_PASSED=true"

if exist "%TEST_EXE%" (
    "%TEST_EXE%"
    if errorlevel 1 (
        echo Some tests failed
        set "TESTS_PASSED=false"
    ) else (
        echo All tests passed
    )
) else (
    echo No tests found at: %TEST_EXE%
)

rem Final summary
echo.
echo === Build Complete ===
echo Build artifacts are in: %BUILD_DIR%\
echo Executable: %BUILD_DIR%\src\%CONFIG%\quillscribe.exe

if "%DEPLOY%"=="true" (
    echo Qt libraries deployed - ready to run
)
if /i "%CONFIG%"=="Release" (
    echo Qt libraries deployed - ready to run
)

echo.
echo To run QuillScribe:
echo   %BUILD_DIR%\src\%CONFIG%\quillscribe.exe
echo.
echo Next steps:
echo   1. Setup whisper models: .\scripts\setup-whisper-models.sh
echo   2. Test microphone input (should work natively on Windows!)
echo   3. Follow development workflow from tasks.md

rem Optional: run the app if requested
if "%RUN%"=="true" (
    set "EXE_PATH=%BUILD_DIR%\src\%CONFIG%\quillscribe.exe"
    if exist "%EXE_PATH%" (
        echo.
        echo Launching QuillScribe...
        start "QuillScribe" "%EXE_PATH%"
    ) else (
        echo WARN: Cannot run app - executable not found at %EXE_PATH%
    )
)

if "%TESTS_PASSED%"=="false" (
    echo.
    echo ERROR: Tests failed - please fix failing tests before proceeding
    goto :error
)

goto :end

:error
echo.
echo Build failed. Please check the errors above.
exit /b 1

:end
echo.
echo Build completed successfully!
exit /b 0
