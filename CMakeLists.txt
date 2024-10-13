cmake_minimum_required(VERSION 3.30)
set(BUILD_TESTING OFF)
set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED "NO")
set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "")
set(CMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "")

# Set BUILD_TESTS option with a default value of OFF
option(BUILD_TESTS "Build the tests" OFF)

# Set project name and version
project(LiveImproved VERSION 1.0 LANGUAGES CXX)

add_definitions(-DDEBUG)
set(CMAKE_BUILD_TYPE Debug)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Custom target for running clang-tidy
if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    # IGNORE windows
    set(PLATFORM_IGNORE_TIDY "windows")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    # IGNORE macos
    set(PLATFORM_IGNORE_TIDY "macos")
endif()

add_custom_target(
    tidy
    COMMAND ${CMAKE_COMMAND} -E echo "Running clang-tidy..."
    COMMAND ${CMAKE_COMMAND}
        -DCLANG_TIDY_EXE=${CLANG_TIDY_EXE}
        -DBUILD_DIR=${CMAKE_BINARY_DIR}
        -DSOURCE_DIR=${CMAKE_SOURCE_DIR}
        -DHOME=$ENV{HOME}
        -P ${CMAKE_SOURCE_DIR}/run-clang-tidy.cmake
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running clang-tidy on source files"
)
# Define CMAKE_CXX_CLANG_TIDY target
find_program(CLANG_TIDY_EXE NAMES "clang-tidy")
if(CLANG_TIDY_EXE)
    # sets globally
    #    set(CMAKE_CXX_CLANG_TIDY
    set(CLANG_TIDY_COMMAND
        ${CLANG_TIDY_EXE}
        "-p=${CMAKE_BINARY_DIR}"
        "--header-filter=.*\\.(cpp|h)$"
        "--checks=-*,cppcoreguidelines-*,modernize-*"
    )
    set_target_properties(tidy PROPERTIES COMMAND "${CLANG_TIDY_COMMAND}")
else()
    message(WARNING "clang-tidy not found. Static analysis will be skipped.")
endif()
#set(CMAKE_CXX_CLANG_TIDY "")

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
if(NOT DEFINED BUILD_TESTS OR NOT BUILD_TESTS)
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

    # Copy additional files (like LICENSE) to the bundle
    if(NOT DEFINED BUILD_TESTS OR NOT BUILD_TESTS)
        add_custom_command(TARGET LiveImproved POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_SOURCE_DIR}/LICENSE
            $<TARGET_BUNDLE_DIR:LiveImproved>/Contents/LICENSE
        )
    endif()
else()
    # For tests, create a dummy executable target instead of an interface library
    add_executable(LiveImproved src/Main.cpp)
    target_link_libraries(LiveImproved PRIVATE common_libs)
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
    src/ipc/IPCResilienceDecorator.cpp
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
        src/platform/macos/system/PathFinder.mm
        src/platform/macos/system/PID.mm
    )
    list(APPEND SRC_CPP
        src/platform/macos/ipc/IPCCore.cpp
    )

elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    list(APPEND SRC_CPP
        src/platform/windows/event/EventHandler.cpp
        src/platform/windows/event/KeySender.cpp
        src/platform/windows/gui/ContextMenu.cpp
        src/platform/windows/ipc/IPCCore.cpp
        src/platform/windows/live/LiveInterface.cpp
        src/platform/windows/system/PID.cpp
    )
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
	# Add the C++ and Objective-C++ files to the target
	target_sources(LiveImproved
	    PRIVATE
		${SRC_CPP}
		${SRC_OBJCXX}
	)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
	target_sources(LiveImproved
	    PRIVATE
		${SRC_CPP}
	)
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    # Set the library paths based on the architecture
    if(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
        link_directories(/usr/local/Cellar/yaml-cpp/0.8.0/lib)
        link_directories(/usr/local/Cellar/harfbuzz/9.0.0/lib)
        set(HARFBUZZ_LIB /usr/local/Cellar/harfbuzz/9.0.0/lib/libharfbuzz.a)
        set(YAML_CPP_LIB /usr/local/Cellar/yaml-cpp/0.8.0/lib/libyaml-cpp.dylib)
    elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64")
        link_directories(/opt/homebrew/Cellar/yaml-cpp/0.8.0/lib)
        link_directories(/opt/homebrew/Cellar/harfbuzz/9.0.0/lib)
        set(HARFBUZZ_LIB /opt/homebrew/Cellar/harfbuzz/9.0.0/lib/libharfbuzz.a)
        set(YAML_CPP_LIB /opt/homebrew/Cellar/yaml-cpp/0.8.0/lib/libyaml-cpp.dylib)
    endif()

    # Link frameworks and libraries
    target_link_libraries(LiveImproved
        PRIVATE
        ${HARFBUZZ_LIB}
        ${YAML_CPP_LIB}
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
    if(NOT DEFINED BUILD_TESTS OR NOT BUILD_TESTS)
        add_custom_command(TARGET LiveImproved POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_SOURCE_DIR}/LICENSE
            $<TARGET_BUNDLE_DIR:LiveImproved>/Contents/LICENSE
        )
    endif()
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    target_link_libraries(LiveImproved
        PRIVATE
            user32.lib
            gdi32.lib
            shell32.lib
            ole32.lib
            advapi32.lib
    )

    # No bundle generation on Windows, so we just generate an .exe
    set_target_properties(LiveImproved PROPERTIES
        OUTPUT_NAME "LiveImproved"
    )

    # Windows-specific install rules
    install(TARGETS LiveImproved
        RUNTIME DESTINATION bin
    )
endif()

# add test target
enable_testing()
add_subdirectory(lib/doctest)

set(DOCTEST_CONFIG_SUPER_QUIET_FAILURE_REPORTING OFF)

if(BUILD_TESTS)
    add_definitions(-DTEST_BUILD)
    add_compile_definitions(RUNNING_TESTS)
endif()

# Function to add a test
function(add_doctest_test TEST_PATH)
    get_filename_component(TEST_NAME ${TEST_PATH} NAME_WE)
    get_filename_component(TEST_DIR ${TEST_PATH} DIRECTORY)
    string(REPLACE "/" "_" TARGET_NAME "${TEST_DIR}_${TEST_NAME}")

    add_executable(${TARGET_NAME} ${TEST_PATH} src/core/ConfigManager.cpp src/event/KeyMapper.cpp mock/MockLogHandler.cpp)
    target_link_libraries(${TARGET_NAME}
        PRIVATE
            doctest::doctest
            common_libs
    )
    target_include_directories(${TARGET_NAME}
        PRIVATE
            ${CMAKE_SOURCE_DIR}/lib/doctest/doctest
            ${COMMON_INCLUDE_DIRS}
            ${CMAKE_SOURCE_DIR}/abstract
            ${CMAKE_SOURCE_DIR}/src/include
            ${CMAKE_SOURCE_DIR}/src/include/event
            ${CMAKE_SOURCE_DIR}/src/include/core
            ${CMAKE_SOURCE_DIR}/mock
    )
    target_compile_definitions(${TARGET_NAME}
        PRIVATE
            JUCE_APP_CONFIG_HEADER=""
            TEST_BUILD
    )
    add_test(NAME ${TARGET_NAME} COMMAND ${TARGET_NAME} -s -o compact -ns)
endfunction()

# Add your tests
add_doctest_test(test/core/test_ConfigManager.cpp)
#add_doctest_test(test/test_ActionHandler.cpp)
# Add other tests as needed

# Add a custom target for building all tests
add_custom_target(build_tests
    DEPENDS
        test_core_test_ConfigManager
        #        test_test_ActionHandler
    COMMENT "Building all tests"
)
