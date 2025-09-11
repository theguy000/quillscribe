# External dependencies CMakeLists.txt

include(FetchContent)

# whisper.cpp integration
FetchContent_Declare(
    whisper
    GIT_REPOSITORY https://github.com/ggerganov/whisper.cpp.git
    GIT_TAG        v1.5.4  # Use stable release
    GIT_SHALLOW    TRUE
)

# Configure whisper.cpp build options
set(WHISPER_BUILD_TESTS OFF CACHE BOOL "Build whisper.cpp tests")
set(WHISPER_BUILD_EXAMPLES OFF CACHE BOOL "Build whisper.cpp examples") 
set(WHISPER_BUILD_SERVER OFF CACHE BOOL "Build whisper.cpp server")

# Force whisper.cpp to build as static library (resolves Windows DLL deployment issues)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries" FORCE)
set(WHISPER_BUILD_SHARED OFF CACHE BOOL "Build whisper as shared library" FORCE)

# Enable CUDA support if available (optional)
option(WHISPER_CUDA "Enable CUDA support for whisper.cpp" OFF)
if(WHISPER_CUDA)
    set(WHISPER_CUDA ON CACHE BOOL "Enable CUDA")
endif()

# Enable OpenMP for better performance (optional)
option(WHISPER_OPENMP "Enable OpenMP support for whisper.cpp" ON)
if(WHISPER_OPENMP)
    set(WHISPER_OPENMP ON CACHE BOOL "Enable OpenMP")
endif()

# Fetch and build whisper.cpp
FetchContent_MakeAvailable(whisper)

# Create interface library for easier linking
add_library(WhisperCpp INTERFACE)
target_link_libraries(WhisperCpp INTERFACE whisper)
target_include_directories(WhisperCpp INTERFACE ${whisper_SOURCE_DIR})

# Export target for main project
set(WHISPER_LIBRARIES WhisperCpp PARENT_SCOPE)
set(WHISPER_INCLUDE_DIRS ${whisper_SOURCE_DIR} PARENT_SCOPE)

