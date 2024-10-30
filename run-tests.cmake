cmake_minimum_required(VERSION 3.15)

message(STATUS "BUILD_DIR: ${BUILD_DIR}")
message(STATUS "SOURCE_DIR: ${SOURCE_DIR}")
message(STATUS "COMMON_INCLUDE_DIRS: ${COMMON_INCLUDE_DIRS}")
message(STATUS "CMAKE_SYSTEM_NAME: ${CMAKE_SYSTEM_NAME}")
message(STATUS "CMAKE_SYSTEM_PROCESSOR: ${CMAKE_SYSTEM_PROCESSOR}")

# Set up common libraries
set(COMMON_LIBS
    ${YAML_CPP_LIB}
    ${HARFBUZZ_LIB}
    pugixml
)

# Add platform-specific libraries
if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    list(APPEND COMMON_LIBS
        "-framework ApplicationServices"
        "-framework Cocoa"
        "-framework CoreFoundation"
        "-framework CoreGraphics"
        "-framework CoreImage"
        "-framework CoreVideo"
        "-framework IOKit"
        "-framework QuartzCore"
        "-framework Security"
    )
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    list(APPEND COMMON_LIBS
        advapi32
        gdi32
        ole32
        shell32
        user32
    )
endif()

# Find all test executables
file(GLOB_RECURSE TEST_EXECUTABLES "${BUILD_DIR}/test_*")

foreach(TEST_EXECUTABLE ${TEST_EXECUTABLES})
    get_filename_component(TEST_NAME ${TEST_EXECUTABLE} NAME_WE)
    message(STATUS "Running test: ${TEST_NAME}")
    
    # Set up the test target
    add_executable(${TEST_NAME} ${TEST_EXECUTABLE})
    
    # Add include directories
    target_include_directories(${TEST_NAME} PRIVATE ${COMMON_INCLUDE_DIRS})
    
    # Link libraries
    target_link_libraries(${TEST_NAME} PRIVATE ${COMMON_LIBS})
    
    # Add compile definitions
    target_compile_definitions(${TEST_NAME} PRIVATE TEST_BUILD RUNNING_TESTS)
    
    # Run the test
    execute_process(
        COMMAND ${TEST_EXECUTABLE} -s -o compact -ns
        RESULT_VARIABLE TEST_RESULT
        OUTPUT_VARIABLE TEST_OUTPUT
        ERROR_VARIABLE TEST_ERROR
    )
    
    if(TEST_RESULT)
        message(WARNING "Test ${TEST_NAME} failed")
        message(STATUS "Test output: ${TEST_OUTPUT}")
        message(STATUS "Test error: ${TEST_ERROR}")
    else()
        message(STATUS "Test ${TEST_NAME} passed")
    endif()
endforeach()
