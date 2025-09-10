#!/bin/bash

# QuillScribe - Whisper Model Setup Script
# Downloads and sets up whisper.cpp models for local transcription

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
MODELS_DIR="$PROJECT_ROOT/models/whisper"

echo "=== QuillScribe Whisper Model Setup ==="
echo "Project root: $PROJECT_ROOT"
echo "Models directory: $MODELS_DIR"

# Create models directory
mkdir -p "$MODELS_DIR"

# Model URLs and info
declare -A MODELS=(
    ["tiny"]="https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-tiny.bin"
    ["base"]="https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-base.bin"  
    ["small"]="https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-small.bin"
    ["medium"]="https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-medium.bin"
    ["large-v3"]="https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-large-v3.bin"
)

declare -A MODEL_SIZES=(
    ["tiny"]="39M"
    ["base"]="142M"
    ["small"]="466M" 
    ["medium"]="1.5G"
    ["large-v3"]="2.9G"
)

# Function to download model if not exists
download_model() {
    local model_name="$1"
    local model_url="$2"
    local model_file="$MODELS_DIR/ggml-${model_name}.bin"
    local model_size="${MODEL_SIZES[$model_name]}"
    
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
download_model "tiny" "${MODELS[tiny]}"
download_model "base" "${MODELS[base]}"

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
        if [[ -n "${MODELS[$model]}" ]]; then
            echo ""
            echo "Installing $model model..."
            download_model "$model" "${MODELS[$model]}"
        else
            echo "Error: Unknown model '$model'"
            echo "Available models: ${!MODELS[*]}"
            exit 1
        fi
    done
fi
