cmake_minimum_required(VERSION 3.16)

# Set project name and version
project(LiveImproved VERSION 1.0 LANGUAGES CXX)

add_definitions(-DDEBUG)

# Set the C++ standard and Objective-C++ flag
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_OBJCXX_FLAGS "${CMAKE_OBJCXX_FLAGS} -ObjC++")

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Add JUCE using the CMake API
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
)

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/src/include/juce
    ${CMAKE_SOURCE_DIR}/lib/juce/modules
    ${CMAKE_SOURCE_DIR}/src/include
    ${CMAKE_SOURCE_DIR}/src/include/platform/macos
    /usr/local/include
    /usr/local/Cellar/harfbuzz/9.0.0/include/harfbuzz
    /usr/local/Cellar/yaml-cpp/0.8.0/include
)

target_compile_definitions(LiveImproved PRIVATE JUCE_APP_CONFIG_HEADER="${CMAKE_SOURCE_DIR}/src/include/juce/AppConfig.h")

# Source files for C++ and Objective-C++
set(SRC_CPP
    src/Main.cpp
    src/ApplicationManager.cpp
    src/LogHandler.cpp
    src/WindowManager.cpp
    src/KeyMapper.cpp
    src/config/ConfigManager.cpp
    src/config/ConfigMenu.cpp
    src/gui/SearchBox.cpp
    src/ActionHandler.cpp
    src/ResponseParser.cpp
    src/platform/macos/IPC.cpp
)

set(SRC_OBJCXX
    src/platform/macos/PlatformInitializer.mm
    src/platform/macos/ContextMenu.mm
    src/platform/macos/PID.mm
    src/platform/macos/EventHandler.mm
    src/platform/macos/KeySender.mm
)

# Add the C++ and Objective-C++ files to the target
target_sources(LiveImproved
    PRIVATE
        ${SRC_CPP}
        ${SRC_OBJCXX}
)

link_directories(/usr/local/Cellar/yaml-cpp/0.8.0/lib)

# Link frameworks (macOS specific)
target_link_libraries(LiveImproved
    PRIVATE
        /usr/local/Cellar/harfbuzz/9.0.0/lib/libharfbuzz.a
        /usr/local/Cellar/yaml-cpp/0.8.0/lib/libyaml-cpp.dylib
        "-framework Cocoa"
        "-framework CoreFoundation"
        "-framework CoreGraphics"
        "-framework ApplicationServices"
        "-framework CoreVideo"
        "-framework IOKit"
        "-framework CoreImage"
        "-framework Security"
        "-framework QuartzCore"
)

set_target_properties(LiveImproved PROPERTIES
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/src/Info.plist
)
# Install rules for macOS application bundle
install(TARGETS LiveImproved
    BUNDLE DESTINATION /Applications
    RUNTIME DESTINATION bin
)

# Copy additional files (like LICENSE) to the bundle
add_custom_command(TARGET LiveImproved POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    ${CMAKE_SOURCE_DIR}/LICENSE
    $<TARGET_BUNDLE_DIR:LiveImproved>/Contents/LICENSE
)

# Testing
# suppress deprecated warning
if (POLICY CMP0167)
    cmake_policy(SET CMP0167 OLD)
endif()
find_package(Boost 1.86.0 REQUIRED COMPONENTS unit_test_framework)

add_executable(test_KeyMapper ${CMAKE_SOURCE_DIR}/test/test_KeyMapper.cpp)
target_link_libraries(test_KeyMapper Boost::unit_test_framework)

add_executable(
    test_config_ConfigManager
    ${CMAKE_SOURCE_DIR}/mock/MockLogHandler.cpp
    ${CMAKE_SOURCE_DIR}/src/KeyMapper.cpp
    ${CMAKE_SOURCE_DIR}/src/config/ConfigManager.cpp
    ${CMAKE_SOURCE_DIR}/test/test_config_ConfigManager.cpp
)
target_link_libraries(test_config_ConfigManager
    Boost::unit_test_framework
    /usr/local/Cellar/yaml-cpp/0.8.0/lib/libyaml-cpp.dylib
)

enable_testing()

add_test(NAME test_KeyMapper COMMAND ${CMAKE_BINARY_DIR}/bin/test_KeyMapper)
add_test(NAME test_config_ConfigManager COMMAND ${CMAKE_BINARY_DIR}/bin/test_config_ConfigManager)
