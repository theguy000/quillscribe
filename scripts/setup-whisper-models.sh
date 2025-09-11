#!/bin/bash

# QuillScribe - Whisper Model Setup Script
# Downloads and sets up whisper.cpp models for local transcription

set -e

# Ensure we're using bash (not sh)
if [ -z "$BASH_VERSION" ]; then
    echo "This script requires bash. Trying to re-exec with bash..."
    exec bash "$0" "$@"
fi

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
MODELS_DIR="$PROJECT_ROOT/models/whisper"

echo "=== QuillScribe Whisper Model Setup ==="
echo "Project root: $PROJECT_ROOT"
echo "Models directory: $MODELS_DIR"

# Create models directory
mkdir -p "$MODELS_DIR"

# Model URLs and sizes (using simple arrays for compatibility)
get_model_url() {
    case "$1" in
        "tiny")    echo "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-tiny.bin" ;;
        "base")    echo "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-base.bin" ;;
        "small")   echo "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-small.bin" ;;
        "medium")  echo "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-medium.bin" ;;
        "large-v3") echo "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-large-v3.bin" ;;
        *) echo "" ;;
    esac
}

get_model_size() {
    case "$1" in
        "tiny")    echo "39M" ;;
        "base")    echo "142M" ;;
        "small")   echo "466M" ;;
        "medium")  echo "1.5G" ;;
        "large-v3") echo "2.9G" ;;
        *) echo "Unknown" ;;
    esac
}

# Available models list
AVAILABLE_MODELS="tiny base small medium large-v3"

# Function to download model if not exists
download_model() {
    local model_name="$1"
    local model_url="$(get_model_url "$model_name")"
    local model_file="$MODELS_DIR/ggml-${model_name}.bin"
    local model_size="$(get_model_size "$model_name")"
    
    if [[ -z "$model_url" ]]; then
        echo "Error: Unknown model '$model_name'"
        echo "Available models: $AVAILABLE_MODELS"
        return 1
    fi
    
    if [[ -f "$model_file" ]]; then
        echo "✓ Model $model_name already exists ($model_size)"
        return 0
    fi
    
    echo "Downloading $model_name model ($model_size)..."
    if command -v wget >/dev/null 2>&1; then
        wget -q --show-progress -O "$model_file" "$model_url"
    elif command -v curl >/dev/null 2>&1; then
        curl -L --progress-bar -o "$model_file" "$model_url"
    else
        echo "Error: Neither wget nor curl is available for downloading"
        return 1
    fi
    
    echo "✓ Downloaded $model_name model"
}

# Default setup - download tiny and base models for development
echo ""
echo "Setting up default models (tiny, base) for development..."
download_model "tiny"
download_model "base"

echo ""
echo "=== Setup Complete ==="
echo "Default models installed:"
echo "  - tiny (39M) - Fastest, lower accuracy"
echo "  - base (142M) - Good balance of speed and accuracy"
echo ""
echo "To install additional models, run:"
echo "  $0 small    # 466M - Better accuracy"
echo "  $0 medium   # 1.5G - High accuracy"  
echo "  $0 large-v3 # 2.9G - Highest accuracy"
echo ""
echo "Models location: $MODELS_DIR"

# Handle command line arguments for specific models
if [[ $# -gt 0 ]]; then
    for model in "$@"; do
        model_url="$(get_model_url "$model")"
        if [[ -n "$model_url" ]]; then
            echo ""
            echo "Installing $model model..."
            download_model "$model"
        else
            echo "Error: Unknown model '$model'"
            echo "Available models: $AVAILABLE_MODELS"
            exit 1
        fi
    done
fi
