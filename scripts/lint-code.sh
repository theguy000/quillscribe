#!/bin/bash

# QuillScribe Code Linting Script  
# Runs clang-tidy static analysis on all C++ source files

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_ROOT/build/default"

echo "=== QuillScribe Code Linting ==="
echo "Project root: $PROJECT_ROOT"
echo "Build directory: $BUILD_DIR"

# Check if clang-tidy is available
if ! command -v clang-tidy >/dev/null 2>&1; then
    echo "Error: clang-tidy is not installed or not in PATH"
    echo "Please install clang-tidy (version 12 or later recommended)"
    exit 1
fi

# Get clang-tidy version
CLANG_TIDY_VERSION=$(clang-tidy --version)
echo "Using: $CLANG_TIDY_VERSION"

# Check if compile_commands.json exists
COMPILE_COMMANDS="$BUILD_DIR/compile_commands.json"
if [[ ! -f "$COMPILE_COMMANDS" ]]; then
    echo "Warning: compile_commands.json not found at $COMPILE_COMMANDS"
    echo "Run cmake build first to generate compilation database:"
    echo "  cmake --preset default"
    echo "  cmake --build --preset default"
    echo ""
    echo "Continuing with basic linting (may have limited effectiveness)..."
    COMPILE_COMMANDS=""
fi

# Find all C++ source files
echo "Finding C++ source files..."
CPP_FILES=$(find "$PROJECT_ROOT/src" "$PROJECT_ROOT/tests" \
    -type f \
    \( -name "*.cpp" -o -name "*.hpp" -o -name "*.h" -o -name "*.cc" -o -name "*.cxx" \) \
    2>/dev/null | sort)

if [[ -z "$CPP_FILES" ]]; then
    echo "No C++ files found to lint"
    exit 0
fi

echo "Found $(echo "$CPP_FILES" | wc -l) C++ files"

# Prepare clang-tidy command
CLANG_TIDY_CMD="clang-tidy"
if [[ -n "$COMPILE_COMMANDS" ]]; then
    CLANG_TIDY_CMD="$CLANG_TIDY_CMD -p $BUILD_DIR"
fi

# Add header filter
CLANG_TIDY_CMD="$CLANG_TIDY_CMD --header-filter='(src|tests)/.*\.(h|hpp)$'"

# Handle command line options
FIX_MODE=false
if [[ "$1" == "--fix" ]]; then
    FIX_MODE=true
    CLANG_TIDY_CMD="$CLANG_TIDY_CMD --fix"
    echo "Running in FIX mode - will apply suggested changes"
fi

# Run linting
echo ""
echo "Running clang-tidy analysis..."
LINT_ERRORS=0

for file in $CPP_FILES; do
    echo "Linting: $file"
    
    # Run clang-tidy on the file
    if eval "$CLANG_TIDY_CMD '$file'" 2>&1; then
        echo "  ✓ No issues found"
    else
        echo "  ⚠ Issues found"
        ((LINT_ERRORS++))
    fi
done

echo ""
echo "=== Linting Complete ==="
if [[ $LINT_ERRORS -eq 0 ]]; then
    echo "✓ No linting errors found"
else
    echo "⚠ Found issues in $LINT_ERRORS files"
    echo ""
    echo "To automatically fix some issues:"
    echo "  $0 --fix"
fi

echo ""
echo "Configuration files:"
echo "  .clang-tidy - Linting rules and naming conventions"
echo "  .clang-format - Code formatting style"

exit $LINT_ERRORS

