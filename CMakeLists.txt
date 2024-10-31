cmake_minimum_required(VERSION 3.15)

# Set Ninja as the preferred generator
set(CMAKE_GENERATOR "Ninja" CACHE INTERNAL "" FORCE)
set(BUILD_TESTING OFF)
set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED "NO")
set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "")
set(CMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "")

option(BUILD_TESTS "Build the tests" OFF)

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    add_definitions(-DWIN32_LEAN_AND_MEAN)
    add_definitions(-DNOMINMAX)
    add_definitions(-D_WIN32_WINNT=0x0A00)  # Target Windows 10
endif()

# if(WIN32) ?
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    if(POLICY CMP0126)
        cmake_policy(SET CMP0126 NEW)
    endif()
    set(BUILD_SHARED_LIBS ON)
    set(CMAKE_CXX_COMPILER clang++)
    set(COMPILER clang)

    add_compile_options(/Z7) # debug information format to C7

    add_definitions(-DWIN32_LEAN_AND_MEAN -DNOMINMAX -D_WIN32_WINNT=0x0A00)

    #    find_package(WindowsSDK REQUIRED)
    include(FindWindowsSDK)
        if(WINDOWSSDK_FOUND)
            message(STATUS "Windows SDK found: ${WINDOWSSDK_LATEST_DIR}")
            include_directories(${WINDOWSSDK_INCLUDE_DIRS})
        else()
            message(FATAL_ERROR "Windows SDK not found")
        endif()

     # Add Windows-specific compile definitions
     add_definitions(-DWIN32_LEAN_AND_MEAN -DNOMINMAX -D_WIN32_WINNT=0x0A00)

endif()

if (CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
    set(CMAKE_GENERATOR_PLATFORM x64)
endif()

# Set BUILD_TESTS option with a default value of OFF
option(BUILD_TESTS "Build the tests" OFF)

# Set project name and version
project(LiveImproved VERSION 1.0 LANGUAGES CXX)

add_definitions(-DDEBUG)
set(CMAKE_BUILD_TYPE Debug)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Custom target for running clang-tidy
find_program(CLANG_TIDY_EXE NAMES "clang-tidy")
if(CLANG_TIDY_EXE)
    add_custom_target(
        tidy
        COMMAND ${CMAKE_COMMAND} -E echo "Running clang-tidy..."
        COMMAND ${CMAKE_COMMAND}
            -DCLANG_TIDY_EXE=${CLANG_TIDY_EXE}
            -DBUILD_DIR=${CMAKE_BINARY_DIR}
            -DSOURCE_DIR=${CMAKE_SOURCE_DIR}
            -DHOME=$ENV{HOME}
            -DCMAKE_SYSTEM_NAME=${CMAKE_SYSTEM_NAME}
            -P ${CMAKE_SOURCE_DIR}/run-clang-tidy.cmake
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Running clang-tidy on source files"
    )
else()
    message(WARNING "clang-tidy not found. Static analysis will be skipped.")
endif()

# Set the C++ standard and Objective-C++ flag
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
	set(CMAKE_OBJCXX_FLAGS "${CMAKE_OBJCXX_FLAGS} -ObjC++")
endif()

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Add non-JUCE subdirectories
add_subdirectory(lib/yaml-cpp)
add_subdirectory(lib/harfbuzz)
add_subdirectory(lib/pugixml)

set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} "${CMAKE_SOURCE_DIR}/lib/yaml-cpp")
set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} "${CMAKE_SOURCE_DIR}/lib/harfbuzz")

# Always create the common_libs target
add_library(common_libs INTERFACE)
target_link_libraries(common_libs
    INTERFACE
        yaml-cpp
        harfbuzz
        pugixml
)

# Conditionally include JUCE and set up the main application
add_subdirectory(lib/JUCE)

# Set up your project with JUCE modules
juce_add_gui_app(LiveImproved
    PRODUCT_NAME "LiveImproved"
)

# Add the necessary JUCE modules
target_link_libraries(LiveImproved
    PRIVATE
        juce::juce_core
        juce::juce_gui_basics
        juce::juce_graphics
        juce::juce_events
        juce::juce_data_structures
        common_libs
)

# JUCE-specific include directories (only for LiveImproved target)
target_include_directories(LiveImproved PRIVATE
    ${CMAKE_SOURCE_DIR}/src/include/juce
    ${CMAKE_SOURCE_DIR}/lib/juce/modules
)

# JUCE-specific compile definitions
target_compile_definitions(LiveImproved PRIVATE
    JUCE_APP_CONFIG_HEADER="${CMAKE_SOURCE_DIR}/src/include/juce/AppConfig.h"
)

# Install rules for macOS application bundle
install(TARGETS LiveImproved
    BUNDLE DESTINATION /Applications
    RUNTIME DESTINATION bin
)

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    # Copy additional files (like LICENSE) to the bundle
    if(NOT DEFINED BUILD_TESTS OR NOT BUILD_TESTS)
        add_custom_command(TARGET LiveImproved POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_SOURCE_DIR}/LICENSE
            $<TARGET_BUNDLE_DIR:LiveImproved>/Contents/LICENSE
        )
    endif()
endif()

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
    ${CMAKE_SOURCE_DIR}/src/include/juce
    ${CMAKE_SOURCE_DIR}/src/include/system
)

target_include_directories(common_libs INTERFACE ${COMMON_INCLUDE_DIRS})

if(NOT DEFINED BUILD_TESTS OR NOT BUILD_TESTS)
    # JUCE-specific include directories (only for LiveImproved target when not building tests)
    target_include_directories(LiveImproved PRIVATE
        ${CMAKE_SOURCE_DIR}/src/include/juce
        ${CMAKE_SOURCE_DIR}/lib/juce/modules
        ${COMMON_INCLUDE_DIRS}
    )

    target_compile_definitions(LiveImproved PRIVATE JUCE_APP_CONFIG_HEADER="${CMAKE_SOURCE_DIR}/src/include/juce/AppConfig.h")
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    message(STATUS "Including macOS-specific directories")
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
    message(STATUS "Including Windows-specific directories")
    target_include_directories(common_libs INTERFACE
        ${CMAKE_SOURCE_DIR}/src/include/platform/windows
        ${CMAKE_SOURCE_DIR}/src/include/platform/windows/event
        ${CMAKE_SOURCE_DIR}/src/include/platform/windows/gui
        ${CMAKE_SOURCE_DIR}/src/include/platform/windows/ipc
        ${CMAKE_SOURCE_DIR}/src/include/platform/windows/live
        ${CMAKE_SOURCE_DIR}/src/include/platform/windows/live/component
        ${CMAKE_SOURCE_DIR}/src/include/platform/windows/system
        ${CMAKE_SOURCE_DIR}/src/include/platform/windows/util
        #        "$ENV{PROGRAMFILES\(X86\)}/Windows Kits/10/Include/10.0.19041.0/um"
        ${WINDOWSSDK_INCLUDE_DIRS}
    )
endif()

# Source files for C++ and Objective-C++
set(SRC_CPP
    src/Main.cpp
    src/core/ConfigManager.cpp
    src/core/ConfigMenu.cpp
    src/core/LogGlobal.cpp
    src/core/LogHandler.cpp
    src/core/PluginManager.cpp
    src/event/ActionHandler.cpp
    src/event/KeyMapper.cpp
    src/gui/SearchBox.cpp
    src/gui/Theme.cpp
    src/gui/WindowManager.cpp
    src/ipc/IPCQueue.cpp
    src/ipc/IPCPipe.cpp
    src/ipc/IPCRequestPipe.cpp
    src/ipc/IPCResponsePipe.cpp
    #src/ipc/IPCResilienceDecorator.cpp
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
    list(APPEND SRC_CPP
        src/platform/macos/ipc/PipeUtil.cpp
    )

elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    list(APPEND SRC_CPP
        src/platform/windows/event/EventHandler.cpp
        src/platform/windows/event/KeySender.cpp
        src/platform/windows/gui/ContextMenu.cpp
        src/platform/windows/ipc/PipeUtil.cpp
        src/platform/windows/live/LiveInterface.cpp
        src/platform/windows/system/PID.cpp
        src/platform/windows/system/PathManager.cpp
    )
endif()

# Create a library with all your main source files
add_library(LiveImprovedLib STATIC
    ${SRC_CPP}
    ${SRC_OBJCXX}
)

target_include_directories(LiveImprovedLib PUBLIC
    ${COMMON_INCLUDE_DIRS}
)

target_link_libraries(LiveImprovedLib
    PUBLIC
        common_libs
        juce::juce_core
        juce::juce_gui_basics
        juce::juce_graphics
        juce::juce_events
        juce::juce_data_structures
)

# Add platform-specific libraries
if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    target_link_libraries(LiveImprovedLib PUBLIC
        ${HARFBUZZ_LIB}
        ${YAML_CPP_LIB}
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
    target_link_libraries(LiveImprovedLib PUBLIC
        advapi32
        gdi32
        ole32
        shell32
        user32
    )
endif()

# Main application
target_link_libraries(LiveImproved PRIVATE LiveImprovedLib)

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    # Set the library paths based on the architecture
    if(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
        link_directories(/usr/local/Cellar/harfbuzz/10.0.1_1/lib)
        link_directories(/usr/local/Cellar/yaml-cpp/0.8.0/lib)
        set(HARFBUZZ_LIB /usr/local/Cellar/harfbuzz/10.0.1_1/lib/libharfbuzz.a)
        set(YAML_CPP_LIB /usr/local/Cellar/yaml-cpp/0.8.0/lib/libyaml-cpp.dylib)
    elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64")
        link_directories(/opt/homebrew/Cellar/harfbuzz/10.0.1_1/lib)
        link_directories(/opt/homebrew/Cellar/yaml-cpp/0.8.0/lib)
        set(HARFBUZZ_LIB /opt/homebrew/Cellar/harfbuzz/10.0.1_1/lib/libharfbuzz.a)
        set(YAML_CPP_LIB /opt/homebrew/Cellar/yaml-cpp/0.8.0/lib/libyaml-cpp.dylib)
    endif()

    target_link_libraries(LiveImproved
        PRIVATE
        ${HARFBUZZ_LIB}
        ${YAML_CPP_LIB}
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
    if(NOT DEFINED BUILD_TESTS OR NOT BUILD_TESTS)
        set_target_properties(LiveImproved PROPERTIES
            MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/src/Info.plist
        )
        install(TARGETS LiveImproved
            BUNDLE DESTINATION /Applications
            RUNTIME DESTINATION bin
        )
        add_custom_command(TARGET LiveImproved POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_SOURCE_DIR}/LICENSE
            $<TARGET_BUNDLE_DIR:LiveImproved>/Contents/LICENSE
        )

        # Install rules for macOS application bundle
	#install(TARGETS LiveImproved
	#    BUNDLE DESTINATION /Applications
	#    RUNTIME DESTINATION bin
	#)

        # Copy additional files (like LICENSE) to the bundle
        if(NOT DEFINED BUILD_TESTS OR NOT BUILD_TESTS)
            add_custom_command(TARGET LiveImproved POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy
                ${CMAKE_SOURCE_DIR}/LICENSE
                $<TARGET_BUNDLE_DIR:LiveImproved>/Contents/LICENSE
            )
        endif()
    endif()

    if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
        install(TARGETS LiveImproved
        BUNDLE DESTINATION ${CMAKE_INSTALL_PREFIX}/Applications
        RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
    )

    endif()

elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    target_link_libraries(LiveImproved
        PRIVATE
            advapi32
            gdi32
            ole32
            shell32
            user32
    )

    set_target_properties(LiveImproved PROPERTIES
        OUTPUT_NAME "LiveImproved"
    )

    install(TARGETS LiveImproved
        RUNTIME DESTINATION bin
    )
endif()

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(lib/doctest)

    # Function to add a test
    function(add_li_test TEST_NAME TEST_SOURCE)
        add_executable(${TEST_NAME} ${TEST_SOURCE})
        target_link_libraries(${TEST_NAME} PRIVATE
            LiveImprovedLib
            doctest::doctest
        )
        target_include_directories(${TEST_NAME} PRIVATE
            ${CMAKE_SOURCE_DIR}/src/abstract
            ${CMAKE_SOURCE_DIR}/lib/doctest/doctest
            ${CMAKE_SOURCE_DIR}/mock
        )
        target_compile_definitions(${TEST_NAME} PRIVATE
            TEST_BUILD
        )
        add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
    endfunction()

    # Add your tests
    add_li_test(test_ConfigManager test/core/test_ConfigManager.cpp)
    add_li_test(test_ActionHandler test/core/test_ActionHandler.cpp)
    # Add other tests as needed
endif()

