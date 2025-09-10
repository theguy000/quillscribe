#!/bin/bash

# QuillScribe Code Formatting Script
# Formats all C++ source files using clang-format

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

echo "=== QuillScribe Code Formatting ==="
echo "Project root: $PROJECT_ROOT"

# Check if clang-format is available
if ! command -v clang-format >/dev/null 2>&1; then
    echo "Error: clang-format is not installed or not in PATH"
    echo "Please install clang-format (version 12 or later recommended)"
    exit 1
fi

# Get clang-format version
CLANG_FORMAT_VERSION=$(clang-format --version)
echo "Using: $CLANG_FORMAT_VERSION"

# Find all C++ source files
echo "Finding C++ source files..."
CPP_FILES=$(find "$PROJECT_ROOT" \
    -type f \
    \( -name "*.cpp" -o -name "*.hpp" -o -name "*.h" -o -name "*.cc" -o -name "*.cxx" \) \
    -not -path "*/build/*" \
    -not -path "*/external/*" \
    -not -path "*/.git/*" \
    -not -path "*/.*" \
    | sort)

if [[ -z "$CPP_FILES" ]]; then
    echo "No C++ files found to format"
    exit 0
fi

echo "Found $(echo "$CPP_FILES" | wc -l) C++ files"

# Format files
FORMATTED_COUNT=0
for file in $CPP_FILES; do
    echo "Formatting: $file"
    if clang-format -i "$file"; then
        ((FORMATTED_COUNT++))
    else
        echo "Warning: Failed to format $file"
    fi
done

echo ""
echo "=== Formatting Complete ==="
echo "Formatted $FORMATTED_COUNT files"
echo ""
echo "To check formatting without modifying files:"
echo "  $0 --check"
echo ""
echo "To format specific files:"
echo "  clang-format -i path/to/file.cpp"

