# QuillScribe

Voice-to-Text Application with AI Enhancement

## Overview

QuillScribe is a cross-platform desktop application for voice recording, speech-to-text transcription, and AI-powered text enhancement. Built with Qt framework for native performance and user experience.

### Key Features

- **One-touch voice recording** with real-time feedback
- **Local speech-to-text transcription** using whisper.cpp (<2s for 1min audio)
- **AI text enhancement** using Google Gemini API with multiple modes
- **Session-based recording management**
- **Offline capability** for transcription with local whisper models
- **Multi-language support** with model abstraction

## Technical Stack

- **C++17/20** with modern features
- **Qt 6.5+** for cross-platform GUI
- **whisper.cpp** for local speech-to-text
- **Google Gemini API** for AI enhancement
- **SQLite** for local data storage
- **CMake 3.20+** build system

## Quick Start

### Prerequisites

- Qt 6.5 or later
- CMake 3.20 or later
- C++17 compatible compiler
- clang-format and clang-tidy (optional, for development)

### Building

```bash
# Clone the repository
git clone <repository-url>
cd quillscribe

# Configure build
cmake --preset default

# Build
cmake --build --preset default

# Run tests
ctest --preset default
```

### Development Setup

```bash
# Setup whisper models (downloads tiny and base models)
./scripts/setup-whisper-models.sh

# Format code
./scripts/format-code.sh

# Run linting
./scripts/lint-code.sh

# Install pre-commit hook (optional)
ln -sf ../../scripts/pre-commit-hook.sh .git/hooks/pre-commit
```

## Performance Targets

| Operation | Target | Status |
|-----------|--------|--------|
| App Startup | < 3s | 🚧 |
| Recording Start | < 500ms | 🚧 |
| Transcription (1min) | < 2s | 🚧 |
| AI Enhancement (500w) | < 5s | 🚧 |
| Memory Usage | < 200MB | 🚧 |

## Architecture

```
src/
├── models/              # Data entities
├── services/           # Business logic
├── lib/               # Component libraries
│   ├── quill-audio/   # Audio recording
│   ├── quill-transcribe/  # whisper.cpp integration
│   ├── quill-enhance/ # Gemini API services
│   └── quill-storage/ # SQLite persistence
└── main.cpp           # Application entry

tests/
├── contract/          # Interface compliance tests
├── integration/       # End-to-end workflow tests
└── unit/             # Component unit tests
```

## Development Status

**Current Phase**: Setup Complete ✅

- [x] CMake project structure
- [x] Qt 6.5+ dependencies configured
- [x] whisper.cpp integration setup
- [x] Google Test framework
- [x] Qt Test framework
- [x] Code formatting and linting tools

**Next Phase**: Test-Driven Development

- [ ] Contract tests for all interfaces
- [ ] Integration tests for user workflows
- [ ] Core model implementations
- [ ] Service layer implementations

## Contributing

1. Follow TDD approach - write tests first
2. Use provided formatting tools: `./scripts/format-code.sh`
3. Run linting: `./scripts/lint-code.sh`
4. All tests must pass before commits
5. Follow Qt naming conventions and modern C++ practices

## License

[License information to be added]

---

**Branch**: `001-voice-to-text`  
**Last Updated**: 2024-12-19

