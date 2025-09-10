# Tests directory CMakeLists.txt

# Create test directory structure
file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/contract)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/integration)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/unit)

# Google Test setup (conditional)
if(GTEST_AVAILABLE)
    include(GoogleTest)
    message(STATUS "Google Test enabled for unit testing")
else()
    message(STATUS "Google Test disabled - install libgtest-dev to enable")
endif()

# Qt Test setup for Qt-specific tests (conditional)
if(QT_AVAILABLE)
    message(STATUS "Qt Test framework enabled")
else()
    message(STATUS "Qt Test framework disabled - install Qt6 to enable")
endif()

# Enable CTest integration
include(CTest)

# Common test files
set(COMMON_TEST_FILES
    test_main.cpp
    test_helpers.h
)

# Function to create test executable (conditional based on available dependencies)
function(create_test_executable test_name)
    set(test_sources ${ARGN})
    if(NOT GTEST_AVAILABLE)
        message(STATUS "Skipping test ${test_name} - Google Test not available")
        return()
    endif()
    
    add_executable(${test_name} ${test_sources} ${COMMON_TEST_FILES})
    
    # Include test helpers
    target_include_directories(${test_name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    
    # Link with available libraries
    target_link_libraries(${test_name} GTest::GTest)
    
    if(QT_AVAILABLE)
        target_link_libraries(${test_name}
            quillscribe_lib
            Qt6::Core
            Qt6::Test
            Qt6::Widgets
            Qt6::Multimedia
            Qt6::Network
            Qt6::Sql
        )
    endif()
    
    # Add to test suite
    gtest_discover_tests(${test_name})
    
    # Set test properties
    set_target_properties(${test_name} PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
    )
endfunction()

# Test executables will be created in T007-T015, T031-T033
create_test_executable(contract_tests
    contract/test_audio_recorder_contract.cpp
)

# Custom test target for running all tests
add_custom_target(run_all_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --verbose
    COMMENT "Running all QuillScribe tests"
)