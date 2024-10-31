cmake_minimum_required(VERSION 3.15)
project(LiveImproved VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Options
option(BUILD_TESTS "Build the tests" OFF)

# Add subdirectories for external libraries
add_subdirectory(lib/yaml-cpp)
add_subdirectory(lib/harfbuzz)
add_subdirectory(lib/pugixml)
add_subdirectory(lib/doctest)

# Common libraries
add_library(common_libs INTERFACE)
target_link_libraries(common_libs
    INTERFACE
        yaml-cpp
        harfbuzz
        pugixml
)

# Common include directories
set(COMMON_INCLUDE_DIRS
    ${CMAKE_SOURCE_DIR}/src/abstract
    ${CMAKE_SOURCE_DIR}/src/include
    ${CMAKE_SOURCE_DIR}/src/include/config
    ${CMAKE_SOURCE_DIR}/src/include/core
    ${CMAKE_SOURCE_DIR}/src/include/event
    ${CMAKE_SOURCE_DIR}/src/include/exception
    ${CMAKE_SOURCE_DIR}/src/include/gui
    ${CMAKE_SOURCE_DIR}/src/include/ipc
    ${CMAKE_SOURCE_DIR}/src/include/system
)

target_include_directories(common_libs INTERFACE ${COMMON_INCLUDE_DIRS})

# Platform-specific settings
if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(CMAKE_OBJCXX_FLAGS "${CMAKE_OBJCXX_FLAGS} -ObjC++")
    target_include_directories(common_libs INTERFACE
        ${CMAKE_SOURCE_DIR}/src/include/platform/macos
        ${CMAKE_SOURCE_DIR}/src/include/platform/macos/event
        ${CMAKE_SOURCE_DIR}/src/include/platform/macos/gui
        ${CMAKE_SOURCE_DIR}/src/include/platform/macos/ipc
        ${CMAKE_SOURCE_DIR}/src/include/platform/macos/live
        ${CMAKE_SOURCE_DIR}/src/include/platform/macos/live/component
        ${CMAKE_SOURCE_DIR}/src/include/platform/macos/system
        ${CMAKE_SOURCE_DIR}/src/include/platform/macos/util
    )
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    target_include_directories(common_libs INTERFACE
        ${CMAKE_SOURCE_DIR}/src/include/platform/windows
        ${CMAKE_SOURCE_DIR}/src/include/platform/windows/event
        ${CMAKE_SOURCE_DIR}/src/include/platform/windows/gui
        ${CMAKE_SOURCE_DIR}/src/include/platform/windows/ipc
        ${CMAKE_SOURCE_DIR}/src/include/platform/windows/live
        ${CMAKE_SOURCE_DIR}/src/include/platform/windows/live/component
        ${CMAKE_SOURCE_DIR}/src/include/platform/windows/system
        ${CMAKE_SOURCE_DIR}/src/include/platform/windows/util
    )
endif()

# Source files
set(SRC_CPP
    src/Main.cpp
    src/core/ConfigManager.cpp
    src/core/ConfigMenu.cpp
    src/core/LogGlobal.cpp
    src/core/LogHandler.cpp
    src/core/PluginManager.cpp
    src/event/KeyMapper.cpp
    src/gui/Theme.cpp
    src/ipc/IPCQueue.cpp
    src/ipc/IPCPipe.cpp
    src/ipc/IPCRequestPipe.cpp
    src/ipc/IPCResponsePipe.cpp
    src/ipc/ResponseParser.cpp
)

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(SRC_OBJCXX
        src/platform/macos/event/EventHandler.mm
        src/platform/macos/event/KeySender.mm
        src/platform/macos/event/PlatformInitializer.mm
        src/platform/macos/gui/ContextMenu.mm
        src/platform/macos/live/AXAttribute.mm
        src/platform/macos/live/AXFinder.mm
        src/platform/macos/live/AXInteraction.mm
        src/platform/macos/live/AXPrinter.mm
        src/platform/macos/live/AXWindow.mm
        src/platform/macos/live/LiveInterface.mm
        src/platform/macos/live/component/AXCheckBox.mm
        src/platform/macos/system/PID.mm
        src/platform/macos/system/PathManager.mm
    )
    list(APPEND SRC_CPP src/platform/macos/ipc/PipeUtil.cpp)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    list(APPEND SRC_CPP
        src/platform/windows/event/KeySender.cpp
        src/platform/windows/ipc/PipeUtil.cpp
        src/platform/windows/system/PID.cpp
        src/platform/windows/system/PathManager.cpp
    )
endif()

# Main executable
add_executable(LiveImproved ${SRC_CPP} ${SRC_OBJCXX})
target_link_libraries(LiveImproved PRIVATE common_libs)
target_include_directories(LiveImproved PRIVATE ${COMMON_INCLUDE_DIRS})

# Platform-specific settings for main executable
if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    target_link_libraries(LiveImproved PRIVATE
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
    target_link_libraries(LiveImproved PRIVATE
        advapi32
        gdi32
        ole32
        shell32
        user32
    )
endif()

# Tests
if(BUILD_TESTS)
    enable_testing()
    add_compile_definitions(TEST_BUILD RUNNING_TESTS)

    # Function to add a test
    function(add_doctest_test TEST_PATH)
        get_filename_component(TEST_NAME ${TEST_PATH} NAME_WE)
        get_filename_component(TEST_DIR ${TEST_PATH} DIRECTORY)
        string(REPLACE "/" "_" TARGET_NAME "${TEST_DIR}_${TEST_NAME}")

        add_executable(${TARGET_NAME} ${TEST_PATH} ${SRC_CPP} ${SRC_OBJCXX})
        target_link_libraries(${TARGET_NAME} PRIVATE doctest::doctest common_libs)
        target_include_directories(${TARGET_NAME} PRIVATE 
            ${COMMON_INCLUDE_DIRS}
            ${CMAKE_SOURCE_DIR}/lib/doctest/doctest
            ${CMAKE_SOURCE_DIR}/mock
        )
        target_compile_definitions(${TARGET_NAME} PRIVATE TEST_BUILD)
        add_test(NAME ${TARGET_NAME} COMMAND ${TARGET_NAME} -s -o compact -ns)
    endfunction()

    # Add your tests
    add_doctest_test(test/core/test_ConfigManager.cpp)
    add_doctest_test(test/system/test_PathManager.cpp)

    # Custom target for building all tests
    add_custom_target(build_tests
        DEPENDS
            test_core_test_ConfigManager
            test_system_test_PathManager
        COMMENT "Building all tests"
    )

    # Custom target for running all tests
    add_custom_target(run_tests
        COMMAND ${CMAKE_COMMAND} -E echo "Running all tests..."
        COMMAND ${CMAKE_COMMAND}
            -DBUILD_DIR=${CMAKE_BINARY_DIR}
            -DSOURCE_DIR=${CMAKE_SOURCE_DIR}
            -P ${CMAKE_SOURCE_DIR}/run-tests.cmake
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Running all tests"
        DEPENDS build_tests
    )
endif()
