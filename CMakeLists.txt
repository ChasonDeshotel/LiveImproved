cmake_minimum_required(VERSION 4.0)
# Set project name and version
project(LiveImproved VERSION 1.0)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15") # Match what Live was probably built against
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++ -D_LIBCPP_ABI_VERSION=2")

set(BUILD_TESTING OFF)
set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED "NO")
set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "")
set(CMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "")

if(MSVC)
    if(POLICY CMP0126)
        cmake_policy(SET CMP0126 NEW)
    endif()
    set(BUILD_SHARED_LIBS ON)
    #set(CMAKE_CXX_COMPILER clang++)
    #set(COMPILER clang)
    add_compile_options(/Z7) # debug information format to C7
    add_definitions(-DWIN32_LEAN_AND_MEAN -DNOMINMAX -D_WIN32_WINNT=0x0A00)
    find_package(WindowsSDK REQUIRED)
    include_directories(${WindowsSDK_INCLUDE_DIRS})
endif()

option(BUILD_TESTS "Build the tests" OFF)

add_definitions(-DDEBUG)
set(CMAKE_BUILD_TYPE Debug)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
	set(CMAKE_OBJCXX_FLAGS "${CMAKE_OBJCXX_FLAGS} -ObjC++")
endif()

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

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
    target_compile_definitions(LiveImproved PRIVATE FMT_HEADER_ONLY)

else()
    # For tests, create a dummy executable target instead of an interface library
    add_executable(LiveImproved src/Main.cpp)
endif()

# Common include directories
set(COMMON_INCLUDE_DIRS
    ${CMAKE_SOURCE_DIR}/src/abstract
    ${CMAKE_SOURCE_DIR}/src/include
    ${CMAKE_SOURCE_DIR}/src/include/config
    ${CMAKE_SOURCE_DIR}/src/include/core
    ${CMAKE_SOURCE_DIR}/src/include/core/sinks
    ${CMAKE_SOURCE_DIR}/src/include/event
    ${CMAKE_SOURCE_DIR}/src/include/exception
    ${CMAKE_SOURCE_DIR}/src/include/gui
    ${CMAKE_SOURCE_DIR}/src/include/ipc
    ${CMAKE_SOURCE_DIR}/src/include/system

    ${CMAKE_SOURCE_DIR}/src/platform/macos/system

    ${CMAKE_SOURCE_DIR}/lib/fmt/include
)

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
    target_include_directories(LiveImproved PRIVATE
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
    target_include_directories(LiveImproved PRIVATE
        ${CMAKE_SOURCE_DIR}/src/include/platform/windows
        ${CMAKE_SOURCE_DIR}/src/include/platform/windows/event
        ${CMAKE_SOURCE_DIR}/src/include/platform/windows/gui
        ${CMAKE_SOURCE_DIR}/src/include/platform/windows/ipc
        ${CMAKE_SOURCE_DIR}/src/include/platform/windows/live
        ${CMAKE_SOURCE_DIR}/src/include/platform/windows/live/component
        ${CMAKE_SOURCE_DIR}/src/include/platform/windows/system
        ${CMAKE_SOURCE_DIR}/src/include/platform/windows/util
        "$ENV{PROGRAMFILES\(X86\)}/Windows Kits/10/Include/10.0.19041.0/um"
    )
endif()

set(SRC_CPP
    src/Main.cpp
    src/core/ConfigManager.cpp
    src/core/ConfigMenu.cpp
    src/core/LogGlobal.cpp
    src/core/PluginManager.cpp
    src/event/ActionHandler.cpp
    src/event/KeyMapper.cpp
    src/gui/SearchBox.cpp
    src/gui/Theme.cpp
    src/gui/WindowManager.cpp
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
        src/platform/macos/system/PathFinder.mm
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
        src/platform/windows/system/PathFinder.cpp
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
    set(CMAKE_PREFIX_PATH "/opt/homebrew")
    find_package(PkgConfig REQUIRED)
    find_package(harfbuzz REQUIRED)
    find_package(pugixml REQUIRED)
    find_package(yaml-cpp REQUIRED)

    target_link_libraries(LiveImproved
        PRIVATE
        harfbuzz::harfbuzz
        pugixml::pugixml
        yaml-cpp::yaml-cpp
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
            XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.example.LiveImproved"
        )
        install(FILES
            ${CMAKE_SOURCE_DIR}/build/xcode/LiveImproved_artefacts/Debug/LiveImproved.app
            DESTINATION /Applications
        )
        add_custom_command(TARGET LiveImproved POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_SOURCE_DIR}/LICENSE
            $<TARGET_BUNDLE_DIR:LiveImproved>/Contents/LICENSE
        )

        add_custom_command(TARGET LiveImproved POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${CMAKE_SOURCE_DIR}/remote_scripts
                $<TARGET_BUNDLE_DIR:LiveImproved>/Contents/Resources/remote-scripts
        )

        add_custom_command(TARGET LiveImproved POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${CMAKE_SOURCE_DIR}/config
                $<TARGET_BUNDLE_DIR:LiveImproved>/Contents/Resources/config
        )

        add_custom_command(TARGET LiveImproved POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
                ${CMAKE_SOURCE_DIR}/resources/StatusIcon.png
                $<TARGET_BUNDLE_DIR:LiveImproved>/Contents/Resources
        )

        add_custom_command(TARGET LiveImproved POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
                $<TARGET_FILE:LiveImprovedDaemon>
                $<TARGET_BUNDLE_DIR:LiveImproved>/Contents/MacOS/LiveImprovedDaemon
        )

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
            advapi32.lib
            gdi32.lib
            ole32.lib
            shell32.lib
            user32.lib
    )

    set_target_properties(LiveImproved PROPERTIES
        OUTPUT_NAME "LiveImproved"
    )

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
            ${COMMON_INCLUDE_DIRS}
            ${CMAKE_SOURCE_DIR}/lib/doctest/doctest
            ${CMAKE_SOURCE_DIR}/abstract
            ${CMAKE_SOURCE_DIR}/src/include
            ${CMAKE_SOURCE_DIR}/src/include/core
            ${CMAKE_SOURCE_DIR}/src/include/event
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

add_executable(LiveImprovedDaemon
    ${CMAKE_SOURCE_DIR}/src/daemon/main.mm
    ${CMAKE_SOURCE_DIR}/src/daemon/LiveObserver.mm
    ${CMAKE_SOURCE_DIR}/src/platform/macos/system/PID.mm
    ${CMAKE_SOURCE_DIR}/src/platform/macos/system/PathFinder.mm
    ${CMAKE_SOURCE_DIR}/src/core/LogGlobal.cpp
)

set_target_properties(LiveImprovedDaemon PROPERTIES
    XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED "NO"
    XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY ""
)

target_include_directories(LiveImprovedDaemon PRIVATE
    ${CMAKE_SOURCE_DIR}/src/daemon
    ${CMAKE_SOURCE_DIR}/src/include/system
    ${CMAKE_SOURCE_DIR}/src/platform/macos/system
    ${CMAKE_SOURCE_DIR}/src/include/core
    ${CMAKE_SOURCE_DIR}/src/include/core/sinks
    ${CMAKE_SOURCE_DIR}/src/include/system
    ${CMAKE_SOURCE_DIR}/src/abstract
    ${CMAKE_SOURCE_DIR}/lib/fmt/include
)

target_link_libraries(LiveImprovedDaemon
    PRIVATE
        "-framework Cocoa"
        "-framework ApplicationServices"

        # maybe not needed
        "-framework CoreFoundation"
        "-framework CoreGraphics"
        "-framework CoreImage"
        "-framework CoreVideo"
        "-framework IOKit"
        "-framework QuartzCore"
        "-framework Security"
)

target_compile_definitions(LiveImprovedDaemon PRIVATE NO_JUCE)
target_compile_definitions(LiveImprovedDaemon PRIVATE FMT_HEADER_ONLY)
