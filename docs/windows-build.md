## Windows One‑Click Build (QuillScribe)

This guide explains how to build and run QuillScribe on Windows with a single command using the `scripts/build-windows.bat` helper.

### What the script does
- **Auto-detect Qt** under `C:\Qt\6.x.x\msvc*_64` or `C:\Qt\6.x.x\mingw*_64` (prefers MSVC, falls back to MinGW).
- **Detect toolchain** (MSVC or MinGW) from your Qt path and set up the environment.
- **Find CMake/Ninja** (uses Qt Tools if not on PATH) and passes the correct generator.
- **Configure + build** using CMake Presets for Windows.
- **Deploy Qt runtime** with `windeployqt` and ensure the `qwindows` platform plugin exists.
- **Summarize** the result and optionally launch the app.

### Prerequisites
- **Qt 6.5+** with either:
  - MSVC x64 (Visual Studio with C++ tools installed), or
  - MinGW 64‑bit (recommended for quick setup). Example verified: `Qt 6.9.2 MinGW 13.1.0 64‑bit`.
- **Qt Tools**: Ninja and CMake (install via Qt Maintenance Tool) or have them on `PATH`.
- Optional for MSVC: Visual Studio 2019/2022 with C++ workload.

Notes
- The script auto-detects Qt in `C:\Qt`. You can override with `-qtpath`.
- If Ninja/CMake are missing, the script looks in `C:\Qt\Tools\Ninja` and `C:\Qt\Tools\CMake[_64]\bin`.

### Script location
`scripts\build-windows.bat`

### Common usage
- **Debug build, deploy and run**
```bat
scripts\build-windows.bat -deploy -run
```

- **Release build, deploy and run**
```bat
scripts\build-windows.bat -config Release -deploy -run
```

- **Explicit Qt path** (if not under `C:\Qt`)
```bat
scripts\build-windows.bat -qtpath C:\Qt\6.9.2\mingw_64 -deploy -run
```

### Options
- **-config Debug|Release**: Build type (default: Debug)
- **-qtpath <path>**: Set Qt install path (e.g. `C:\Qt\6.9.2\msvc2019_64` or `...\mingw_64`)
- **-deploy**: Run `windeployqt` to stage Qt DLLs and plugins
- **-run**: Launch the app after successful build/deploy
- **-help**: Show help

### What gets built and where
- MinGW Debug: `build\windows-mingw\src\Debug\quillscribe.exe`
- MinGW Release: `build\windows-mingw-release\src\Release\quillscribe.exe`
- MSVC Debug: `build\windows-msvc\src\Debug\quillscribe.exe`
- MSVC Release: `build\windows-msvc-release\src\Release\quillscribe.exe`

### Deployment details
- The script calls `windeployqt` with:
  - Debug: `--debug --pdb --multimedia --network --sql`
  - Release: `--release --multimedia --network --sql`
- If the `qwindows` platform plugin is still missing, the script copies it from your Qt plugins dir to `...\platforms\` under the exe.

### Preflight summary
Before configuring, the script prints a short summary:
- Project root, Qt path, toolchain (MSVC/MinGW)
- Detected CMake/Ninja locations
- MSVC or MinGW path hints

### Troubleshooting
- **Ninja not found**
  - Install Ninja via Qt Maintenance Tool, or `winget install Ninja-build.Ninja`.
  - Ensure it is in PATH or let the script use `C:\Qt\Tools\Ninja`.

- **CMake not found**
  - Install via Qt Maintenance Tool (CMake) or `https://cmake.org/download/`.
  - The script searches `C:\Qt\Tools\CMake[_64]\bin` if not on PATH.

- **Platform plugin missing (qwindows)**
  - The script now ensures `qwindows.dll` (or `qwindowsd.dll` for Debug) is copied to `...\platforms`.
  - For deeper diagnostics:
    ```bat
    set QT_DEBUG_PLUGINS=1
    set QT_QPA_PLATFORM_PLUGIN_PATH=FULL_PATH_TO\build\...\platforms
    FULL_PATH_TO\build\...\quillscribe.exe
    ```

- **MSVC not detected**
  - Install Visual Studio with C++ workload or use the MinGW Qt package.

- **windeployqt failed**
  - Ensure you pointed to the correct Qt kit with `-qtpath`.
  - Re-run with `-deploy`. The script also attempts to copy the platform plugin as a fallback.

### Notes on MSVC vs MinGW
- MSVC builds integrate with Visual Studio and are required for some enterprise environments.
- MinGW builds are simpler to set up and are fully supported by this script.

### Advanced
- Override CMake preset at the CLI if needed (not commonly required). The script selects the proper preset based on toolchain and config.
- `CMAKE_PREFIX_PATH` is passed automatically to point at your Qt path.

### Getting help
```bat
scripts\build-windows.bat -help
```

If you run into issues, include the preflight summary and the last 50 lines of the script output when asking for support.


