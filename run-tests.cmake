cmake_minimum_required(VERSION 3.15)

message(STATUS "BUILD_DIR: ${BUILD_DIR}")
message(STATUS "SOURCE_DIR: ${SOURCE_DIR}")

# Find all test executables
file(GLOB_RECURSE TEST_EXECUTABLES "${BUILD_DIR}/test_*")

foreach(TEST_EXECUTABLE ${TEST_EXECUTABLES})
    get_filename_component(TEST_NAME ${TEST_EXECUTABLE} NAME_WE)
    message(STATUS "Running test: ${TEST_NAME}")
    
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
