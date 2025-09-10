#!/bin/bash

# QuillScribe Pre-commit Hook
# Runs code formatting and linting before commits

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

echo "=== QuillScribe Pre-commit Checks ==="

# Check if we're in a git repository
if ! git rev-parse --git-dir >/dev/null 2>&1; then
    echo "Error: Not in a git repository"
    exit 1
fi

# Get list of staged C++ files
STAGED_CPP_FILES=$(git diff --cached --name-only --diff-filter=ACM | grep -E '\.(cpp|hpp|h|cc|cxx)$' || true)

if [[ -z "$STAGED_CPP_FILES" ]]; then
    echo "No C++ files staged for commit"
    exit 0
fi

echo "Found staged C++ files:"
echo "$STAGED_CPP_FILES" | sed 's/^/  /'

# Format staged files
echo ""
echo "Formatting staged files..."
for file in $STAGED_CPP_FILES; do
    if [[ -f "$file" ]]; then
        echo "Formatting: $file"
        clang-format -i "$file"
        git add "$file"  # Re-add formatted file
    fi
done

# Run quick lint check on staged files
echo ""
echo "Running lint checks..."
LINT_ISSUES=0
for file in $STAGED_CPP_FILES; do
    if [[ -f "$file" ]]; then
        echo "Checking: $file"
        if ! clang-tidy "$file" --quiet 2>/dev/null; then
            echo "  ⚠ Lint issues found in $file"
            ((LINT_ISSUES++))
        else
            echo "  ✓ Clean"
        fi
    fi
done

# Summary
echo ""
echo "=== Pre-commit Summary ==="
if [[ $LINT_ISSUES -eq 0 ]]; then
    echo "✓ All checks passed"
    echo "✓ Code formatted and staged"
else
    echo "⚠ Found $LINT_ISSUES files with lint issues"
    echo ""
    echo "Commit will proceed, but consider running:"
    echo "  ./scripts/lint-code.sh --fix"
    echo ""
    echo "To skip this hook for emergency commits:"
    echo "  git commit --no-verify"
fi

exit 0

