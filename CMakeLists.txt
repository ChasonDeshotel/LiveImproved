cmake_minimum_required(VERSION 3.16)
set(BUILD_TESTING OFF)
set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED "NO")
set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "")
set(CMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "")

# Set project name and version
project(LiveImproved VERSION 1.0 LANGUAGES CXX)

add_definitions(-DDEBUG)
set(CMAKE_BUILD_TYPE Debug)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Define CMAKE_CXX_CLANG_TIDY target
find_program(CLANG_TIDY_EXE NAMES "clang-tidy")
if(CLANG_TIDY_EXE)
    set(CMAKE_CXX_CLANG_TIDY 
        ${CLANG_TIDY_EXE}
        -p=${CMAKE_BINARY_DIR}
        -header-filter=.*\.(cpp|h)$
        -checks=-*,cppcoreguidelines-*,modernize-*
    )
else()
    message(WARNING "clang-tidy not found. Static analysis will be skipped.")
endif()

message(STATUS "Note: HarfBuzz is planning to remove CMake support in the future. Consider using Meson for HarfBuzz.")

# Custom target for running clang-tidy
add_custom_target(
    tidy
    COMMAND ${CMAKE_COMMAND} -E echo "Running clang-tidy..."
    COMMAND find ${CMAKE_SOURCE_DIR}/src -name '*.cpp' -or -name '*.h' | 
            grep -v "lib/JUCE\\|lib/yaml-cpp\\|lib/harfbuzz\\|lib/pugixml" |
            xargs -I {} ${CLANG_TIDY_EXE} -p=${CMAKE_BINARY_DIR} -header-filter=.*\\.(cpp|h)$ -checks=-*,cppcoreguidelines-*,modernize-* {}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Running clang-tidy on source files"
)

# Set the C++ standard and Objective-C++ flag
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(APPLE)
	set(CMAKE_OBJCXX_FLAGS "${CMAKE_OBJCXX_FLAGS} -ObjC++")
endif()

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Add subdirectories after the tidy target
add_subdirectory(lib/JUCE)
add_subdirectory(lib/yaml-cpp)
add_subdirectory(lib/harfbuzz)
add_subdirectory(lib/pugixml)

set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} ${CMAKE_SOURCE_DIR}/lib/yaml-cpp)
set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} ${CMAKE_SOURCE_DIR}/lib/harfbuzz)

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
        yaml-cpp
        harfbuzz
        pugixml
)

# cross-platform nclude directories
include_directories(
    ${CMAKE_SOURCE_DIR}/src/include/juce
    ${CMAKE_SOURCE_DIR}/lib/juce/modules
    ${CMAKE_SOURCE_DIR}/src/include
    ${CMAKE_SOURCE_DIR}/src/interface
)
if(APPLE)
    message(STATUS "Including macOS-specific directories")
    include_directories(${CMAKE_SOURCE_DIR}/src/include/platform/macos)
elseif(WIN32)
    message(STATUS "Including Windows-specific directories")
    include_directories(${CMAKE_SOURCE_DIR}/src/include/platform/windows)
endif()

target_compile_definitions(LiveImproved PRIVATE JUCE_APP_CONFIG_HEADER="${CMAKE_SOURCE_DIR}/src/include/juce/AppConfig.h")

# Source files for C++ and Objective-C++
set(SRC_CPP
    src/Main.cpp
    src/LogHandler.cpp
    src/WindowManager.cpp
    src/KeyMapper.cpp
    src/PluginManager.cpp
    src/Theme.cpp
    src/config/ConfigManager.cpp
    src/config/ConfigMenu.cpp
    src/gui/SearchBox.cpp
    src/ActionHandler.cpp
    src/ResponseParser.cpp
)

if(APPLE)
    set(SRC_OBJCXX
        src/platform/macos/PlatformInitializer.mm
        src/platform/macos/ContextMenu.mm
        src/platform/macos/PID.mm
        src/platform/macos/EventHandler.mm
        src/platform/macos/KeySender.mm
        src/platform/macos/LiveInterface.mm
    )
    list(APPEND SRC_CPP
        src/platform/macos/IPC.cpp
    )

elseif(WIN32)
    list(APPEND SRC_CPP
        src/platform/windows/IPC.cpp
        src/platform/windows/ContextMenu.cpp
        src/platform/windows/PID.cpp
        src/platform/windows/EventHandler.cpp
        src/platform/windows/KeySender.cpp
        src/platform/windows/LiveInterface.cpp
    )
endif()

if(APPLE)
	# Add the C++ and Objective-C++ files to the target
	target_sources(LiveImproved
	    PRIVATE
		${SRC_CPP}
		${SRC_OBJCXX}
	)
elseif(WIN32)
	target_sources(LiveImproved
	    PRIVATE
		${SRC_CPP}
	)
endif()

if(APPLE)
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
elseif(WIN32)
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


# Testing
# suppress deprecated warning
#if (POLICY CMP0167)
#    cmake_policy(SET CMP0167 OLD)
#endif()
#find_package(Boost 1.86.0 REQUIRED COMPONENTS unit_test_framework)
#
#add_executable(test_KeyMapper ${CMAKE_SOURCE_DIR}/test/test_KeyMapper.cpp)
#target_link_libraries(test_KeyMapper Boost::unit_test_framework)
#
#add_executable(
#    test_config_ConfigManager
#    ${CMAKE_SOURCE_DIR}/mock/MockLogHandler.cpp
#    ${CMAKE_SOURCE_DIR}/src/KeyMapper.cpp
#    ${CMAKE_SOURCE_DIR}/src/config/ConfigManager.cpp
#    ${CMAKE_SOURCE_DIR}/test/test_config_ConfigManager.cpp
#)
#target_link_libraries(test_config_ConfigManager
#    Boost::unit_test_framework
#    /usr/local/Cellar/yaml-cpp/0.8.0/lib/libyaml-cpp.dylib
#)
#
#enable_testing()
#
#add_test(NAME test_KeyMapper COMMAND ${CMAKE_BINARY_DIR}/bin/test_KeyMapper)
#add_test(NAME test_config_ConfigManager COMMAND ${CMAKE_BINARY_DIR}/bin/test_config_ConfigManager)
