#!/bin/bash

# QuillScribe WSL Audio Setup Script
# Sets up audio capture dependencies for Qt Multimedia in WSL environment

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

echo "=== QuillScribe WSL Audio Setup ==="
echo "Project root: $PROJECT_ROOT"

# Check if we're running in WSL
if ! grep -qi microsoft /proc/version 2>/dev/null; then
    echo "❌ This script is designed for WSL (Windows Subsystem for Linux)"
    echo "For native Linux, use your distribution's package manager to install Qt Multimedia dependencies"
    exit 1
fi

echo "✓ Detected WSL environment"

# Check WSL version and WSLg support
if [[ -n "$WAYLAND_DISPLAY" ]]; then
    echo "✓ WSLg detected (Wayland display: $WAYLAND_DISPLAY)"
    WSLG_AVAILABLE=true
else
    echo "⚠️  WSLg not detected - audio capture may not work"
    echo "   Make sure you're using Windows 11 with WSLg support"
    WSLG_AVAILABLE=false
fi

# Update package list
echo ""
echo "Updating package list..."
sudo apt update

# Install Qt Multimedia and audio dependencies
echo ""
echo "Installing Qt Multimedia and audio dependencies..."
sudo apt install -y \
    libqt6multimedia6 \
    libqt6multimedia6-plugins \
    qt6-multimedia-dev \
    gstreamer1.0-plugins-base \
    gstreamer1.0-plugins-good \
    gstreamer1.0-plugins-bad \
    gstreamer1.0-pulseaudio \
    pulseaudio-utils \
    alsa-utils \
    libasound2-dev \
    libpulse-dev

if [[ "$WSLG_AVAILABLE" == "true" ]]; then
    echo ""
    echo "Installing additional WSLg audio packages..."
    sudo apt install -y \
        pipewire \
        pipewire-pulse \
        pipewire-alsa
fi

# Check audio system status
echo ""
echo "Checking audio system status..."

# Check PulseAudio
if command -v pactl >/dev/null 2>&1; then
    echo "✓ PulseAudio tools available"
    
    if pactl info >/dev/null 2>&1; then
        echo "✓ PulseAudio server is running"
        
        # List audio sources
        echo ""
        echo "Available audio input sources:"
        pactl list sources short 2>/dev/null || echo "  No sources found"
    else
        echo "⚠️  PulseAudio server not running"
        if [[ "$WSLG_AVAILABLE" == "true" ]]; then
            echo "   Try: pulseaudio --start"
        fi
    fi
else
    echo "❌ PulseAudio tools not found"
fi

# Check ALSA
if command -v arecord >/dev/null 2>&1; then
    echo ""
    echo "Available ALSA recording devices:"
    arecord -l 2>/dev/null || echo "  No ALSA recording devices found"
else
    echo "❌ ALSA tools not found"
fi

# Check GStreamer plugins
echo ""
echo "Checking GStreamer plugins..."
if command -v gst-inspect-1.0 >/dev/null 2>&1; then
    if gst-inspect-1.0 pulsesrc >/dev/null 2>&1; then
        echo "✓ GStreamer PulseAudio plugin available"
    else
        echo "❌ GStreamer PulseAudio plugin not found"
    fi
    
    if gst-inspect-1.0 alsasrc >/dev/null 2>&1; then
        echo "✓ GStreamer ALSA plugin available"
    else
        echo "❌ GStreamer ALSA plugin not found"
    fi
else
    echo "⚠️  GStreamer tools not available"
fi

# Test audio capture (if PulseAudio is working)
if pactl info >/dev/null 2>&1 && pactl list sources short | grep -q .; then
    echo ""
    echo "Testing audio capture (5 seconds)..."
    echo "Speak into your microphone..."
    
    if timeout 5 parecord --format=s16le --rate=16000 --channels=1 /dev/null 2>/dev/null; then
        echo "✓ Audio capture test successful"
    else
        echo "⚠️  Audio capture test failed - check microphone permissions"
    fi
fi

# Create test script for Qt audio devices
echo ""
echo "Creating Qt audio device test script..."
cat > "$PROJECT_ROOT/test-qt-audio.sh" << 'EOF'
#!/bin/bash
# Test Qt audio device detection

export QT_DEBUG_PLUGINS=1
export QT_LOGGING_RULES="qt.multimedia*=true"

echo "Testing Qt Multimedia audio device detection..."
echo "This will show debug output for Qt audio plugins"
echo ""

# Simple Qt test program would go here
# For now, we'll test with available tools

if command -v pactl >/dev/null 2>&1; then
    echo "=== PulseAudio Sources ==="
    pactl list sources short
    echo ""
fi

if command -v arecord >/dev/null 2>&1; then
    echo "=== ALSA Recording Devices ==="
    arecord -l
    echo ""
fi

echo "To test with QuillScribe:"
echo "  QT_DEBUG_PLUGINS=1 ./build/default/src/quillscribe"
EOF

chmod +x "$PROJECT_ROOT/test-qt-audio.sh"

echo ""
echo "=== Setup Complete ==="
echo ""
echo "Summary:"
if [[ "$WSLG_AVAILABLE" == "true" ]]; then
    echo "✓ WSLg environment detected"
else
    echo "⚠️  WSLg not available - consider upgrading to Windows 11"
fi
echo "✓ Qt Multimedia packages installed"
echo "✓ Audio system dependencies installed"
echo "✓ Test script created: test-qt-audio.sh"
echo ""
echo "Next steps:"
echo "  1. Build QuillScribe: ./scripts/build.sh"
echo "  2. Test audio devices: ./test-qt-audio.sh"
echo "  3. Run with debug: QT_DEBUG_PLUGINS=1 ./build/default/src/quillscribe"
echo ""
echo "Troubleshooting:"
echo "  - If no audio devices found, restart WSL: wsl --shutdown && wsl"
echo "  - Check Windows audio privacy settings for WSL apps"
echo "  - For better audio support, consider native Windows build"
echo "    Use: cmake --preset windows-msvc && cmake --build build/windows-msvc"
