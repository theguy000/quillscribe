# Source directory CMakeLists.txt

# Create directory structure
file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/models)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/services)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib/quill-audio)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib/quill-transcribe)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib/quill-enhance)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib/quill-storage)

# Collect all library source files
set(QUILLSCRIBE_LIB_SOURCES
    # Model files
    models/BaseModel.cpp
    models/Recording.cpp
    models/Transcription.cpp
    models/EnhancedText.cpp
    models/UserSession.cpp
    models/EnhancementProfile.cpp
    
    # Service files
    services/AudioRecorderService.cpp
    services/AudioLevelIODevice.cpp
    services/TranscriptionService.cpp
    services/TextEnhancementService.cpp
    services/StorageManager.cpp
    services/ConfigurationManager.cpp
    services/ErrorHandler.cpp
    
    # Interface implementations
    lib/quill-audio/AudioRecorderInterface.cpp
    lib/quill-transcribe/TranscriptionServiceInterface.cpp
    lib/quill-enhance/TextEnhancementServiceInterface.cpp
    lib/quill-storage/StorageInterface.cpp
    MainWindow.cpp
)

# Collect application source files
set(QUILLSCRIBE_APP_SOURCES
    main.cpp
)

# Collect all header files
set(QUILLSCRIBE_HEADERS
    MainWindow.h
    
    # Model headers
    models/BaseModel.h
    models/Recording.h
    models/Transcription.h
    models/EnhancedText.h
    models/UserSession.h
    models/EnhancementProfile.h
    
    # Service headers
    services/AudioRecorderService.h
    services/AudioLevelIODevice.h
    services/TranscriptionService.h
    services/TextEnhancementService.h
    services/StorageManager.h
    services/ConfigurationManager.h
    services/ErrorHandler.h

    # Contract headers
    ../specs/001-voice-to-text/contracts/ai-enhancement-interface.h
    ../specs/001-voice-to-text/contracts/audio-recording-interface.h
    ../specs/001-voice-to-text/contracts/storage-interface.h
    ../specs/001-voice-to-text/contracts/transcription-service-interface.h
)

# Only create executable if Qt is available
if(QT_AVAILABLE)
    # Core library
    add_library(quillscribe_lib STATIC ${QUILLSCRIBE_LIB_SOURCES})
    
    # Add headers to the library target for IDEs and structure
    target_sources(quillscribe_lib PUBLIC ${QUILLSCRIBE_HEADERS})

    # Link library to dependencies
    target_link_libraries(quillscribe_lib PUBLIC
        Qt6::Core
        Qt6::Widgets
        Qt6::Multimedia
        Qt6::Network
        Qt6::Sql
    )
    
    # Link whisper.cpp if available
    if(TARGET whisper)
        target_link_libraries(quillscribe_lib PUBLIC whisper)
    endif()

    # Main executable
    add_executable(quillscribe ${QUILLSCRIBE_APP_SOURCES})
    
    # Link executable to our library
    target_link_libraries(quillscribe PRIVATE quillscribe_lib)
    
    # Set executable properties
    set_target_properties(quillscribe PROPERTIES
        WIN32_EXECUTABLE TRUE
        MACOSX_BUNDLE TRUE
    )
    
    message(STATUS "QuillScribe executable configured successfully")
else()
    message(WARNING "Skipping QuillScribe executable - Qt6 not available")
endif()

# Library subdirectories (structure ready for T021-T024)
# add_subdirectory(lib/quill-audio)
# add_subdirectory(lib/quill-transcribe)
# add_subdirectory(lib/quill-enhance)
# add_subdirectory(lib/quill-storage)
# add_subdirectory(lib/quill-storage)