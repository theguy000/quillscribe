cmake_minimum_required(VERSION 3.20)

project(QuillScribe VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable testing
enable_testing()

# Find Qt components (required for full build)
find_package(Qt6 REQUIRED COMPONENTS
    Core
    Widgets
    Multimedia
    Network
    Sql
    Test
)

if(Qt6_FOUND)
    message(STATUS "Qt6 found: ${Qt6_VERSION}")
    message(STATUS "Qt6 Core found: ${Qt6Core_VERSION}")
    message(STATUS "Qt6 installation directory: ${Qt6_DIR}")
    set(QT_AVAILABLE TRUE)
    add_compile_definitions(QT_AVAILABLE)
    
    # Ensure Qt libraries are properly linked on Windows
    if(WIN32)
        set_property(TARGET Qt6::Core PROPERTY INTERFACE_COMPILE_DEFINITIONS QT_CORE_LIB)
    endif()
else()
    message(FATAL_ERROR "Qt6 not found. Qt6 is required for QuillScribe.")
endif()

# Find Google Test (optional for basic build testing)
find_package(GTest QUIET)

if(GTest_FOUND)
    message(STATUS "Google Test found: ${GTest_VERSION}")
    set(GTEST_AVAILABLE TRUE)
else()
    message(WARNING "Google Test not found. Unit tests will be disabled.")
    message(STATUS "To install GTest: sudo apt install libgtest-dev")
    set(GTEST_AVAILABLE FALSE)
endif()

# Set Qt MOC, UIC, and RCC to run automatically (always when Qt is available)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Qt-specific compiler definitions for proper linking
if(WIN32)
    add_compile_definitions(
        QT_CORE_LIB
        QT_WIDGETS_LIB
        QT_MULTIMEDIA_LIB
        QT_NETWORK_LIB
        QT_SQL_LIB
    )
endif()

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_SOURCE_DIR}/specs/001-voice-to-text/contracts)

# Add external dependencies first
add_subdirectory(external)

# Add subdirectories
add_subdirectory(src)
add_subdirectory(tests)

# Make whisper.cpp available to the rest of the project
set(WHISPER_CPP_AVAILABLE TRUE CACHE BOOL "whisper.cpp is available")

# Compiler-specific options
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Debug/Release configurations
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
