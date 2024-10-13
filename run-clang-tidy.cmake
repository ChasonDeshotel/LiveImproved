cmake_minimum_required(VERSION 3.30)

message(STATUS "CLANG_TIDY_EXE: ${CLANG_TIDY_EXE}")
message(STATUS "BUILD_DIR: ${BUILD_DIR}")
message(STATUS "SOURCE_DIR: ${SOURCE_DIR}")
message(STATUS "HOME: ${HOME}")

file(GLOB_RECURSE ALL_SOURCE_FILES
    ${SOURCE_DIR}/src/*.cpp
    ${SOURCE_DIR}/src/*.h
    ${SOURCE_DIR}/src/*.mm
)

list(FILTER ALL_SOURCE_FILES EXCLUDE REGEX ".*/lib/.*")
list(FILTER ALL_SOURCE_FILES EXCLUDE REGEX ".*/src/include/juce/.*")

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    list(FILTER ALL_SOURCE_FILES EXCLUDE REGEX ".*/windows/.*")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    list(FILTER ALL_SOURCE_FILES EXCLUDE REGEX ".*/macos/.*")
endif()

foreach(SOURCE_FILE ${ALL_SOURCE_FILES})
    execute_process(
        COMMAND "${CLANG_TIDY_EXE}"
            "-p=${BUILD_DIR}"
            "--header-filter=.*(?<!lib/).*"
            "--checks=-*,cppcoreguidelines-*,modernize-*"
            "--warnings-as-errors=*"
            "${SOURCE_FILE}"
        RESULT_VARIABLE CLANG_TIDY_RESULT
        OUTPUT_VARIABLE CLANG_TIDY_OUTPUT
        ERROR_VARIABLE CLANG_TIDY_ERROR
    )
    if(CLANG_TIDY_RESULT)
        message(WARNING "clang-tidy failed for ${SOURCE_FILE}")
        message(STATUS "clang-tidy output: ${CLANG_TIDY_OUTPUT}")
        message(STATUS "clang-tidy error: ${CLANG_TIDY_ERROR}")
    else()
        message(STATUS "clang-tidy succeeded for ${SOURCE_FILE}")
    endif()
endforeach()
